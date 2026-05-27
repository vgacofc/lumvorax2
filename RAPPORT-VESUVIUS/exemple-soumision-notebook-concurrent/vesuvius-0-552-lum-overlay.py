# ============================================================
# Vesuvius — SINGLE-PATH DROP-IN (public-anchored + private-seeded hysteresis)
# Goal: beat/at least match public 0.55 using ONE safe upgrade:
#   strong = (private_prob >= 0.90)
#   weak   = (private_prob >= 0.50) OR (public_argmax != 0)
#   mask   = binary_propagation(strong within weak) + closing + dust
#
# Writes: /kaggle/working/submission.zip
# ============================================================

from IPython.display import clear_output
import os

# protobuf compatibility shim (protobuf>=5 removed MessageFactory.GetPrototype used by some deps)
try:
    from google.protobuf import message_factory as _pb_message_factory
    if not hasattr(_pb_message_factory.MessageFactory, "GetPrototype"):
        def _compat_get_prototype(self, descriptor):
            return _pb_message_factory.GetMessageClass(descriptor)
        _pb_message_factory.MessageFactory.GetPrototype = _compat_get_prototype
except Exception:
    pass

# protobuf stability (common Kaggle container mismatch)
os.environ.setdefault("PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION", "python")
os.environ["KERAS_BACKEND"] = "jax"
os.environ.setdefault("OMP_NUM_THREADS", "4")
os.environ.setdefault("MKL_NUM_THREADS", "4")
os.environ.setdefault("OPENBLAS_NUM_THREADS", "4")
os.environ.setdefault("NUMEXPR_NUM_THREADS", "4")

import subprocess
from pathlib import Path

var = Path("/kaggle/input/vsdetection-packages-offline-installer-only/whls")
whl_patterns = [
    "keras_nightly-*.whl",
    "tifffile-*.whl",
    "imagecodecs-*.whl",
    "medicai-*.whl",
]
whls = []
for pat in whl_patterns:
    found = sorted(var.glob(pat))
    if not found:
        raise FileNotFoundError(f"Missing wheel for pattern: {pat} in {var}")
    whls.append(str(found[0]))

subprocess.check_call([
    "python", "-m", "pip", "install", *whls,
    "--no-index", "--find-links", str(var)
])
clear_output()

import time, zipfile
import numpy as np
import pandas as pd
import tifffile
import scipy.ndimage as ndi
from skimage.morphology import remove_small_objects
import keras
from medicai.transforms import Compose, NormalizeIntensity
from medicai.models import TransUNet
from medicai.utils.inference import SlidingWindowInference

print("Keras backend:", keras.config.backend(), "Keras version:", keras.version())

# ----------------------------
# CONFIG (minimal, safe)
# ----------------------------
CFG = dict(
    # model
    kaggle_model_path="/kaggle/input/vsd-model/keras/",
    weights_relpath="transunet/3/transunet.seresnext50.160px.comboloss.weights.h5",

    # SWI overlaps
    overlap_public=0.42,  # EXACT public 0.55
    overlap_base=0.43,    # EXACT private 0.55 (important!)
    overlap_hi=0.60,      # OV06
    OV06_MAIN_ONLY=True,

    # TTA
    USE_TTA=True,

    # binary logit definition (match your private 0.55)
    INK_MODE="fg12",

    # thresholds (match your private 0.55)
    T_low=0.50,
    T_high=0.90,

    # topology (match both notebooks' common setting)
    z_radius=3,
    xy_radius=2,
    dust_min_size=100,

    # warmup
    DO_WARMUP=True,

)

# SHF/RSR overlay isolated from competitor baseline config
# Baseline-safe defaults: disabled unless explicitly enabled
LUM_CFG = dict(
    # synchronized with competitor baseline parameters
    USE_LUM_RESONANCE=True,
    LUM_BLEND_ALPHA=CFG["overlap_base"],
    LUM_SPECTRAL_GAMMA=CFG["overlap_hi"],
    LUM_PRIOR_WEIGHT=CFG["overlap_public"],
    LUM_SMOOTH_STEPS=CFG["z_radius"],
    LUM_PRIOR_THRESHOLD=CFG["T_low"],
)

