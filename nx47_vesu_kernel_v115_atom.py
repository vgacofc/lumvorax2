from __future__ import annotations

import gc
import importlib
import json
import os
import subprocess
import sys
import time
import zipfile
from dataclasses import asdict, dataclass, field
from hashlib import sha512
from pathlib import Path
from typing import Any, Dict, List, Tuple

import numpy as np
from scipy.ndimage import (
    binary_closing,
    binary_propagation,
    gaussian_filter,
    generate_binary_structure,
    label,
    laplace,
    sobel,
    uniform_filter,
)

import tifffile

try:
    from PIL import Image, ImageSequence
except Exception:  # pragma: no cover
    Image = None
    ImageSequence = None


@dataclass
class V115Config:
    # Feature + model
    top_k_features: int = 6
    train_max_samples: int = 250_000
    l1_candidates: Tuple[float, ...] = (1e-4, 3e-4, 1e-3, 3e-3, 1e-2)
    l2_candidates: Tuple[float, ...] = (1e-4, 1e-3, 1e-2)
    max_iter: int = 120
    lr: float = 0.08

    # Pseudo-labeling
    pseudo_pos_pct: float = 99.0
    pseudo_neg_pct: float = 50.0

    # Hysteresis + topology
    strong_th: float = 0.85
    weak_th: float = 0.45
    z_radius: int = 3
    xy_radius: int = 2
    dust_min_size: int = 100

    # Calibration
    target_active_ratio: float = 0.03

    # Runtime
    max_layers: int = 320
    overlay_stride: int = 8
    full_pixel_trace: bool = False
    trace_pixel_budget: int = 4000


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

    def log_array(self, stage: str, arr: np.ndarray) -> None:
        arr = np.asarray(arr)
        self.events.append(
            {
                "ts_ns": time.time_ns(),
                "stage": stage,
                "shape": list(arr.shape),
                "dtype": str(arr.dtype),
                "bytes": int(arr.nbytes),
                "min": float(arr.min()) if arr.size else 0.0,
                "max": float(arr.max()) if arr.size else 0.0,
                "mean": float(arr.mean()) if arr.size else 0.0,
                "sha512": sha512(arr.tobytes()).hexdigest(),
            }
        )


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


class NX47AtomNeuron:
    """Autonomous logistic neuron with L1+L2 regularization via proximal gradient."""

    def __init__(self, n_features: int) -> None:
        self.w = np.zeros(n_features, dtype=np.float64)
        self.b = 0.0

    @staticmethod
    def _sigmoid(z: np.ndarray) -> np.ndarray:
        z = np.clip(z, -30, 30)
        return 1.0 / (1.0 + np.exp(-z))

    def predict_proba(self, x: np.ndarray) -> np.ndarray:
        return self._sigmoid(x @ self.w + self.b)

    def fit_prox(
        self,
        x: np.ndarray,
        y: np.ndarray,
        lr: float,
        max_iter: int,
        l1: float,
        l2: float,
    ) -> Dict[str, float]:
        n = max(1, x.shape[0])
        for _ in range(max_iter):
            p = self.predict_proba(x)
            err = p - y
            grad_w = (x.T @ err) / n + l2 * self.w
            grad_b = float(np.mean(err))

            w_temp = self.w - lr * grad_w
            # proximal L1 (soft threshold)
            self.w = np.sign(w_temp) * np.maximum(np.abs(w_temp) - lr * l1, 0.0)
            self.b -= lr * grad_b

        p = self.predict_proba(x)
        eps = 1e-9
        ce = -float(np.mean(y * np.log(p + eps) + (1.0 - y) * np.log(1.0 - p + eps)))
        nz = int(np.sum(np.abs(self.w) > 1e-8))
        return {"cross_entropy": ce, "non_zero_weights": nz}


def _zscore(arr: np.ndarray) -> np.ndarray:
    m = float(arr.mean())
    s = float(arr.std())
    return (arr - m) / (s + 1e-6)


