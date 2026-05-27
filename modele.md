# ============================================================================
# NX47 VESUVIUS — ADVANCED 3D KERNEL (Z-WEIGHTED PROJECTION)
# SINGLE CELL — KAGGLE STRICT
# ============================================================================

import gc, zipfile
from pathlib import Path
import numpy as np
import pandas as pd
import tifffile
from scipy import ndimage as ndi
from skimage.morphology import remove_small_objects
from PIL import Image, ImageSequence

# ============================================================================
# CONFIG
# ============================================================================
ROOT = Path("/kaggle/input/vesuvius-challenge-surface-detection")
TEST_DIR = ROOT / "test_images"
TEST_CSV = ROOT / "test.csv"

WORK = Path("/kaggle/working")
TMP = WORK / "tmp"
ZIP_PATH = WORK / "submission.zip"
TMP.mkdir(exist_ok=True)

T_LOW = 0.45
T_HIGH = 0.80
GAUSS_SIGMA = 1.2
DUST_MIN_SIZE = 300
Z_SIGMA = 0.25   # profondeur préférentielle

# ============================================================================
# IO
# ============================================================================
def load_volume(path):
    try:
        vol = tifffile.imread(str(path))
    except:
        frames = []
        with Image.open(path) as im:
            for f in ImageSequence.Iterator(im):
                frames.append(np.array(f, dtype=np.float32))
        vol = np.stack(frames)

    if vol.ndim == 2:
        vol = vol[None]
    return vol.astype(np.float32)

def normalize(v):
    v -= v.min()
    if v.max() > 0:
        v /= v.max()
    return np.clip(v, 0, 1)

# ============================================================================
# MODEL
# ============================================================================
def predict(vol):
    smooth = ndi.gaussian_filter(vol, sigma=GAUSS_SIGMA)
    high = np.clip(vol - smooth + 0.5, 0, 1)
    return normalize(0.6 * vol + 0.4 * high)

def postprocess(p):
    strong = p >= T_HIGH
    weak = p >= T_LOW
    if not strong.any():
        return np.zeros_like(p, dtype=np.uint8)
    mask = ndi.binary_propagation(strong, mask=weak)
    return remove_small_objects(mask, DUST_MIN_SIZE).astype(np.uint8)

# ============================================================================
# ADVANCED Z PROJECTION
# ============================================================================
def z_weighted_projection(probs):
    z = probs.shape[0]
    zz = np.linspace(-1, 1, z)
    weights = np.exp(-(zz**2) / (2 * Z_SIGMA**2))
    weights /= weights.sum()

    proj = np.tensordot(weights, probs, axes=(0,0))
    return (proj > 0.5).astype(np.uint8) * 255

# ============================================================================
# MAIN
# ============================================================================
df = pd.read_csv(TEST_CSV)

if ZIP_PATH.exists():
    ZIP_PATH.unlink()

with zipfile.ZipFile(ZIP_PATH, "w", zipfile.ZIP_STORED) as zf:
    for i, img_id in enumerate(df["id"], 1):
        print(f"[{i}/{len(df)}] {img_id}")

        vol = normalize(load_volume(TEST_DIR / f"{img_id}.tif"))
        probs = predict(vol)
        mask3d = postprocess(probs)
        mask2d = z_weighted_projection(mask3d)

        out = TMP / f"{img_id}.tif"
        tifffile.imwrite(out, mask2d, dtype=np.uint8, photometric="minisblack")
        zf.write(out, arcname=f"{img_id}.tif")
        out.unlink()

        del vol, probs, mask3d, mask2d
        gc.collect()

print("SUBMISSION READY:", ZIP_PATH)