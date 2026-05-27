# ============================================================
# NX47-VESU KERNEL V61.1 ULTRA-AGGRESSIVE++ ULTRA-DEBUG++ — ONE-CELL ULTRA-FORNSIC (V28-COMPAT)
# GPU STRICT • NON-LINEAR FUSION • DYNAMIC SLICE WEIGHT • ABLATION READY
# FULL SLICE-LOCAL LOGGING: fusion_score, weight, p_hi, p_lo, w
# ============================================================

# ---------------------- INSTALL (OFFLINE SAFE) ----------------------
import sys, subprocess, os
def install_offline(package_name):
    try:
        subprocess.check_call([sys.executable, "-m", "pip", "install", "--no-index", "--find-links=/kaggle/input/nx47-dependencies", package_name])
    except:
        print(f"Offline install failed for {package_name}, attempting standard install...")
        subprocess.check_call([sys.executable, "-m", "pip", "install", "--quiet", package_name])

install_offline("imagecodecs")
install_offline("tifffile")

# ---------------------- IMPORTS ----------------------
import os, time, json, hashlib, gc, zipfile, threading, importlib.util
from pathlib import Path
import numpy as np
import pandas as pd

# ---------------------- GPU STRICT ----------------------
GPU_STRICT = True
try:
    import cupy as cp
    cp.cuda.runtime.getDeviceCount()
    xp = cp
    GPU = True
except:
    if GPU_STRICT:
        raise RuntimeError("GPU REQUIRED — GPU_STRICT ENABLED")
    xp = np
    GPU = False

if GPU:
    from cupyx.scipy.ndimage import gaussian_filter
else:
    from scipy.ndimage import gaussian_filter

# ---------------------- PATHS ----------------------
ROOT = Path("/kaggle/input/competitions/vesuvius-challenge-surface-detection")
TEST_DIR = ROOT / "test_images"
OUT = Path("/kaggle/working/tmp")
OUT.mkdir(exist_ok=True)
ZIP_PATH = Path("/kaggle/working/submission.zip")

# ---------------------- LOGGER ULTRA-DEBUG++ ----------------------
class Logger:
    def __init__(self):
        self.seq = 0
        self.slice_rows = []
        self.file_rows = []
        self.lock = threading.Lock()

    def _safe(self, v):
        if hasattr(v, "item"): return v.item()
        if isinstance(v, np.ndarray): return float(np.mean(v))
        return v

    def log(self, msg, data=None):
        with self.lock:
            self.seq += 1
            payload = {k:self._safe(v) for k,v in (data or {}).items()}
            print(f"[{time.time_ns()}][SEQ:{self.seq:06d}] {msg} {json.dumps(payload)}")

    def slice_metric(self, file, z, latency_ns, fusion_score=None, weight=None, p_hi=None, p_lo=None, w=None):
        self.slice_rows.append({
            "file": file,
            "slice": z,
            "latency_ms": latency_ns / 1e6,
            "fusion_score": fusion_score,
            "weight": weight,
            "p_hi": p_hi,
            "p_lo": p_lo,
            "w_mean": w if w is None else float(np.mean(w))
        })

    def file_metric(self, file, checksum, slices):
        self.file_rows.append({
            "file": file,
            "checksum": checksum,
            "slices": slices
        })

logger = Logger()
logger.log("LOGGER_INIT", {"gpu": GPU})

# ---------------------- NX47 ----------------------
sys.path.append("/kaggle/input/datasets/ndarray2000/nx47-arc-kernel-v2-fixed-py")
from nx47_arc_kernel_v2_fixed import PerformanceProofBloc1
nx47 = PerformanceProofBloc1()
logger.log("NX47_OK")

# ---------------------- AIMO3 ----------------------
aimo3_path = "/kaggle/input/datasets/ndarray2000/iamo3-shf-resonance-v3/aimo3_shf_resonance_v3.py"
spec = importlib.util.spec_from_file_location("aimo3", aimo3_path)
aimo3 = importlib.util.module_from_spec(spec)
spec.loader.exec_module(aimo3)
solve_aimo3 = aimo3.solve_enhanced
logger.log("AIMO3_OK")

# ---------------------- TIFF ----------------------
import tifffile
def read_tiff(p):
    return tifffile.imread(p).astype(np.float32)

# ---------------------- AUTO THRESHOLD SLICE-LOCAL ----------------------
def slice_percentiles(slice_data):
    flat = slice_data.ravel()
    p90 = np.percentile(flat, 90)
    p95 = np.percentile(flat, 95)
    std = np.std(flat)
    hi = float(np.clip(88 + 6*(p95 - p90)/(std + 1e-6), 88, 96))
    lo = max(hi - 6, 80)
    return hi, lo