def extract_multi_features(volume: np.ndarray) -> Tuple[np.ndarray, List[str]]:
    proj_mean = np.mean(volume, axis=0)
    proj_max = np.max(volume, axis=0)

    gx = sobel(proj_mean, axis=1)
    gy = sobel(proj_mean, axis=0)
    grad_mag = np.sqrt(gx * gx + gy * gy)

    lap = laplace(proj_mean)

    mu = uniform_filter(proj_mean, size=7)
    mu2 = uniform_filter(proj_mean * proj_mean, size=7)
    local_var = np.maximum(mu2 - mu * mu, 0.0)

    # local entropy approximation from local variance & dynamic range
    local_entropy = np.log1p(local_var * 255.0)

    coherence = 1.0 / (1.0 + np.std(volume, axis=0))

    low = gaussian_filter(proj_mean, sigma=3.0)
    high = proj_mean - gaussian_filter(proj_mean, sigma=1.0)
    bandpass = high + (proj_mean - low)

    feats = [proj_mean, proj_max, grad_mag, lap, local_var, local_entropy, coherence, bandpass]
    names = [
        "proj_mean",
        "proj_max",
        "grad_mag",
        "laplace",
        "local_var",
        "local_entropy",
        "coherence_inter_slice",
        "bandpass_response",
    ]

    feats = [_zscore(f.astype(np.float32)) for f in feats]
    return np.stack(feats, axis=0), names


def auto_select_features(features: np.ndarray, names: List[str], top_k: int) -> Tuple[np.ndarray, List[str], np.ndarray]:
    # variance score
    variances = np.array([float(np.var(features[i])) for i in range(features.shape[0])], dtype=np.float64)
    order = np.argsort(variances)[::-1]

    selected = []
    selected_names = []
    corr_threshold = 0.97

    for idx in order:
        cand = features[idx].ravel()
        keep = True
        for s in selected:
            c = np.corrcoef(cand, s.ravel())[0, 1]
            if np.isfinite(c) and abs(c) >= corr_threshold:
                keep = False
                break
        if keep:
            selected.append(features[idx])
            selected_names.append(names[idx])
        if len(selected) >= top_k:
            break

    if not selected:
        selected = [features[order[0]]]
        selected_names = [names[order[0]]]

    sel = np.stack(selected, axis=0)
    return sel, selected_names, variances


def pseudo_labels(score_map: np.ndarray, pos_pct: float, neg_pct: float) -> Tuple[np.ndarray, np.ndarray]:
    flat = score_map.ravel()
    pos_mask = flat > np.percentile(flat, pos_pct)
    neg_mask = flat < np.percentile(flat, neg_pct)
    keep = pos_mask | neg_mask
    y = np.zeros_like(flat, dtype=np.float64)
    y[pos_mask] = 1.0
    return keep, y


