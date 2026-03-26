import gc
import importlib
import importlib.util
import json
import math
import os
import subprocess
import sys
import time
import zipfile
from dataclasses import dataclass, field
from hashlib import sha256
from pathlib import Path
from typing import Dict, List

import numpy as np
import tifffile
from scipy.ndimage import gaussian_filter as cpu_gaussian_filter
from scipy.ndimage import sobel


cp = importlib.import_module("cupy") if importlib.util.find_spec("cupy") else None
gpu_gaussian_filter = (
    importlib.import_module("cupyx.scipy.ndimage").gaussian_filter
    if importlib.util.find_spec("cupyx.scipy.ndimage")
    else None
)
PIL_Image = importlib.import_module("PIL.Image") if importlib.util.find_spec("PIL.Image") else None
PIL_ImageSequence = importlib.import_module("PIL.ImageSequence") if importlib.util.find_spec("PIL.ImageSequence") else None


@dataclass
class PlanStep:
    name: str
    description: str
    progress: float = 0.0
    done: bool = False


@dataclass
class PlanTracker:
    output_path: Path
    steps: List[PlanStep] = field(default_factory=list)

    def add_step(self, name: str, description: str) -> None:
        self.steps.append(PlanStep(name=name, description=description))

    def update(self, name: str, progress: float, done: bool = False) -> None:
        for step in self.steps:
            if step.name == name:
                step.progress = float(np.clip(progress, 0.0, 100.0))
                step.done = done
                break
        self._write()

    def _write(self) -> None:
        payload = {
            "generated_at_ns": time.time_ns(),
            "roadmap": [
                {
                    "name": s.name,
                    "description": s.description,
                    "progress_percent": round(s.progress, 2),
                    "done": s.done,
                }
                for s in self.steps
            ],
            "overall_progress_percent": round(float(np.mean([s.progress for s in self.steps])) if self.steps else 0.0, 2),
        }
        self.output_path.write_text(json.dumps(payload, indent=2), encoding="utf-8")


@dataclass
class MemoryTracker:
    output_dir: Path
    events: List[Dict[str, object]] = field(default_factory=list)

    def log(self, event: str, **kwargs: object) -> None:
        row = {"ts_ns": time.time_ns(), "event": event, **kwargs}
        self.events.append(row)

    def log_array(self, name: str, arr: np.ndarray, z: int = -1) -> None:
        a = np.asarray(arr)
        self.log(
            "MEMORY_ARRAY",
            name=name,
            z=z,
            shape=list(a.shape),
            dtype=str(a.dtype),
            bytes=int(a.nbytes),
            min=float(a.min()) if a.size else 0.0,
            max=float(a.max()) if a.size else 0.0,
            mean=float(a.mean()) if a.size else 0.0,
            sha256=sha256(a.tobytes()).hexdigest(),
        )

    def flush(self, filename: str = "v114_memory_tracker.json") -> None:
        (self.output_dir / filename).write_text(json.dumps(self.events, indent=2), encoding="utf-8")



def install_offline(package_name: str) -> None:
    exact_wheel_dir = Path("/kaggle/input/datasets/ndarray2000/nx47-dependencies")
    fallback_wheel_dir = Path("/kaggle/input/nx47-dependencies")

    exact_wheels = {
        "imagecodecs": exact_wheel_dir / "imagecodecs-2026.1.14-cp311-abi3-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl",
        "numpy": exact_wheel_dir / "numpy-2.4.2-cp311-cp311-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl",
        "tifffile": exact_wheel_dir / "tifffile-2026.1.28-py3-none-any.whl",
    }

    if package_name == "numpy" and importlib.util.find_spec("numpy"):
        return

    if package_name in exact_wheels and exact_wheels[package_name].exists():
        try:
            subprocess.check_call([sys.executable, "-m", "pip", "install", "--no-index", str(exact_wheels[package_name])])
            return
        except subprocess.CalledProcessError:
            pass

    if exact_wheel_dir.exists():
        subprocess.check_call([sys.executable, "-m", "pip", "install", "--no-index", f"--find-links={exact_wheel_dir}", package_name])
        return

    if fallback_wheel_dir.exists():
        subprocess.check_call([sys.executable, "-m", "pip", "install", "--no-index", f"--find-links={fallback_wheel_dir}", package_name])
        return


