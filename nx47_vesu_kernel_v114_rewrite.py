from __future__ import annotations

import gc
import importlib
import json
import math
import os
import subprocess
import sys
import time
import zipfile
from dataclasses import asdict, dataclass, field
from hashlib import sha512
from pathlib import Path
from typing import Any, Dict, List

import numpy as np

try:
    import cupy as cp
    from cupyx.scipy.ndimage import gaussian_filter as gpu_gaussian_filter
except Exception:  # pragma: no cover
    cp = None
    gpu_gaussian_filter = None

from scipy.ndimage import (
    binary_closing,
    binary_propagation,
    gaussian_filter as cpu_gaussian_filter,
    generate_binary_structure,
    label,
    sobel,
)

import tifffile

try:
    from PIL import Image, ImageSequence
except Exception:  # pragma: no cover
    Image = None
    ImageSequence = None


@dataclass
class V114Config:
    t_low: float = 0.50
    t_high: float = 0.90
    z_radius: int = 3
    xy_radius: int = 2
    dust_min_size: int = 100
    max_layers: int = 320
    full_pixel_trace: bool = False
    trace_pixel_budget: int = 5000
    overlay_stride: int = 8
    target_active_ratio: float = 0.20


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


class MemoryTracker:
    def __init__(self) -> None:
        self.events: List[Dict[str, Any]] = []

    @staticmethod
    def _array_fingerprint(arr: np.ndarray) -> Dict[str, Any]:
        arr = np.asarray(arr)
        return {
            "shape": list(arr.shape),
            "dtype": str(arr.dtype),
            "bytes": int(arr.nbytes),
            "min": float(arr.min()) if arr.size else 0.0,
            "max": float(arr.max()) if arr.size else 0.0,
            "mean": float(arr.mean()) if arr.size else 0.0,
            "sha512": sha512(arr.tobytes()).hexdigest(),
        }

    def log_array(self, stage: str, arr: np.ndarray) -> None:
        self.events.append({"ts_ns": time.time_ns(), "stage": stage, "array": self._array_fingerprint(arr)})


def install_offline(package_name: str) -> None:
    exact_wheel_dir = Path("/kaggle/input/datasets/ndarray2000/nx47-dependencies")
    fallback_wheel_dir = Path("/kaggle/input/nx47-dependencies")

    exact_wheels = {
        "imagecodecs": exact_wheel_dir / "imagecodecs-2026.1.14-cp311-abi3-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl",
        "numpy": exact_wheel_dir / "numpy-2.4.2-cp311-cp311-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl",
        "tifffile": exact_wheel_dir / "tifffile-2026.1.28-py3-none-any.whl",
    }

    if package_name == "numpy":
        try:
            import numpy as _np  # noqa: F401
            return
        except Exception:
            pass

    if package_name in exact_wheels and exact_wheels[package_name].exists():
        try:
            subprocess.check_call([sys.executable, "-m", "pip", "install", "--no-index", str(exact_wheels[package_name])])
            return
        except subprocess.CalledProcessError:
            pass

    for wheel_dir in (exact_wheel_dir, fallback_wheel_dir):
        if wheel_dir.exists():
            subprocess.check_call([sys.executable, "-m", "pip", "install", "--no-index", f"--find-links={wheel_dir}", package_name])
            return

    raise RuntimeError(f"Offline dependency directory not found for {package_name}")


def bootstrap_dependencies_fail_fast() -> None:
    install_offline("numpy")
    install_offline("imagecodecs")
    install_offline("tifffile")
    global tifffile
    tifffile = importlib.reload(tifffile)


def ensure_imagecodecs() -> bool:
    try:
        import imagecodecs  # noqa: F401
        return True
    except Exception:
        pass

    try:
        install_offline("imagecodecs")
        import imagecodecs  # noqa: F401
        global tifffile
        tifffile = importlib.reload(tifffile)
        return True
    except Exception:
        return False


def read_tiff_lzw_safe(path: Path) -> np.ndarray:
    try:
        return tifffile.imread(path)
    except ValueError as exc:
        if "requires the 'imagecodecs' package" not in str(exc):
            raise

    ensure_imagecodecs()
    try:
        return tifffile.imread(path)
    except ValueError as exc:
        if "requires the 'imagecodecs' package" not in str(exc):
            raise

    if Image is None or ImageSequence is None:
        raise RuntimeError("LZW TIFF read failed and Pillow fallback unavailable")

    with Image.open(path) as img:
        frames = [np.array(frame, dtype=np.float32) for frame in ImageSequence.Iterator(img)]
    if not frames:
        raise RuntimeError(f"No frames decoded from TIFF: {path}")
    return np.stack(frames, axis=0)