root_dir = "/kaggle/input/vesuvius-challenge-surface-detection"
test_dir = f"{root_dir}/test_images"
output_dir = "/kaggle/working/submission_masks"
zip_path = "/kaggle/working/submission.zip"
os.makedirs(output_dir, exist_ok=True)

test_df = pd.read_csv(f"{root_dir}/test.csv")
ids = test_df["id"].tolist()
print("Num test volumes:", len(ids))

ROI = (160, 160, 160)

# ----------------------------
# Transform (same style)
# ----------------------------
_val_pipeline = Compose([
    NormalizeIntensity(keys=["image"], nonzero=True, channel_wise=False),
])
def val_transformation(image):
    return _val_pipeline({"image": image})["image"]

def load_volume(path):
    vol = tifffile.imread(path).astype(np.float32)
    return vol[None, ..., None]  # (1, D, H, W, 1)

# ----------------------------
# Numerics
# ----------------------------
def sigmoid_stable(x):
    x = np.asarray(x, dtype=np.float32)
    out = np.empty_like(x, dtype=np.float32)
    pos = x >= 0
    out[pos] = 1.0 / (1.0 + np.exp(-x[pos]))
    ex = np.exp(x[~pos])
    out[~pos] = ex / (1.0 + ex)
    return out

def logsumexp2(a, b):
    a = np.asarray(a, dtype=np.float32)
    b = np.asarray(b, dtype=np.float32)
    m = np.maximum(a, b)
    return m + np.log(np.exp(a - m) + np.exp(b - m) + 1e-12)

def binary_logit_from_multiclass_logits(logits_5d, mode="fg12"):
    x = np.asarray(logits_5d, dtype=np.float32)[0]  # (D,H,W,3)
    L0, L1, L2 = x[...,0], x[...,1], x[...,2]
    if mode == "fg12":
        return (logsumexp2(L1, L2) - L0).astype(np.float32, copy=False)
    elif mode == "class1":
        return (L1 - logsumexp2(L0, L2)).astype(np.float32, copy=False)
    else:
        raise ValueError(f"Unknown INK_MODE={mode}")

# ----------------------------
# LUM/VORAX SHF-RSR helpers
# ----------------------------
def shf_spectral_signature(prob):
    p = np.asarray(prob, dtype=np.float32)
    fft_mag = np.abs(np.fft.rfftn(p)).astype(np.float32, copy=False)
    hi = float(np.percentile(fft_mag, 92.0))
    lo = float(np.percentile(fft_mag, 60.0))
    score = hi / (lo + 1e-6)
    return float(np.clip(score, 0.0, 8.0))

def rsr_recursive_prior(pub_fg_bool, smooth_steps=2):
    prior = pub_fg_bool.astype(np.float32, copy=False)
    for _ in range(max(0, int(smooth_steps))):
        prior = ndi.gaussian_filter(prior, sigma=(0.0, 1.2, 1.2))
        prior = np.clip(prior, 0.0, 1.0)
    return prior

def lum_resonance_blend(prob, pub_fg_bool, lum_cfg):
    spectral = shf_spectral_signature(prob)
    spectral_gain = np.clip((spectral - 1.0) * lum_cfg["LUM_SPECTRAL_GAMMA"], 0.0, 1.0)

    prior = rsr_recursive_prior(pub_fg_bool, smooth_steps=lum_cfg["LUM_SMOOTH_STEPS"])
    prior_gate = (prior >= lum_cfg["LUM_PRIOR_THRESHOLD"]).astype(np.float32)

    alpha = float(lum_cfg["LUM_BLEND_ALPHA"])
    w_prior = float(lum_cfg["LUM_PRIOR_WEIGHT"])
    blended = prob * (1.0 - alpha) + alpha * ((1.0 - w_prior) * prob + w_prior * prior_gate)
    boosted = np.clip(blended + spectral_gain * prior * 0.08, 0.0, 1.0).astype(np.float32, copy=False)

    return boosted, {
        "lum_spectral_score": float(spectral),
        "lum_spectral_gain": float(spectral_gain),
        "lum_prior_mean": float(prior.mean()),
        "lum_prior_gate_rate": float(prior_gate.mean()),
    }