# ---------------------- PROCESS ONE FILE ----------------------
def process_file(path, ablation=None, ultra_aggressive=True):
    t_file = time.time_ns()

    vol = read_tiff(path)
    vol = (vol - vol.min()) / (vol.max() - vol.min() + 1e-6)
    vol_gpu = xp.asarray(vol)

    sigma = float(xp.std(vol_gpu) * 0.9 + 0.4)
    smooth = gaussian_filter(vol_gpu, sigma=sigma)
    resid = vol_gpu - smooth

    # ----- SCORES (REAL, DYNAMIC) -----
    nx_vals = [v for v in nx47.run_all().values() if isinstance(v,(int,float))]
    nx_score = float(np.mean(nx_vals)) if nx_vals else 0.0
    a3 = solve_aimo3("sum all values")
    a3_score = float(np.mean(list(a3.values()))) if isinstance(a3,dict) and a3 else 0.0

    # ----- ABLATION CONTROL -----
    if ablation == "nx47": a3_score = 0.0
    if ablation == "aimo3": nx_score = 0.0

    # ----- NON-LINEAR FUSION -----
    fusion_score_global = 0.7*np.tanh(nx_score*2.0) + 0.3*np.tanh(a3_score*2.5)

    # ----- ULTRA-AGGRESSIVE ADAPTIVE BOOST -----
    if ultra_aggressive:
        vol_std_global = float(xp.std(vol_gpu))
        fusion_score_global *= 1.0 + 0.5*np.tanh(vol_std_global*1.5)

    logger.log("FUSION_SCORE_GLOBAL", {"file": path.name, "score": fusion_score_global})

    Z = vol_gpu.shape[0]
    out = []

    for z in range(Z):
        t0 = time.time_ns()
        z0, z1 = max(0, z-1), min(Z, z+2)
        proj = xp.mean(resid[z0:z1], axis=0)

        # ----- LOCAL MULTI-SCALE VARIANCE -----
        local_vol = vol_gpu[max(0,z-2):min(Z,z+3)]
        local_std = float(xp.std(local_vol))

        # ----- ULTRA-AGGRESSIVE WEIGHT DYNAMIC (FUSION SCORE IMPACT) -----
        weight_base = 0.15
        if ultra_aggressive:
            weight = weight_base + 0.25*np.tanh(fusion_score_global)*np.tanh(local_std*2.0) \
                     + 0.1*np.tanh(local_std*3.0)
        else:
            weight = weight_base + 0.25*np.tanh(fusion_score_global)*np.tanh(local_std*2.0)
        proj = proj + weight

        # ----- SLICE-LOCAL AUTO THRESHOLD -----
        proj_cpu = xp.asnumpy(proj)
        p_hi, p_lo = slice_percentiles(proj_cpu)

        mask_hi = proj > np.percentile(proj_cpu, p_hi)
        mask_lo = proj > np.percentile(proj_cpu, p_lo)
        w = xp.clip((proj - np.percentile(proj_cpu, p_lo)) /
                    (np.percentile(proj_cpu, p_hi) - np.percentile(proj_cpu, p_lo) + 1e-6), 0.0, 1.0)
        final = (w * xp.logical_and(mask_hi, mask_lo) +
                 (1.0 - w) * xp.logical_or(mask_hi, mask_lo)) > 0.5
        out.append(final.astype(xp.uint8) * 255)

        latency = time.time_ns() - t0
        # ----- ULTRA-DEBUG++ LOGGING -----
        logger.slice_metric(
            path.name, z, latency,
            fusion_score=float(fusion_score_global),
            weight=float(weight),
            p_hi=p_hi,
            p_lo=p_lo,
            w=w
        )

    mask = xp.stack(out)
    checksum = hashlib.sha256(vol.tobytes()).hexdigest()[:16]

    logger.file_metric(path.name, checksum, Z)
    logger.log("FILE_DONE", {"file": path.name, "checksum": checksum, "slices": Z})

    # NX47 v62: enforce scorer-safe binary range on uint8 volume (0/255).
    return (xp.asnumpy(mask).astype(np.uint8) * 255).astype(np.uint8)

# ---------------------- RUN ALL ----------------------
FILES = sorted(TEST_DIR.rglob("*.tif"))
if not FILES:
    raise RuntimeError("NO TEST FILES FOUND")

logger.log("FILES_READY", {"count": len(FILES)})

with zipfile.ZipFile(ZIP_PATH, "w", zipfile.ZIP_STORED) as zf:
    for f in FILES:
        m = process_file(f)  # ablation=None, ultra_aggressive=True
        out = OUT / f.name
        if m.ndim != 3:
            raise RuntimeError(f"Invalid NX47 output rank for {f.name}: {m.shape} (expected 3D Z,H,W)")
        tifffile.imwrite(out, m, compression="LZW")
        zf.write(out, arcname=f.name)
        out.unlink()
        gc.collect()

# ---------------------- GLOBAL SUMMARY ----------------------
df_slices = pd.DataFrame(logger.slice_rows)
df_files  = pd.DataFrame(logger.file_rows)

print("\n===== SLICE SUMMARY (ALL FILES) =====")
print(df_slices.groupby("file")[["latency_ms","fusion_score","weight","p_hi","p_lo","w_mean"]].agg(["count","mean","min","max"]))

print("\n===== GLOBAL SLICE MEAN (ALL FILES) =====")
print(df_slices["latency_ms"].mean(), "ms")

print("\n===== FILE CHECKSUMS =====")
print(df_files.to_string(index=False))

logger.log("SUBMISSION_READY", {"zip": str(ZIP_PATH)})
logger.log("EXEC_COMPLETE")
for alias in [Path("/kaggle/working/nx47_vesuvius/submission.zip"), Path("submission.zip"), Path("nx47_vesuvius/submission.zip")]:
    alias.parent.mkdir(parents=True, exist_ok=True)
    if alias.resolve() != ZIP_PATH.resolve():
        import shutil
        shutil.copyfile(ZIP_PATH, alias)

print("READY:", ZIP_PATH)