def write_tiff_lzw_safe(path: Path, arr: np.ndarray) -> None:
    arr = np.asarray(arr)
    if arr.ndim == 2:
        arr = arr[np.newaxis, ...]
    if arr.ndim != 3:
        raise RuntimeError(f"Unsupported TIFF array shape for write: {arr.shape}")

    try:
        if ensure_imagecodecs():
            tifffile.imwrite(path, arr, compression="LZW")
            return
    except Exception:
        pass

    if Image is None:
        raise RuntimeError("LZW TIFF write failed: Pillow fallback unavailable")

    pages = [Image.fromarray(frame.astype(np.uint8)) for frame in arr]
    if not pages:
        raise RuntimeError("Cannot write empty TIFF volume")
    pages[0].save(path, save_all=True, append_images=pages[1:], compression="tiff_lzw")


class NX47V114RebuildKernel:
    def __init__(
        self,
        root: Path = Path("/kaggle/input/competitions/vesuvius-challenge-surface-detection"),
        output_dir: Path = Path("/kaggle/working"),
        config: V114Config | None = None,
    ) -> None:
        self.version = "NX47 V114 REWRITE"
        self.root = self._resolve_root(root)
        self.test_dir = self.root / "test_images"
        self.output_dir = output_dir
        self.tmp_dir = output_dir / "tmp_masks"
        self.overlay_dir = output_dir / "overlays"
        self.submission_path = output_dir / "submission.zip"
        self.roadmap_path = output_dir / "v114_rewrite_roadmap_realtime.json"
        self.logs_path = output_dir / "v114_rewrite_execution_logs.json"
        self.memory_path = output_dir / "v114_rewrite_memory_tracker.json"
        self.metadata_path = output_dir / "v114_rewrite_execution_metadata.json"
        self.diff_report_path = output_dir / "v114_rewrite_competitor_diff_report.json"

        self.cfg = config or V114Config()
        self.plan = PlanTracker(output_path=self.roadmap_path)
        self.memory_tracker = MemoryTracker()
        self.logs: List[Dict[str, Any]] = []

        bootstrap_dependencies_fail_fast()
        if not ensure_imagecodecs():
            raise RuntimeError("imagecodecs is mandatory at startup for LZW TIFF I/O")

        self.tmp_dir.mkdir(parents=True, exist_ok=True)
        self.overlay_dir.mkdir(parents=True, exist_ok=True)

        self.plan.add_step("discovery", "Validation des dossiers/format de competition")
        self.plan.add_step("load", "Chargement volume TIFF + normalisation")
        self.plan.add_step("features", "Extraction features + fusion")
        self.plan.add_step("segment", "Segmentation hysteresis + topologie")
        self.plan.add_step("overlay", "Generation overlays")
        self.plan.add_step("package", "Generation submission.zip")
        self.plan._write()

        self.log("BOOT", version=self.version, root=str(self.root), config=asdict(self.cfg))

    def _resolve_root(self, preferred: Path) -> Path:
        candidates = [
            preferred,
            Path("/kaggle/input/competitions/vesuvius-challenge-surface-detection"),
            Path("/kaggle/input/vesuvius-challenge-surface-detection"),
        ]
        for c in candidates:
            if c.exists():
                return c

        if preferred.parent.exists():
            matches = [
                p
                for p in preferred.parent.iterdir()
                if p.is_dir() and "vesuvius" in p.name.lower() and "surface" in p.name.lower()
            ]
            if matches:
                return matches[0]
        raise FileNotFoundError(f"Dataset path missing. Tried: {[str(c) for c in candidates]}")

    def log(self, event: str, **kwargs: Any) -> None:
        payload = {"ts_ns": time.time_ns(), "event": event, **kwargs}
        payload["signature"] = sha512(json.dumps(payload, sort_keys=True, default=str).encode()).hexdigest()
        self.logs.append(payload)
        print(json.dumps(payload, ensure_ascii=False))

    @property
    def using_gpu(self) -> bool:
        return cp is not None

    def _xp(self):
        return cp if self.using_gpu else np

    def _gaussian(self, arr, sigma: float):
        if self.using_gpu:
            return gpu_gaussian_filter(arr, sigma=sigma)
        return cpu_gaussian_filter(arr, sigma=sigma)

    def discover_dataset_assets(self) -> Dict[str, Any]:
        all_files = [p for p in self.root.rglob("*") if p.is_file()]
        suffix_stats: Dict[str, int] = {}
        for p in all_files:
            suffix = p.suffix.lower() or "<noext>"
            suffix_stats[suffix] = suffix_stats.get(suffix, 0) + 1
        payload = {
            "root": str(self.root),
            "total_files": len(all_files),
            "suffix_stats": suffix_stats,
            "csv_files": [str(p) for p in all_files if p.suffix.lower() == ".csv"],
        }
        self.log("DATASET_DISCOVERY", **payload)
        return payload

    def compare_competitor_notebooks(self, folder: Path, baseline: Path | None = None, top_n: int = 100) -> None:
        from tools.vesuvius_notebook_diff import build_key_findings, extract_features, notebook_source, summarize

        files = sorted([p for p in folder.rglob("*") if p.suffix in {".ipynb", ".py"}])
        if top_n > 0:
            files = files[:top_n]

        aggregate: Dict[str, List[float]] = {}
        report_files: Dict[str, Dict[str, float]] = {}
        for p in files:
            feats = extract_features(notebook_source(p))
            report_files[str(p)] = feats
            for k, v in feats.items():
                aggregate.setdefault(k, []).append(v)

        agg = {k: summarize(v) for k, v in aggregate.items() if v}
        baseline_feats = extract_features(notebook_source(baseline)) if baseline and baseline.exists() else None
        report = {
            "file_count": len(files),
            "aggregate": agg,
            "key_findings": build_key_findings(agg, baseline_feats),
            "baseline": baseline_feats,
            "files": report_files,
        }
        self.diff_report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
        self.log("COMPETITOR_NOTEBOOK_ANALYSIS", file_count=len(files), output=str(self.diff_report_path))

    def discover_inputs(self) -> List[Path]:
        self.plan.update("discovery", 25.0)
        if not self.test_dir.exists():
            raise FileNotFoundError(f"Missing test_images directory: {self.test_dir}")
        files = sorted(self.test_dir.rglob("*.tif"))
        if not files:
            raise RuntimeError(f"No TIFF files found in {self.test_dir}")
        self.log("INPUT_DISCOVERED", file_count=len(files), using_gpu=self.using_gpu)
        self.plan.update("discovery", 100.0, done=True)
        return files

    def read_volume(self, path: Path) -> np.ndarray:
        self.plan.update("load", 25.0)
        vol = read_tiff_lzw_safe(path).astype(np.float32)
        if vol.ndim != 3:
            raise RuntimeError(f"Unsupported TIFF shape for {path.name}: {vol.shape}")
        vmin, vmax = float(vol.min()), float(vol.max())
        vol = (vol - vmin) / (vmax - vmin + 1e-6)
        self.plan.update("load", 100.0, done=True)
        self.memory_tracker.log_array("volume", vol)
        return vol

    def extract_features(self, vol: np.ndarray) -> Dict[str, float]:
        self.plan.update("features", 20.0)
        gx, gy, gz = sobel(vol, axis=2), sobel(vol, axis=1), sobel(vol, axis=0)
        gradient_energy = float(np.mean(np.sqrt(gx * gx + gy * gy + gz * gz)))
        intensity_std = float(np.std(vol))
        intensity_mean = float(np.mean(vol))
        p95 = float(np.percentile(vol, 95))
        p50 = float(np.percentile(vol, 50))

        hist_counts, _ = np.histogram(vol, bins=64, range=(0.0, 1.0), density=False)
        hist_probs = hist_counts.astype(np.float64)
        hist_probs /= max(float(hist_probs.sum()), 1.0)
        entropy = -float(np.sum(hist_probs * np.log(hist_probs + 1e-12)))

        nx47_signal = 0.35 * math.tanh(gradient_energy / 2.0) + 0.30 * math.tanh(intensity_std * 8.0)
        nx47_signal += 0.20 * math.tanh((p95 - p50) * 6.0) + 0.15 * (entropy / math.log(64.0))
        atom_signal = 0.5 * (1.0 + math.tanh((intensity_mean - 0.35) * 6.0)) + 0.5 * (
            1.0 + math.tanh((intensity_std - 0.10) * 10.0)
        )
        fusion_score = float(np.clip(0.5 + 0.5 * math.tanh((0.7 * nx47_signal + 0.3 * (atom_signal - 0.5)) * 1.2), 0.05, 0.95))

        self.plan.update("features", 100.0, done=True)
        return {
            "gradient_energy": gradient_energy,
            "intensity_std": intensity_std,
            "intensity_mean": intensity_mean,
            "entropy": entropy,
            "nx47_signal": nx47_signal,
            "atom_signal": atom_signal,
            "fusion_score": fusion_score,
        }

    @staticmethod
    def _build_struct(z_radius: int, xy_radius: int) -> np.ndarray:
        z, r = int(z_radius), int(xy_radius)
        struct = np.zeros((2 * z + 1, 2 * r + 1, 2 * r + 1), dtype=bool)
        for dz in range(-z, z + 1):
            for dy in range(-r, r + 1):
                for dx in range(-r, r + 1):
                    if dy * dy + dx * dx <= r * r:
                        struct[dz + z, dy + r, dx + r] = True
        return struct

    def _remove_small_components(self, mask: np.ndarray, min_size: int) -> np.ndarray:
        labeled, num = label(mask)
        if num == 0:
            return mask
        counts = np.bincount(labeled.ravel())
        keep = counts >= int(min_size)
        keep[0] = False
        return keep[labeled]

    def segment_volume(self, vol: np.ndarray, fusion_score: float) -> np.ndarray:
        self.plan.update("segment", 10.0)
        xp = self._xp()
        vol_xp = xp.asarray(vol)

        sigma = float(max(0.5, np.std(vol) * 1.1 + 0.35))
        smooth = 0.65 * self._gaussian(vol_xp, sigma=sigma) + 0.35 * self._gaussian(vol_xp, sigma=sigma * 1.8)
        resid = vol_xp - smooth

        mean_sig = xp.asarray(xp.mean(resid, axis=0))
        std_sig = xp.asarray(xp.std(resid, axis=0))
        private_prob = xp.clip(0.70 * mean_sig + 0.30 * std_sig + fusion_score * 0.05, 0.0, 1.0)

        mean_cpu = cp.asnumpy(mean_sig) if self.using_gpu else np.asarray(mean_sig)
        prob_cpu = cp.asnumpy(private_prob) if self.using_gpu else np.asarray(private_prob)

        public_anchor = mean_cpu >= np.percentile(mean_cpu, 80)
        strong = prob_cpu >= self.cfg.t_high
        weak = (prob_cpu >= self.cfg.t_low) | public_anchor

        if strong.any():
            base2d = binary_propagation(strong, mask=weak, structure=generate_binary_structure(2, 2))
        else:
            base2d = np.zeros_like(strong, dtype=bool)

        mask3d = np.repeat(base2d[np.newaxis, ...], vol.shape[0], axis=0)
        closed3d = binary_closing(mask3d, structure=self._build_struct(self.cfg.z_radius, self.cfg.xy_radius))
        out2d = closed3d.any(axis=0)
        out2d = self._remove_small_components(out2d, self.cfg.dust_min_size)

        if self.cfg.full_pixel_trace:
            ys, xs = np.where(out2d)
            budget = min(len(ys), self.cfg.trace_pixel_budget)
            traces = []
            for i in range(budget):
                y, x = int(ys[i]), int(xs[i])
                traces.append(
                    {
                        "y": y,
                        "x": x,
                        "private_prob": float(prob_cpu[y, x]),
                        "mean_sig": float(mean_cpu[y, x]),
                        "bit": 1,
                        "ts_ns": time.time_ns(),
                    }
                )
            self.log("PIXEL_TRACE", count=len(traces), sample=traces[:20])

        self.memory_tracker.log_array("private_prob", prob_cpu)
        self.memory_tracker.log_array("final_mask", out2d.astype(np.uint8))
        self.plan.update("segment", 100.0, done=True)
        return np.repeat(out2d[np.newaxis, ...].astype(np.uint8) * 255, vol.shape[0], axis=0)

    def _build_submission_mask(self, mask_stack: np.ndarray, file_name: str) -> np.ndarray:
        reconstruction = np.mean(mask_stack.astype(np.float32) / 255.0, axis=0)
        low = float(np.percentile(reconstruction, 40))
        high = float(np.percentile(reconstruction, 95))

        if high <= low + 1e-9:
            threshold = float(np.percentile(reconstruction, 80))
            threshold_source = "fallback_percentile80"
        else:
            target_ratio = float(np.clip(self.cfg.target_active_ratio, 0.05, 0.35))
            candidates = np.linspace(low, high, 64, dtype=np.float32)
            ratios = np.array([float((reconstruction >= th).mean()) for th in candidates], dtype=np.float32)
            idx = int(np.argmin(np.abs(ratios - target_ratio)))
            threshold = float(candidates[idx])
            threshold_source = f"target_active_ratio_{target_ratio:.3f}"

        binary_mask = (reconstruction >= threshold).astype(np.uint8)
        self.log(
            "SUBMISSION_MASK_STATS",
            version="v114_rewrite",
            file=file_name,
            threshold=round(threshold, 6),
            threshold_source=threshold_source,
            active_ratio=round(float(binary_mask.mean()), 6),
        )
        return binary_mask

    def _save_overlay(self, file_stem: str, vol: np.ndarray, mask: np.ndarray) -> None:
        self.plan.update("overlay", 20.0)
        try:
            import matplotlib.pyplot as plt
        except Exception as exc:
            self.log("OVERLAY_SKIPPED", reason=f"matplotlib unavailable: {exc}")
            self.plan.update("overlay", 100.0, done=True)
            return

        z_values = list(range(0, vol.shape[0], max(1, self.cfg.overlay_stride)))
        for idx, z in enumerate(z_values, start=1):
            fig, ax = plt.subplots(1, 1, figsize=(8, 8))
            ax.imshow(vol[z], cmap="gray")
            m = (mask[z] > 0).astype(np.uint8)
            ax.imshow(np.ma.masked_where(m == 0, m), cmap="autumn", alpha=0.45)
            ax.axis("off")
            fig.savefig(self.overlay_dir / f"{file_stem}_slice_{z:04d}_overlay.png", dpi=120, bbox_inches="tight")
            plt.close(fig)
            self.plan.update("overlay", 20.0 + 70.0 * (idx / max(1, len(z_values))))

        self.plan.update("overlay", 100.0, done=True)

    def run(self) -> Path:
        self.discover_dataset_assets()

        competitor_dir = Path(os.environ.get("V114_COMPETITOR_NOTEBOOKS", "competitor_notebooks"))
        if competitor_dir.exists():
            self.compare_competitor_notebooks(competitor_dir, baseline=Path(__file__), top_n=100)
        else:
            self.log("COMPETITOR_NOTEBOOK_ANALYSIS_SKIPPED", reason=f"missing folder: {competitor_dir}")

        files = self.discover_inputs()
        self.plan.update("package", 10.0)

        with zipfile.ZipFile(self.submission_path, "w", zipfile.ZIP_STORED) as zf:
            for i, fpath in enumerate(files, start=1):
                self.log("FILE_START", file=fpath.name, index=i, total=len(files))
                self.plan.update("load", 0.0, done=False)
                self.plan.update("features", 0.0, done=False)
                self.plan.update("segment", 0.0, done=False)
                self.plan.update("overlay", 0.0, done=False)

                vol = self.read_volume(fpath)
                features = self.extract_features(vol)
                self.log("FEATURES", file=fpath.name, **{k: round(v, 6) for k, v in features.items()})

                mask_stack = self.segment_volume(vol, fusion_score=features["fusion_score"])
                self._save_overlay(fpath.stem, vol, mask_stack)

                submission_mask = self._build_submission_mask(mask_stack, fpath.name)
                out_mask = self.tmp_dir / fpath.name
                write_tiff_lzw_safe(out_mask, submission_mask[np.newaxis, ...])
                zf.write(out_mask, arcname=fpath.name)
                out_mask.unlink(missing_ok=True)
                gc.collect()

                self.log("FILE_DONE", file=fpath.name, slices=int(mask_stack.shape[0]))
                self.plan.update("package", 10.0 + 85.0 * (i / len(files)))

        metadata = {
            "version": self.version,
            "root": str(self.root),
            "input_dir": str(self.test_dir),
            "submission_zip": str(self.submission_path),
            "overlay_dir": str(self.overlay_dir),
            "log_count": len(self.logs),
            "gpu": self.using_gpu,
            "config": asdict(self.cfg),
        }
        self.metadata_path.write_text(json.dumps(metadata, indent=2), encoding="utf-8")
        self.logs_path.write_text(json.dumps(self.logs, indent=2), encoding="utf-8")
        self.memory_path.write_text(json.dumps(self.memory_tracker.events, indent=2), encoding="utf-8")

        self.plan.update("package", 100.0, done=True)
        self.log("EXEC_COMPLETE", submission=str(self.submission_path))
        return self.submission_path


if __name__ == "__main__":
    cfg = V114Config(
        full_pixel_trace=os.environ.get("V114_FULL_PIXEL_TRACE", "0") == "1",
        trace_pixel_budget=int(os.environ.get("V114_TRACE_PIXEL_BUDGET", "5000")),
        overlay_stride=int(os.environ.get("V114_OVERLAY_STRIDE", "8")),
        target_active_ratio=float(os.environ.get("V114_TARGET_ACTIVE_RATIO", "0.20")),
    )
    kernel = NX47V114RebuildKernel(
        root=Path(os.environ.get("VESUVIUS_ROOT", "/kaggle/input/competitions/vesuvius-challenge-surface-detection")),
        output_dir=Path(os.environ.get("VESUVIUS_OUTPUT", "/kaggle/working")),
        config=cfg,
    )
    submission = kernel.run()
    print(f"READY: {submission}")