# ----------------------------
# Topology helpers
# ----------------------------
def build_anisotropic_struct(z_radius: int, xy_radius: int):
    z, r = int(z_radius), int(xy_radius)
    if z == 0 and r == 0:
        return None
    if z == 0 and r > 0:
        size = 2*r + 1
        struct = np.zeros((1, size, size), dtype=bool)
        cy = cx = r
        for dy in range(-r, r+1):
            for dx in range(-r, r+1):
                if dy*dy + dx*dx <= r*r:
                    struct[0, cy+dy, cx+dx] = True
        return struct
    if z > 0 and r == 0:
        struct = np.zeros((2*z+1, 1, 1), dtype=bool)
        struct[:, 0, 0] = True
        return struct
    depth = 2*z + 1
    size  = 2*r + 1
    struct = np.zeros((depth, size, size), dtype=bool)
    cz = z; cy = cx = r
    for dz in range(-z, z+1):
        for dy in range(-r, r+1):
            for dx in range(-r, r+1):
                if dy*dy + dx*dx <= r*r:
                    struct[cz+dz, cy+dy, cx+dx] = True
    return struct

def seeded_hysteresis_with_topology(
    prob, pub_fg_bool,
    T_low=0.50, T_high=0.90,
    z_radius=3, xy_radius=2, dust_min_size=100
):
    prob = np.asarray(prob, dtype=np.float32)
    strong = prob >= float(T_high)

    # SAFE expansion: weak includes private weak OR public foreground
    weak = (prob >= float(T_low)) | pub_fg_bool

    if not strong.any():
        return np.zeros_like(prob, dtype=np.uint8)

    struct_hyst = ndi.generate_binary_structure(3, 3)
    mask = ndi.binary_propagation(strong, mask=weak, structure=struct_hyst)

    if not mask.any():
        return np.zeros_like(prob, dtype=np.uint8)

    struct_close = build_anisotropic_struct(z_radius, xy_radius)
    if struct_close is not None:
        mask = ndi.binary_closing(mask, structure=struct_close)

    if int(dust_min_size) > 0:
        mask = remove_small_objects(mask.astype(bool), min_size=int(dust_min_size))

    return mask.astype(np.uint8)

# ----------------------------
# Model + SWI (logits)
# ----------------------------
weights_path = f"{CFG['kaggle_model_path']}/{CFG['weights_relpath']}"

model = TransUNet(
    input_shape=(160, 160, 160, 1),
    encoder_name="seresnext50",
    classifier_activation=None,  # TRUE logits
    num_classes=3,
)
model.load_weights(weights_path)
print("Model params (M):", model.count_params() / 1e6)

def build_swi(overlap):
    return SlidingWindowInference(
        model,
        num_classes=3,
        roi_size=ROI,
        sw_batch_size=1,
        mode="gaussian",
        overlap=float(overlap),
    )

swi_public = build_swi(CFG["overlap_public"])  # public 0.55
swi_base   = build_swi(CFG["overlap_base"])    # private base 0.55
swi_hi     = build_swi(CFG["overlap_hi"])      # OV06

# ----------------------------
# TTA (same set)
# ----------------------------
def iter_tta(volume):
    yield volume, (lambda y: y)
    for axis in [1, 2, 3]:
        v = np.flip(volume, axis=axis)
        inv = (lambda y, axis=axis: np.flip(y, axis=axis))
        yield v, inv
    for k in [1, 2, 3]:
        v = np.rot90(volume, k=k, axes=(2, 3))
        inv = (lambda y, k=k: np.rot90(y, k=-k, axes=(2, 3)))
        yield v, inv

