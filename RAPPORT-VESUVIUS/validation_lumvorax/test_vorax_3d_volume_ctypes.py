import ctypes
import subprocess
import tempfile
from pathlib import Path

import numpy as np


class VoraxVolume3D(ctypes.Structure):
    _fields_ = [
        ("data", ctypes.POINTER(ctypes.c_float)),
        ("depth", ctypes.c_size_t),
        ("height", ctypes.c_size_t),
        ("width", ctypes.c_size_t),
    ]


def build_lib(out_path: Path) -> Path:
    cmd = [
        "gcc",
        "-shared",
        "-fPIC",
        "-O2",
        "-Isrc",
        "src/vorax/vorax_3d_volume.c",
        "-o",
        str(out_path),
    ]
    subprocess.check_call(cmd)
    return out_path


def main() -> int:
    with tempfile.TemporaryDirectory() as td:
        so_path = build_lib(Path(td) / "libvorax3d.so")
        lib = ctypes.CDLL(str(so_path))

        lib.vorax_volume3d_validate.argtypes = [ctypes.POINTER(VoraxVolume3D)]
        lib.vorax_volume3d_validate.restype = ctypes.c_int

        lib.vorax_volume3d_normalize.argtypes = [ctypes.POINTER(VoraxVolume3D)]
        lib.vorax_volume3d_normalize.restype = ctypes.c_int

        lib.vorax_volume3d_threshold.argtypes = [
            ctypes.POINTER(VoraxVolume3D),
            ctypes.c_float,
            ctypes.POINTER(ctypes.c_uint8),
            ctypes.c_size_t,
        ]
        lib.vorax_volume3d_threshold.restype = ctypes.c_int

        # 128*128*8 = 131072 voxels (~10-100K+ stress band)
        arr = np.random.default_rng(5).random((8, 128, 128), dtype=np.float32)
        ptr = arr.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
        vol = VoraxVolume3D(ptr, arr.shape[0], arr.shape[1], arr.shape[2])

        assert lib.vorax_volume3d_validate(ctypes.byref(vol)) == 1
        assert lib.vorax_volume3d_normalize(ctypes.byref(vol)) == 1

        mask = np.zeros(arr.size, dtype=np.uint8)
        mask_ptr = mask.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8))
        assert lib.vorax_volume3d_threshold(ctypes.byref(vol), ctypes.c_float(0.5), mask_ptr, mask.size) == 1

        print("vorax3d_ctypes_ok")
        print(f"voxels={arr.size}")
        print(f"mask_ones={int(mask.sum())}")
        print(f"mask_ratio={float(mask.mean()):.6f}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