def train_nx47_autonomous(
    features: np.ndarray,
    cfg: V115Config,
    rng: np.random.Generator,
) -> Tuple[NX47AtomNeuron, Dict[str, Any]]:
    x = features.reshape(features.shape[0], -1).T.astype(np.float64)
    score = np.mean(features, axis=0)
    keep, y_all = pseudo_labels(score, cfg.pseudo_pos_pct, cfg.pseudo_neg_pct)

    idx = np.where(keep)[0]
    if idx.size > cfg.train_max_samples:
        idx = rng.choice(idx, size=cfg.train_max_samples, replace=False)

    x_train = x[idx]
    y_train = y_all[idx]

    # split val
    n = x_train.shape[0]
    cut = int(n * 0.85)
    x_tr, y_tr = x_train[:cut], y_train[:cut]
    x_va, y_va = x_train[cut:], y_train[cut:]
    if x_va.shape[0] < 100:
        x_va, y_va = x_tr, y_tr

    best = None
    best_state = None

    for l1 in cfg.l1_candidates:
        for l2 in cfg.l2_candidates:
            m = NX47AtomNeuron(n_features=x.shape[1])
            tr = m.fit_prox(x_tr, y_tr, lr=cfg.lr, max_iter=cfg.max_iter, l1=float(l1), l2=float(l2))
            p = m.predict_proba(x_va)
            eps = 1e-9
            ce = -float(np.mean(y_va * np.log(p + eps) + (1.0 - y_va) * np.log(1.0 - p + eps)))
            sparsity = float(np.mean(np.abs(m.w) < 1e-8))
            objective = ce + 0.02 * (1.0 - sparsity)
            rec = {
                "l1": float(l1),
                "l2": float(l2),
                "val_ce": ce,
                "sparsity": sparsity,
                "objective": objective,
                "train_ce": tr["cross_entropy"],
                "non_zero_weights": tr["non_zero_weights"],
            }
            if best is None or objective < best["objective"]:
                best = rec
                best_state = (m.w.copy(), float(m.b))

    model = NX47AtomNeuron(n_features=x.shape[1])
    if best_state is not None:
        model.w, model.b = best_state
    return model, {"selected_hyperparams": best, "train_samples": int(n), "label_keep_ratio": float(keep.mean())}


def hysteresis_topology_3d(prob: np.ndarray, cfg: V115Config) -> np.ndarray:
    strong = prob >= float(cfg.strong_th)
    weak = prob >= float(cfg.weak_th)
    core = binary_propagation(strong, mask=weak, structure=generate_binary_structure(2, 2)) if strong.any() else np.zeros_like(strong, dtype=bool)

    # 3D close by replicating through z then collapsing
    z, r = int(cfg.z_radius), int(cfg.xy_radius)
    struct = np.zeros((2 * z + 1, 2 * r + 1, 2 * r + 1), dtype=bool)
    for dz in range(-z, z + 1):
        for dy in range(-r, r + 1):
            for dx in range(-r, r + 1):
                if dy * dy + dx * dx <= r * r:
                    struct[dz + z, dy + r, dx + r] = True

    vol = np.repeat(core[np.newaxis, ...], 3, axis=0)
    closed = binary_closing(vol, structure=struct)
    mask = closed.any(axis=0)

    lbl, n = label(mask)
    if n > 0 and cfg.dust_min_size > 1:
        counts = np.bincount(lbl.ravel())
        keep = counts >= int(cfg.dust_min_size)
        keep[0] = False
        mask = keep[lbl]

    return mask


def calibrate_target_ratio(prob: np.ndarray, target_ratio: float) -> np.ndarray:
    ratio = float(np.clip(target_ratio, 0.001, 0.35))
    th = float(np.percentile(prob, 100.0 * (1.0 - ratio)))
    return prob >= th