# ----------------------------
# Predict BOTH streams in one loop (single final path)
# - Public: mean multiclass logits -> argmax labels
# - Private: OV06 main-only + mean binary logits -> prob
# ----------------------------
def predict_pub_labels_and_private_prob(volume):
    mode = CFG["INK_MODE"]

    if not CFG["USE_TTA"]:
        l_pub = np.asarray(swi_public(volume))
        pub_labels = l_pub.argmax(-1).astype(np.uint8).squeeze()

        l_prv = np.asarray(swi_hi(volume))
        s = binary_logit_from_multiclass_logits(l_prv, mode=mode)
        prob = sigmoid_stable(s)
        return pub_labels, prob

    logits_sum = None
    s_sum = None
    n = 0

    for t, (v, inv) in enumerate(iter_tta(volume)):
        # public stream
        l_pub = np.asarray(swi_public(v))
        l_pub = inv(l_pub)
        logits_sum = l_pub.astype(np.float32) if logits_sum is None else (logits_sum + l_pub.astype(np.float32))

        # private stream (OV06 main-only)
        if CFG["OV06_MAIN_ONLY"]:
            swi_use = swi_hi if (t == 0) else swi_base
        else:
            swi_use = swi_hi

        l_prv = np.asarray(swi_use(v))
        l_prv = inv(l_prv)
        s = binary_logit_from_multiclass_logits(l_prv, mode=mode)
        s_sum = s.astype(np.float32) if s_sum is None else (s_sum + s.astype(np.float32))

        n += 1

    mean_logits = logits_sum / float(n)
    pub_labels = mean_logits.argmax(-1).astype(np.uint8).squeeze()

    s_mean = (s_sum / float(n)).astype(np.float32, copy=False)
    prob = sigmoid_stable(s_mean)
    return pub_labels, prob

# ----------------------------
# Warmup (compile once)
# ----------------------------
def warmup(volume):
    _ = np.asarray(swi_public(volume))
    _ = np.asarray(swi_base(volume))
    _ = np.asarray(swi_hi(volume))

# ----------------------------
# Run + zip
# ----------------------------
print("CFG:",
      f"overlap_public={CFG['overlap_public']}, overlap_base={CFG['overlap_base']}, overlap_hi={CFG['overlap_hi']},",
      f"INK_MODE={CFG['INK_MODE']}, T_low={CFG['T_low']}, T_high={CFG['T_high']},",
      f"OV06_MAIN_ONLY={CFG['OV06_MAIN_ONLY']},",
      f"USE_LUM_RESONANCE={LUM_CFG['USE_LUM_RESONANCE']}")

t_global0 = time.perf_counter()
run_rows = []

with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
    for i, image_id in enumerate(ids):
        t0 = time.perf_counter()

        tif_path = f"{test_dir}/{image_id}.tif"
        volume = load_volume(tif_path)
        volume = val_transformation(volume)

        if i == 0 and CFG["DO_WARMUP"]:
            print("Warming up JAX (compile once)...")
            warmup(volume)

        pub_labels, prob = predict_pub_labels_and_private_prob(volume)

        # public fg anchor (no topology here; used only as weak region expansion)
        pub_fg = (pub_labels != 0)

        lum_metrics = {
            "lum_spectral_score": 0.0,
            "lum_spectral_gain": 0.0,
            "lum_prior_mean": 0.0,
            "lum_prior_gate_rate": 0.0,
        }
        prob_for_output = prob
        if LUM_CFG["USE_LUM_RESONANCE"]:
            prob_for_output, lum_metrics = lum_resonance_blend(prob, pub_fg, LUM_CFG)

        # single final path
        output = seeded_hysteresis_with_topology(
            prob_for_output,
            pub_fg_bool=pub_fg,
            T_low=CFG["T_low"],
            T_high=CFG["T_high"],
            z_radius=CFG["z_radius"],
            xy_radius=CFG["xy_radius"],
            dust_min_size=CFG["dust_min_size"],
        )

        out_path = f"{output_dir}/{image_id}.tif"
        tifffile.imwrite(out_path, output.astype(np.uint8))
        zf.write(out_path, arcname=f"{image_id}.tif")
        os.remove(out_path)

        dt = time.perf_counter() - t0
        elapsed = time.perf_counter() - t_global0
        run_rows.append({
            "id": image_id,
            "positives": int(output.sum()),
            **lum_metrics,
            "minutes": float(dt / 60.0),
        })
        print(f"[{i+1}/{len(ids)}] id={image_id} | {dt/60:.2f} min | elapsed {elapsed/3600:.2f} h | positives={int(output.sum())} | lum_score={lum_metrics['lum_spectral_score']:.3f}")

if run_rows:
    run_df = pd.DataFrame(run_rows)
    print("\nLUM resonance summary:")
    print(run_df[["id", "positives", "lum_spectral_score", "lum_spectral_gain", "lum_prior_mean", "lum_prior_gate_rate", "minutes"]].to_string(index=False))

print("Submission ZIP:", zip_path)