def ensure_imagecodecs() -> bool:
    if importlib.util.find_spec("imagecodecs"):
        return True
    install_offline("imagecodecs")
    return importlib.util.find_spec("imagecodecs") is not None


class NX47V114Kernel:
    def __init__(
        self,
        root: Path = Path("/kaggle/input/competitions/vesuvius-challenge-surface-detection"),
        output_dir: Path = Path("/kaggle/working"),
        overlay_stride: int = 8,
        max_dataset_manifest_files: int = 50000,
        per_pixel_trace: bool = False,
    ) -> None:
        self.version = "v114"
        self.root = root
        self.test_dir = self.root / "test_images"
        self.output_dir = output_dir
        self.tmp_dir = output_dir / "tmp_masks"
        self.overlay_dir = output_dir / "overlays"
        self.trace_dir = output_dir / "v114_pixel_trace"
        self.roadmap_path = output_dir / "v114_roadmap_realtime.json"
        self.submission_path = output_dir / "submission.zip"
        self.overlay_stride = max(1, int(overlay_stride))
        self.max_dataset_manifest_files = max(1, int(max_dataset_manifest_files))
        self.per_pixel_trace = bool(per_pixel_trace)

        self.tmp_dir.mkdir(parents=True, exist_ok=True)
        self.overlay_dir.mkdir(parents=True, exist_ok=True)
        self.trace_dir.mkdir(parents=True, exist_ok=True)

        self.plan = PlanTracker(output_path=self.roadmap_path)
        self.plan.add_step("discovery", "Validation des dossiers/format de competition")
        self.plan.add_step("dataset_manifest", "Inventaire complet des fichiers dataset")
        self.plan.add_step("load", "Chargement volume TIFF + normalisation")
        self.plan.add_step("features", "Extraction features + calibration")
        self.plan.add_step("segment", "Segmentation slice-wise avec hysteresis")
        self.plan.add_step("overlay", "Generation overlays")
        self.plan.add_step("package", "Generation submission.zip")
        self.plan._write()

        self.logs: List[Dict[str, object]] = []
        self.mem = MemoryTracker(output_dir=self.output_dir)

    def log(self, event: str, **kwargs: object) -> None:
        row = {"ts_ns": time.time_ns(), "event": event, **kwargs}
        self.logs.append(row)
        print(json.dumps(row, ensure_ascii=False))

    @property
    def using_gpu(self) -> bool:
        return cp is not None and gpu_gaussian_filter is not None

    def _xp(self):
        return cp if self.using_gpu else np

    def _gaussian(self, arr, sigma: float):
        if self.using_gpu:
            return gpu_gaussian_filter(arr, sigma=sigma)
        return cpu_gaussian_filter(arr, sigma=sigma)

    def _quick_sha256(self, path: Path, chunk_size: int = 1024 * 1024) -> str:
        digest = sha256()
        with path.open("rb") as f:
            while True:
                chunk = f.read(chunk_size)
                if not chunk:
                    break
                digest.update(chunk)
        return digest.hexdigest()

    def build_dataset_manifest(self) -> List[Dict[str, object]]:
        self.plan.update("dataset_manifest", 5.0)
        all_files = sorted([p for p in self.root.rglob("*") if p.is_file()])
        total = min(len(all_files), self.max_dataset_manifest_files)
        manifest: List[Dict[str, object]] = []
        for i, path in enumerate(all_files[:total], start=1):
            rel = str(path.relative_to(self.root))
            item = {
                "path": rel,
                "size_bytes": int(path.stat().st_size),
                "sha256": self._quick_sha256(path),
            }
            manifest.append(item)
            if i % max(1, total // 20) == 0:
                self.plan.update("dataset_manifest", 5.0 + 90.0 * (i / max(1, total)))
        self.plan.update("dataset_manifest", 100.0, done=True)
        self.log("DATASET_MANIFEST_READY", discovered_files=len(all_files), logged_files=len(manifest), limited=(total < len(all_files)))
        return manifest

    def discover_inputs(self) -> List[Path]:
        self.plan.update("discovery", 30.0)
        if not self.test_dir.exists():
            raise FileNotFoundError(f"Missing test_images directory: {self.test_dir}")
        files = sorted(self.test_dir.rglob("*.tif"))
        if not files:
            raise RuntimeError(f"No TIFF files found in {self.test_dir}")
        self.plan.update("discovery", 100.0, done=True)
        self.log("INPUT_DISCOVERED", file_count=len(files), using_gpu=self.using_gpu)
        return files

    def read_volume(self, path: Path) -> np.ndarray:
        self.plan.update("load", 25.0)
        if ensure_imagecodecs():
            vol = tifffile.imread(path).astype(np.float32)
        elif PIL_Image is not None and PIL_ImageSequence is not None:
            with PIL_Image.open(path) as img:
                frames = [np.array(frame, dtype=np.float32) for frame in PIL_ImageSequence.Iterator(img)]
            vol = np.stack(frames, axis=0)
        else:
            vol = tifffile.imread(path).astype(np.float32)

        if vol.ndim != 3:
            raise RuntimeError(f"Unsupported TIFF shape for {path.name}: {vol.shape}")

        vmin = float(vol.min())
        vmax = float(vol.max())
        vol = (vol - vmin) / (vmax - vmin + 1e-6)
        self.plan.update("load", 100.0, done=True)
        self.mem.log_array("volume_normalized", vol)
        return vol

    def extract_features(self, vol: np.ndarray) -> Dict[str, float]:
        self.plan.update("features", 25.0)
        gx = sobel(vol, axis=2)
        gy = sobel(vol, axis=1)
        gz = sobel(vol, axis=0)
        gradient_energy = float(np.mean(np.sqrt(gx * gx + gy * gy + gz * gz)))
        intensity_std = float(np.std(vol))
        intensity_mean = float(np.mean(vol))
        entropy_hist, _ = np.histogram(vol, bins=64, range=(0.0, 1.0), density=False)
        entropy_p = entropy_hist / max(float(entropy_hist.sum()), 1.0)
        entropy = -float(np.sum(entropy_p * np.log(entropy_p + 1e-12)))

        # competitor-inspired stable thresholds from provided notebook
        t_low = float(np.clip(float(os.environ.get("V114_T_LOW", "0.50")), 0.0, 1.0))
        t_high = float(np.clip(float(os.environ.get("V114_T_HIGH", "0.90")), 0.0, 1.0))

        # sniper analogy: distance and wind correction terms
        z_axis = np.linspace(-1.0, 1.0, vol.shape[0], dtype=np.float32)
        distance_term = float(np.mean(np.abs(z_axis)))
        wind_field = np.diff(vol, axis=2, prepend=vol[:, :, :1])
        wind_term = float(np.mean(np.abs(wind_field)))

        fusion_score = 0.42 * math.tanh(gradient_energy * 2.0) + 0.20 * math.tanh(intensity_std * 8.0) + 0.18 * math.tanh(entropy / math.log(64.0))
        fusion_score += 0.10 * math.tanh(distance_term * 2.0) + 0.10 * math.tanh(wind_term * 8.0)
        fusion_score = float(np.clip(0.5 + 0.5 * fusion_score, 0.05, 0.95))

        self.plan.update("features", 100.0, done=True)
        return {
            "gradient_energy": gradient_energy,
            "intensity_std": intensity_std,
            "intensity_mean": intensity_mean,
            "entropy": entropy,
            "distance_term": distance_term,
            "wind_term": wind_term,
            "fusion_score": fusion_score,
            "T_low": t_low,
            "T_high": t_high,
        }

    def _save_slice_trace(self, file_stem: str, z: int, proj: np.ndarray, p_lo: float, p_hi: float) -> None:
        q = np.clip(np.asarray(proj) * 65535.0, 0, 65535).astype(np.uint16)
        trace_path = self.trace_dir / f"{file_stem}_z{z:04d}.npz"
        np.savez_compressed(trace_path, proj_uint16=q, p_lo=np.float32(p_lo), p_hi=np.float32(p_hi))
        self.log("SLICE_PIXEL_TRACE", z=z, trace_file=trace_path.name, bits="uint16", pixel_count=int(q.size), packed_sha256=sha256(q.tobytes()).hexdigest())

    def segment_volume(self, vol: np.ndarray, features: Dict[str, float], file_stem: str) -> np.ndarray:
        self.plan.update("segment", 10.0)
        xp = self._xp()
        vol_xp = xp.asarray(vol)
        sigma = float(max(0.5, np.std(vol) * 1.1 + 0.35))
        smooth = self._gaussian(vol_xp, sigma=sigma)
        resid = vol_xp - smooth

        t_low = float(features["T_low"])
        t_high = float(features["T_high"])

        masks: List[np.ndarray] = []
        z_count = vol.shape[0]
        for z in range(z_count):
            if z % max(1, z_count // 20) == 0:
                self.plan.update("segment", 10.0 + 85.0 * (z / max(1, z_count - 1)))

            z0, z1 = max(0, z - 1), min(z_count, z + 2)
            proj = xp.mean(resid[z0:z1], axis=0)
            proj_cpu = cp.asnumpy(proj) if self.using_gpu else np.asarray(proj)

            # distance/wind adaptive correction for slice
            distance = abs((z / max(1, z_count - 1)) - 0.5)
            wind = float(np.mean(np.abs(np.diff(vol[z], axis=1, prepend=vol[z, :, :1]))))
            drift = 0.04 * distance + 0.06 * math.tanh(wind * 8.0)
            adj_low = np.clip(t_low - drift, 0.05, 0.95)
            adj_high = np.clip(t_high - drift * 0.5, adj_low + 0.01, 0.99)

            p_lo = float(np.percentile(proj_cpu, 100.0 * adj_low))
            p_hi = float(np.percentile(proj_cpu, 100.0 * adj_high))

            strong = proj_cpu >= p_hi
            weak = proj_cpu >= p_lo
            mask = np.where(strong | weak, 255, 0).astype(np.uint8)
            masks.append(mask)

            self.log(
                "SLICE_METRIC",
                z=z,
                fusion_score=round(float(features["fusion_score"]), 6),
                distance=round(distance, 6),
                wind=round(wind, 6),
                drift=round(float(drift), 6),
                p_lo=round(p_lo, 6),
                p_hi=round(p_hi, 6),
            )
            self.mem.log_array("slice_mask", mask, z=z)

            if self.per_pixel_trace:
                self._save_slice_trace(file_stem, z, proj_cpu, p_lo, p_hi)

        self.plan.update("segment", 100.0, done=True)
        return np.stack(masks, axis=0)

    def _build_submission_mask(self, mask_stack: np.ndarray, file_name: str) -> np.ndarray:
        recon = np.mean(mask_stack.astype(np.float32) / 255.0, axis=0)
        target_active_ratio = float(np.clip(float(os.environ.get("V114_TARGET_ACTIVE_RATIO", "0.20")), 0.05, 0.35))
        candidates = np.linspace(float(np.percentile(recon, 40)), float(np.percentile(recon, 97)), 96, dtype=np.float32)
        ratios = np.array([float((recon >= th).mean()) for th in candidates], dtype=np.float32)
        idx = int(np.argmin(np.abs(ratios - target_active_ratio)))
        threshold = float(candidates[idx])
        out = (recon >= threshold).astype(np.uint8)

        self.log(
            "SUBMISSION_MASK_STATS",
            version=self.version,
            file=file_name,
            threshold=round(threshold, 6),
            target_active_ratio=target_active_ratio,
            active_ratio=round(float(out.mean()), 6),
            recon_mean=round(float(recon.mean()), 6),
            recon_std=round(float(recon.std()), 6),
        )
        return out

    def _save_overlay(self, file_stem: str, vol: np.ndarray, mask_stack: np.ndarray) -> None:
        self.plan.update("overlay", 20.0)
        if not importlib.util.find_spec("matplotlib"):
            self.log("OVERLAY_SKIPPED", reason="matplotlib unavailable")
            self.plan.update("overlay", 100.0, done=True)
            return
        plt = importlib.import_module("matplotlib.pyplot")

        z_count = vol.shape[0]
        for i, z in enumerate(range(0, z_count, self.overlay_stride), start=1):
            fig, ax = plt.subplots(1, 1, figsize=(8, 8))
            ax.imshow(vol[z], cmap="gray")
            m = (mask_stack[z] > 0).astype(np.uint8)
            ax.imshow(np.ma.masked_where(m == 0, m), cmap="autumn", alpha=0.45)
            ax.set_title(f"{file_stem} | slice {z}")
            ax.axis("off")
            fig.savefig(self.overlay_dir / f"{file_stem}_slice_{z:04d}_overlay.png", dpi=120, bbox_inches="tight")
            plt.close(fig)
            self.plan.update("overlay", 20.0 + 70.0 * (i / max(1, len(range(0, z_count, self.overlay_stride)))))

        self.plan.update("overlay", 100.0, done=True)

    def run(self) -> Path:
        manifest = self.build_dataset_manifest()
        files = self.discover_inputs()

        self.plan.update("package", 10.0)
        with zipfile.ZipFile(self.submission_path, "w", compression=zipfile.ZIP_STORED) as zf:
            for i, fpath in enumerate(files, start=1):
                self.log("FILE_START", file=fpath.name, index=i, total=len(files))
                self.plan.update("load", 0.0, done=False)
                self.plan.update("features", 0.0, done=False)
                self.plan.update("segment", 0.0, done=False)
                self.plan.update("overlay", 0.0, done=False)

                vol = self.read_volume(fpath)
                features = self.extract_features(vol)
                self.log("FEATURES", file=fpath.name, **{k: round(float(v), 6) if isinstance(v, (float, int)) else v for k, v in features.items()})
                mask_stack = self.segment_volume(vol, features, fpath.stem)
                self._save_overlay(fpath.stem, vol, mask_stack)

                submission_mask = self._build_submission_mask(mask_stack, fpath.name)
                out_mask = self.tmp_dir / fpath.name
                tifffile.imwrite(out_mask, submission_mask[np.newaxis, ...], compression="LZW")
                zf.write(out_mask, arcname=fpath.name)
                out_mask.unlink(missing_ok=True)
                gc.collect()

                self.log("FILE_DONE", file=fpath.name, slices=int(mask_stack.shape[0]))
                self.plan.update("package", 10.0 + 85.0 * (i / len(files)))

        metadata = {
            "version": self.version,
            "submission_zip": str(self.submission_path),
            "input_dir": str(self.test_dir),
            "output_masks_format": "LZW TIFF binary {0,1}",
            "overlay_dir": str(self.overlay_dir),
            "trace_dir": str(self.trace_dir),
            "log_count": len(self.logs),
            "gpu": self.using_gpu,
            "dataset_manifest_file_count": len(manifest),
            "per_pixel_trace": self.per_pixel_trace,
        }
        (self.output_dir / "v114_execution_metadata.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")
        (self.output_dir / "v114_execution_logs.json").write_text(json.dumps(self.logs, indent=2), encoding="utf-8")
        (self.output_dir / "v114_dataset_manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
        self.mem.flush("v114_memory_tracker.json")
        self.plan.update("package", 100.0, done=True)
        self.log("EXEC_COMPLETE", submission=str(self.submission_path))
        return self.submission_path


NX47V113Kernel = NX47V114Kernel
NX47V112Kernel = NX47V114Kernel
NX47V111Kernel = NX47V114Kernel
NX47V110Kernel = NX47V114Kernel
NX47V109Kernel = NX47V114Kernel
NX47V108Kernel = NX47V114Kernel
NX47V107Kernel = NX47V114Kernel
NX47V106Kernel = NX47V114Kernel
NX47V96Kernel = NX47V114Kernel


if __name__ == "__main__":
    kernel = NX47V114Kernel(
        root=Path(os.environ.get("VESUVIUS_ROOT", "/kaggle/input/competitions/vesuvius-challenge-surface-detection")),
        output_dir=Path(os.environ.get("VESUVIUS_OUTPUT", "/kaggle/working")),
        overlay_stride=int(os.environ.get("V114_OVERLAY_STRIDE", os.environ.get("V113_OVERLAY_STRIDE", "8"))),
        max_dataset_manifest_files=int(os.environ.get("V114_DATASET_MANIFEST_MAX", "50000")),
        per_pixel_trace=os.environ.get("V114_PER_PIXEL_TRACE", "0") == "1",
    )
    submission = kernel.run()
    print(f"READY: {submission}")