class NX47V115AtomKernel:
    def __init__(
        self,
        root: Path = Path("/kaggle/input/competitions/vesuvius-challenge-surface-detection"),
        output_dir: Path = Path("/kaggle/working"),
        config: V115Config | None = None,
    ) -> None:
        self.version = "NX47 V115 ATOM"
        self.root = self._resolve_root(root)
        self.test_dir = self.root / "test_images"
        self.output_dir = output_dir
        self.tmp_dir = output_dir / "tmp_masks_v115"
        self.overlay_dir = output_dir / "overlays_v115"
        self.submission_path = output_dir / "submission_v115.zip"
        self.roadmap_path = output_dir / "v115_roadmap_realtime.json"
        self.logs_path = output_dir / "v115_execution_logs.json"
        self.memory_path = output_dir / "v115_memory_tracker.json"
        self.metadata_path = output_dir / "v115_execution_metadata.json"

        self.cfg = config or V115Config()
        self.plan = PlanTracker(self.roadmap_path)
        self.memory = MemoryTracker()
        self.logs: List[Dict[str, Any]] = []

        bootstrap_dependencies_fail_fast()
        if not ensure_imagecodecs():
            raise RuntimeError("imagecodecs is mandatory for LZW TIFF I/O")

        self.tmp_dir.mkdir(parents=True, exist_ok=True)
        self.overlay_dir.mkdir(parents=True, exist_ok=True)

        self.plan.add_step("discovery", "Validation dataset et assets")
        self.plan.add_step("load", "Chargement volume")
        self.plan.add_step("features", "Extraction + sélection features")
        self.plan.add_step("train", "Apprentissage neurone NX-47 L1/L2")
        self.plan.add_step("segment", "Probabilité + hysteresis + calibration")
        self.plan.add_step("package", "Génération submission zip")
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
            hits = [p for p in preferred.parent.iterdir() if p.is_dir() and "vesuvius" in p.name.lower() and "surface" in p.name.lower()]
            if hits:
                return hits[0]
        raise FileNotFoundError(f"Dataset path missing. Tried: {[str(c) for c in candidates]}")

    def log(self, event: str, **kwargs: Any) -> None:
        payload = {"ts_ns": time.time_ns(), "event": event, **kwargs}
        payload["signature"] = sha512(json.dumps(payload, sort_keys=True, default=str).encode()).hexdigest()
        self.logs.append(payload)
        print(json.dumps(payload, ensure_ascii=False))

    def discover_inputs(self) -> List[Path]:
        self.plan.update("discovery", 25.0)
        if not self.test_dir.exists():
            raise FileNotFoundError(f"Missing test_images directory: {self.test_dir}")
        files = sorted(self.test_dir.rglob("*.tif"))
        if not files:
            raise RuntimeError(f"No TIFF files found in {self.test_dir}")

        all_files = [p for p in self.root.rglob("*") if p.is_file()]
        suffix_stats: Dict[str, int] = {}
        for p in all_files:
            s = p.suffix.lower() or "<noext>"
            suffix_stats[s] = suffix_stats.get(s, 0) + 1

        self.log("DATASET_DISCOVERY", file_count=len(files), total_assets=len(all_files), suffix_stats=suffix_stats)
        self.plan.update("discovery", 100.0, done=True)
        return files

    def _load_volume(self, path: Path) -> np.ndarray:
        self.plan.update("load", 25.0)
        vol = read_tiff_lzw_safe(path).astype(np.float32)
        if vol.ndim != 3:
            raise RuntimeError(f"Unsupported TIFF shape for {path.name}: {vol.shape}")
        if vol.shape[0] > self.cfg.max_layers:
            vol = vol[: self.cfg.max_layers]
        vmin, vmax = float(vol.min()), float(vol.max())
        vol = (vol - vmin) / (vmax - vmin + 1e-6)
        self.memory.log_array("volume", vol)
        self.plan.update("load", 100.0, done=True)
        return vol

    def _predict_mask(self, vol: np.ndarray) -> Tuple[np.ndarray, Dict[str, Any]]:
        self.plan.update("features", 10.0)
        features, names = extract_multi_features(vol)
        selected, selected_names, variances = auto_select_features(features, names, self.cfg.top_k_features)
        self.memory.log_array("features_selected", selected)
        self.plan.update("features", 100.0, done=True)

        self.plan.update("train", 15.0)
        rng = np.random.default_rng(47)
        model, train_info = train_nx47_autonomous(selected, self.cfg, rng)
        self.plan.update("train", 100.0, done=True)

        self.plan.update("segment", 20.0)
        x_full = selected.reshape(selected.shape[0], -1).T.astype(np.float64)
        prob = model.predict_proba(x_full).reshape(selected.shape[1:]).astype(np.float32)
        self.memory.log_array("probability_map", prob)

        hysteresis_mask = hysteresis_topology_3d(prob, self.cfg)
        calibrated_mask = calibrate_target_ratio(prob, self.cfg.target_active_ratio)
        final = (hysteresis_mask | calibrated_mask).astype(np.uint8)

        self.memory.log_array("mask_hysteresis", hysteresis_mask.astype(np.uint8))
        self.memory.log_array("mask_calibrated", calibrated_mask.astype(np.uint8))
        self.memory.log_array("mask_final", final)

        if self.cfg.full_pixel_trace:
            ys, xs = np.where(final > 0)
            budget = min(len(ys), self.cfg.trace_pixel_budget)
            sample = []
            for i in range(budget):
                y, x = int(ys[i]), int(xs[i])
                sample.append({"y": y, "x": x, "prob": float(prob[y, x]), "bit": int(final[y, x]), "ts_ns": time.time_ns()})
            self.log("PIXEL_TRACE", count=len(sample), sample=sample[:20])

        self.plan.update("segment", 100.0, done=True)
        metrics = {
            "selected_features": selected_names,
            "feature_variances": {names[i]: float(variances[i]) for i in range(len(names))},
            "train_info": train_info,
            "active_ratio_final": float(final.mean()),
            "active_ratio_hysteresis": float(hysteresis_mask.mean()),
            "active_ratio_calibrated": float(calibrated_mask.mean()),
        }
        return final, metrics

    def run(self) -> Path:
        files = self.discover_inputs()
        self.plan.update("package", 10.0)

        with zipfile.ZipFile(self.submission_path, "w", zipfile.ZIP_STORED) as zf:
            for i, fpath in enumerate(files, start=1):
                self.log("FILE_START", file=fpath.name, index=i, total=len(files))
                for st in ("load", "features", "train", "segment"):
                    self.plan.update(st, 0.0, done=False)

                vol = self._load_volume(fpath)
                mask2d, metrics = self._predict_mask(vol)
                self.log("NX47_METRICS", file=fpath.name, **metrics)

                out_mask = self.tmp_dir / fpath.name
                write_tiff_lzw_safe(out_mask, mask2d[np.newaxis, ...])
                zf.write(out_mask, arcname=fpath.name)
                out_mask.unlink(missing_ok=True)
                gc.collect()

                self.log("FILE_DONE", file=fpath.name, active_ratio=round(float(mask2d.mean()), 6))
                self.plan.update("package", 10.0 + 85.0 * (i / len(files)))

        metadata = {
            "version": self.version,
            "root": str(self.root),
            "input_dir": str(self.test_dir),
            "submission_zip": str(self.submission_path),
            "log_count": len(self.logs),
            "config": asdict(self.cfg),
        }
        self.metadata_path.write_text(json.dumps(metadata, indent=2), encoding="utf-8")
        self.logs_path.write_text(json.dumps(self.logs, indent=2), encoding="utf-8")
        self.memory_path.write_text(json.dumps(self.memory.events, indent=2), encoding="utf-8")

        self.plan.update("package", 100.0, done=True)
        self.log("EXEC_COMPLETE", submission=str(self.submission_path))
        return self.submission_path


if __name__ == "__main__":
    cfg = V115Config(
        top_k_features=int(os.environ.get("V115_TOP_K_FEATURES", "6")),
        target_active_ratio=float(os.environ.get("V115_TARGET_ACTIVE_RATIO", "0.03")),
        full_pixel_trace=os.environ.get("V115_FULL_PIXEL_TRACE", "0") == "1",
        trace_pixel_budget=int(os.environ.get("V115_TRACE_PIXEL_BUDGET", "4000")),
    )
    kernel = NX47V115AtomKernel(
        root=Path(os.environ.get("VESUVIUS_ROOT", "/kaggle/input/competitions/vesuvius-challenge-surface-detection")),
        output_dir=Path(os.environ.get("VESUVIUS_OUTPUT", "/kaggle/working")),
        config=cfg,
    )
    submission = kernel.run()
    print(f"READY: {submission}")
