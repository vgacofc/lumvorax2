"""Convert a competitor TIFF volume to LUMV1 container with forensic logs.

Usage:
  python3 convert_competitor_tiff_to_lum.py --input /path/file.tif --output /path/file.lum --log /path/log.json
"""

import argparse
import json
import struct
import time
from hashlib import sha512
from pathlib import Path

import numpy as np
import tifffile

LUM_MAGIC = b"LUMV1\x00\x00\x00"


def normalize_volume(arr: np.ndarray) -> np.ndarray:
    if arr.ndim == 2:
        arr = arr[np.newaxis, :, :]
    if arr.ndim != 3:
        raise ValueError(f"Expected 2D/3D TIFF, got shape={arr.shape}")
    return np.ascontiguousarray(arr.astype(np.float32, copy=False))


def encode_lum(arr3d: np.ndarray) -> bytes:
    z, h, w = arr3d.shape
    payload = arr3d.tobytes()
    digest = sha512(payload).digest()
    header = struct.pack("<8sIII16s", LUM_MAGIC, z, h, w, digest[:16])
    return header + payload


def decode_lum(blob: bytes) -> np.ndarray:
    header_size = struct.calcsize("<8sIII16s")
    magic, z, h, w, digest16 = struct.unpack("<8sIII16s", blob[:header_size])
    if magic != LUM_MAGIC:
        raise ValueError("invalid LUM magic")
    payload = blob[header_size:]
    expected = int(z) * int(h) * int(w) * 4
    if len(payload) != expected:
        raise ValueError("payload size mismatch")
    if sha512(payload).digest()[:16] != digest16:
        raise ValueError("payload checksum mismatch")
    return np.frombuffer(payload, dtype=np.float32).reshape((z, h, w))


def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--input", required=True)
    p.add_argument("--output", required=True)
    p.add_argument("--log", required=True)
    args = p.parse_args()

    t0 = time.time()
    input_path = Path(args.input)
    output_path = Path(args.output)
    log_path = Path(args.log)

    arr = normalize_volume(np.asarray(tifffile.imread(input_path)))
    blob = encode_lum(arr)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(blob)

    restored = decode_lum(blob)

    per_slice = []
    for zi in range(restored.shape[0]):
        sl = restored[zi]
        per_slice.append(
            {
                "z": zi,
                "min": float(sl.min()),
                "max": float(sl.max()),
                "mean": float(sl.mean()),
                "std": float(sl.std()),
                "nonzero_pct": float((sl > 0).mean() * 100.0),
            }
        )

    report = {
        "input": str(input_path),
        "output": str(output_path),
        "shape": [int(x) for x in restored.shape],
        "dtype": str(restored.dtype),
        "global_min": float(restored.min()),
        "global_max": float(restored.max()),
        "global_mean": float(restored.mean()),
        "global_nonzero_pct": float((restored > 0).mean() * 100.0),
        "payload_sha512": sha512(restored.tobytes()).hexdigest(),
        "slice_logs": per_slice,
        "elapsed_s": time.time() - t0,
    }
    log_path.parent.mkdir(parents=True, exist_ok=True)
    log_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps({"status": "ok", "shape": report["shape"], "elapsed_s": report["elapsed_s"]}))


if __name__ == "__main__":
    main()
