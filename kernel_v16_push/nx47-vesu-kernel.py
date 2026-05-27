# %% [code]
# CELL 1 — ENVIRONMENT & DATASET AUDIT (NX47 V10.3)
# Responsibility: verify Kaggle environment and locate dataset root

import os
import time
import json
import hashlib
import threading
from typing import Optional, Dict

# --------------------------------------------------
# Forensic Logger (shared pattern)
# --------------------------------------------------
class ForensicLogger:
    def __init__(self, outdir="/kaggle/working/nx47_logs"):
        os.makedirs(outdir, exist_ok=True)
        self.sid = hashlib.sha256(str(time.time_ns()).encode()).hexdigest()[:16]
        self.path = os.path.join(outdir, f"nx47_{self.sid}.log")
        self.lock = threading.Lock()

    def log(self, msg: str, level: str = "INFO", data: Optional[Dict] = None):
        ts = time.time_ns()
        line = f"[{ts}][{level}] {msg}"
        if data is not None:
            line += " | DATA: " + json.dumps(data, ensure_ascii=False)
        print(line)
        with self.lock:
            with open(self.path, "a", encoding="utf-8") as f:
                f.write(line + "\n")

logger = ForensicLogger()

# --------------------------------------------------
# Dataset audit
# --------------------------------------------------
EXPECTED_DATASET = "vesuvius-challenge-surface-detection"
BASE_PATH = "/kaggle/input"

logger.log("CELL1_START_ENV_AUDIT", data={"base_path": BASE_PATH})

if not os.path.exists(BASE_PATH):
    logger.log("NON_KAGGLE_ENVIRONMENT", level="ERROR")
    raise RuntimeError("/kaggle/input not found")

entries = os.listdir(BASE_PATH)
logger.log("INPUT_DIRECTORY_LISTED", data={"entries": entries})

if EXPECTED_DATASET not in entries:
    logger.log("DATASET_NOT_FOUND", level="ERROR", data={"expected": EXPECTED_DATASET})
    raise RuntimeError("Vesuvius dataset not attached")

dataset_path = os.path.join(BASE_PATH, EXPECTED_DATASET)
logger.log("DATASET_ROOT_CONFIRMED", data={"dataset_path": dataset_path})

print("CELL 1 OK — DATASET ROOT VERIFIED")

# %% [code]
# CELL 2 — FILE DISCOVERY (NX47 V10.3)
# Responsibility: recursively discover all files, no assumptions

import os
from collections import defaultdict

logger.log("CELL2_START_FILE_DISCOVERY")

all_files = []
for root, dirs, files in os.walk(dataset_path):
    for f in files:
        all_files.append(os.path.join(root, f))

logger.log(
    "CELL2_DISCOVERY_COMPLETE",
    data={"total_files": len(all_files), "sample": all_files[:25]}
)

if not all_files:
    raise RuntimeError("CELL2_FAILURE: dataset root contains no files")

print(f"CELL 2 OK — {len(all_files)} FILES DISCOVERED")

# %% [code]
# CELL 3 — CSV AUDIT (NX47 V10.3)
# Responsibility: validate train.csv structure

import pandas as pd

logger.log("CELL3_START_CSV_AUDIT")

csv_path = os.path.join(dataset_path, "train.csv")
if not os.path.exists(csv_path):
    raise RuntimeError("CELL3_FAILURE: train.csv not found")

train_df = pd.read_csv(csv_path)

logger.log(
    "CELL3_CSV_LOADED",
    data={
        "rows": len(train_df),
        "columns": list(train_df.columns)
    }
)

print("CELL 3 OK — train.csv LOADED")

# %% [code]
# CELL 4 — IMAGE LOAD (NX47 V10.3)
# Responsibility: load real TIFF images

from PIL import Image

logger.log("CELL4_START_IMAGE_LOAD")

image_dir = os.path.join(dataset_path, "train_images")
if not os.path.isdir(image_dir):
    raise RuntimeError("CELL4_FAILURE: train_images directory not found")

image_files = sorted([
    os.path.join(image_dir, f)
    for f in os.listdir(image_dir)
    if f.lower().endswith(".tif")
])

if not image_files:
    raise RuntimeError("CELL4_FAILURE: no TIFF images found")

# Load a small deterministic sample
sample_images = image_files[:5]
loaded = []
for p in sample_images:
    img = Image.open(p)
    loaded.append((p, img.size, img.mode))

logger.log(
    "CELL4_IMAGES_LOADED",
    data={"count": len(loaded), "details": loaded}
)

print("CELL 4 OK — IMAGES LOADED")

# %% [code]
# CELL 5 — IMAGE STATISTICS (NX47 V10.3)
# Responsibility: compute real stats and checksums

import numpy as np
import hashlib

logger.log("CELL5_START_IMAGE_STATS")

stats = []
for path, size, mode in loaded:
    with open(path, "rb") as f:
        sha = hashlib.sha256(f.read()).hexdigest()
    img = Image.open(path)
    arr = np.array(img)
    stats.append({
        "file": os.path.basename(path),
        "shape": arr.shape,
        "dtype": str(arr.dtype),
        "min": int(arr.min()),
        "max": int(arr.max()),
        "mean": float(arr.mean()),
        "sha256": sha
    })

logger.log("CELL5_IMAGE_STATS", data={"images": stats})

print("CELL 5 OK — IMAGE STATS COMPUTED")

# %% [code]
# CELL 6 — NX47 / ARC HOOK (NX47 V10.3)
# Responsibility: handoff point to ARC kernel

logger.log("CELL6_START_NX47_ARC_HOOK")

nx47_payload = {
    "dataset": EXPECTED_DATASET,
    "total_files": len(all_files),
    "image_sample_stats": stats,
    "timestamp": time.time_ns()
}

logger.log("CELL6_PAYLOAD_READY", data=nx47_payload)

print("CELL 6 OK — NX47 PAYLOAD READY FOR ARC")
