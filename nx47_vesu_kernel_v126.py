# ================================================================
# 01) NX47 V126 Kernel
# 02) Kaggle Vesuvius pipeline: discovery -> load -> features -> segment -> overlay -> package
# 03) Robust offline dependencies + LZW-safe TIFF I/O + slice-wise adaptive fusion
# ================================================================
from __future__ import annotations

import gc
import importlib
import json
import os
import platform
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
class V126Config:
    top_k_features: int = 6
    train_max_samples: int = 250_000
    l1_candidates: Tuple[float, ...] = (1e-4, 3e-4, 1e-3, 3e-3, 1e-2)
    l2_candidates: Tuple[float, ...] = (1e-4, 1e-3, 1e-2)
    max_iter: int = 120
    lr: float = 0.08

    pseudo_pos_pct: float = 99.0
    pseudo_neg_pct: float = 50.0

    z_radius: int = 3
    xy_radius: int = 2

    target_active_ratio: float = 0.03

    max_layers: int = 320
    overlay_stride: int = 8
    full_pixel_trace: bool = False
    trace_pixel_budget: int = 4000

    ultra_console_log: bool = True
    ultra_step_log: bool = True
    ultra_bit_trace_arrays: bool = True
    ultra_bit_trace_limit: int = 64

    # V125: meta-neuron / evolutionary controls
    meta_neurons: int = 3
    ratio_candidates: Tuple[float, ...] = (0.02, 0.04, 0.06, 0.08, 0.12)
    pruning_quantile: float = 0.25
    mutation_noise: float = 0.015
    f1_stagnation_window: int = 5
    run_simulation_100: bool = False
    simulation_export_curve: bool = True

    # V125 supervised mode
    supervised_train: bool = True
    max_train_volumes: int = 24
    max_val_volumes: int = 8
    max_samples_per_volume: int = 40_000
    pos_neg_ratio: float = 1.0
    strong_th: float = 0.65
    weak_th: float = 0.45
    dust_min_size: int = 24
    golden_nonce_topk: int = 11
    supervised_epochs: int = 3
    threshold_scan: Tuple[float, ...] = (0.35, 0.4, 0.45, 0.5, 0.55, 0.6)
    fbeta_beta: float = 0.5


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


class UltraAuthentic360Merkle:
    def __init__(self, path: Path, console: bool = True) -> None:
        self.path = path
        self.console = console
        self.prev_hash = "0" * 128
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.path.write_text("", encoding="utf-8")

    def bit_stats(self, arr: np.ndarray, bit_limit: int) -> Dict[str, Any]:
        raw = np.asarray(arr).tobytes()
        preview = raw[: max(0, int(bit_limit))]
        ones = int(sum(bin(b).count("1") for b in preview))
        return {
            "byte_preview_len": len(preview),
            "one_bits_in_preview": ones,
            "zero_bits_in_preview": len(preview) * 8 - ones,
            "preview_sha512": sha512(preview).hexdigest(),
        }

    def emit(self, event: Dict[str, Any]) -> None:
        payload = dict(event)
        payload["prev_merkle"] = self.prev_hash
        canonical = json.dumps(payload, sort_keys=True, default=str)
        cur = sha512(canonical.encode()).hexdigest()
        payload["merkle"] = cur
        self.prev_hash = cur
        line = json.dumps(payload, ensure_ascii=False)
        with self.path.open("a", encoding="utf-8") as f:
            f.write(line + "\n")
        if self.console:
            print(line, flush=True)


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
            import numpy as _np  # noqa
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
        import imagecodecs  # noqa
        return True
    except Exception:
        pass
    try:
        install_offline("imagecodecs")
        import imagecodecs  # noqa
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
    def __init__(self, n_features: int) -> None:
        self.w = np.zeros(n_features, dtype=np.float64)
        self.alpha = np.zeros(n_features, dtype=np.float64)
        self.beta = np.zeros(n_features, dtype=np.float64)
        self.b = 0.0

    @staticmethod
    def _sigmoid(z: np.ndarray) -> np.ndarray:
        z = np.clip(z, -30, 30)
        return 1.0 / (1.0 + np.exp(-z))

    def predict_proba(self, x: np.ndarray, grad_x: np.ndarray | None = None) -> np.ndarray:
        gx = grad_x if grad_x is not None else np.gradient(x, axis=0)
        z = x @ self.w + (x * x) @ self.alpha + gx @ self.beta + self.b
        return self._sigmoid(z)

    def fit_prox(self, x: np.ndarray, y: np.ndarray, lr: float, max_iter: int, l1: float, l2: float) -> Dict[str, float]:
        n = max(1, x.shape[0])
        gx = np.gradient(x, axis=0)
        active_start = int(np.sum((np.abs(self.w) + np.abs(self.alpha) + np.abs(self.beta)) > 1e-8))
        active_mid = active_start
        for it in range(max_iter):
            p = self.predict_proba(x, gx)
            err = p - y
            grad_w = (x.T @ err) / n + l2 * self.w
            grad_alpha = ((x * x).T @ err) / n + l2 * self.alpha
            grad_beta = (gx.T @ err) / n + l2 * self.beta
            grad_b = float(np.mean(err))

            w_temp = self.w - lr * grad_w
            a_temp = self.alpha - lr * grad_alpha
            b_temp = self.beta - lr * grad_beta
            self.w = np.sign(w_temp) * np.maximum(np.abs(w_temp) - lr * l1, 0.0)
            self.alpha = np.sign(a_temp) * np.maximum(np.abs(a_temp) - lr * l1, 0.0)
            self.beta = np.sign(b_temp) * np.maximum(np.abs(b_temp) - lr * l1, 0.0)
            self.b -= lr * grad_b
            if it == max_iter // 2:
                active_mid = int(np.sum((np.abs(self.w) + np.abs(self.alpha) + np.abs(self.beta)) > 1e-8))
        p = self.predict_proba(x, gx)
        eps = 1e-9
        ce = -float(np.mean(y * np.log(p + eps) + (1.0 - y) * np.log(1.0 - p + eps)))
        active_end = int(np.sum((np.abs(self.w) + np.abs(self.alpha) + np.abs(self.beta)) > 1e-8))
        return {
            "cross_entropy": ce,
            "non_zero_weights": active_end,
            "active_neurons_start": active_start,
            "active_neurons_mid": active_mid,
            "active_neurons_end": active_end,
        }


@dataclass
class NX47EvolutionMemory:
    f1_history: List[float] = field(default_factory=list)
    ratio_history: List[float] = field(default_factory=list)
    mutation_events: int = 0
    pruning_events: int = 0

    def update(self, f1_proxy: float, ratio: float) -> None:
        self.f1_history.append(float(f1_proxy))
        self.ratio_history.append(float(ratio))

    def adapt_learning_rate(self, base_lr: float, window: int) -> float:
        if len(self.f1_history) < max(2, window):
            return float(base_lr)
        recent = self.f1_history[-window:]
        spread = float(np.max(recent) - np.min(recent))
        return float(base_lr * (0.65 if spread < 1e-3 else 1.0))


def compute_proxy_f1(prob: np.ndarray, target: np.ndarray, threshold: float = 0.5) -> float:
    pred = prob >= threshold
    tp = float(np.logical_and(pred, target > 0.5).sum())
    fp = float(np.logical_and(pred, target <= 0.5).sum())
    fn = float(np.logical_and(~pred, target > 0.5).sum())
    precision = tp / (tp + fp + 1e-9)
    recall = tp / (tp + fn + 1e-9)
    return float(2.0 * precision * recall / (precision + recall + 1e-9))


def choose_adaptive_ratio(prob: np.ndarray, ratios: Tuple[float, ...]) -> Tuple[np.ndarray, Dict[str, Any]]:
    best_ratio = float(ratios[0])
    best_score = -1e18
    best_mask = calibrate_target_ratio(prob, best_ratio)
    details: List[Dict[str, float]] = []
    for r in ratios:
        cand = calibrate_target_ratio(prob, float(r))
        lbl, comp_count = label(cand.astype(np.uint8))
        comp_sizes = np.bincount(lbl.ravel()) if comp_count > 0 else np.array([0])
        coherence = float(cand.mean())
        noise = float((comp_sizes[1:] < 16).sum()) if comp_count > 0 else 0.0
        score = coherence - 0.001 * noise
        details.append({'ratio': float(r), 'score': score, 'coherence': coherence, 'noise_components': noise})
        if score > best_score:
            best_score = score
            best_ratio = float(r)
            best_mask = cand
    return best_mask.astype(bool), {'selected_ratio': best_ratio, 'ratio_scan': details}


def choose_slicewise_adaptive_ratio(volume: np.ndarray, prob: np.ndarray, ratios: Tuple[float, ...]) -> Dict[str, Any]:
    # ratio(x,y,z) proxy: each z slice votes a ratio according to entropy/gradient complexity
    ent = np.log1p(np.maximum(volume.var(axis=(1, 2)), 1e-12))
    gx = np.mean(np.abs(np.gradient(volume, axis=1)), axis=(1, 2))
    gy = np.mean(np.abs(np.gradient(volume, axis=2)), axis=(1, 2))
    complexity = _zscore(ent + gx + gy)
    ratios_arr = np.array(ratios, dtype=np.float64)
    bins = np.linspace(complexity.min() - 1e-9, complexity.max() + 1e-9, len(ratios_arr) + 1)
    assigned = []
    for c in complexity:
        idx = int(np.clip(np.digitize(c, bins) - 1, 0, len(ratios_arr) - 1))
        assigned.append(float(ratios_arr[idx]))
    ratio_global = float(np.mean(assigned))
    ratio_global = float(ratios_arr[np.argmin(np.abs(ratios_arr - ratio_global))])
    mask_global = calibrate_target_ratio(prob, ratio_global)
    return {
        'slice_ratio_profile': assigned,
        'slice_ratio_mean': float(np.mean(assigned)),
        'slice_ratio_std': float(np.std(assigned)),
        'ratio_global_selected': ratio_global,
        'mask_global': mask_global,
    }


def dynamic_regularization_lambda(mask: np.ndarray, features: np.ndarray) -> float:
    var_mask = float(np.var(mask.astype(np.float32)))
    feat_entropy = float(np.mean(np.log1p(np.maximum(np.var(features, axis=(1, 2)), 1e-12))))
    return float(var_mask / (abs(feat_entropy) + 1e-8))


def simulate_f1_vs_ratio_curve() -> Dict[str, Any]:
    ratios = np.linspace(0.01, 0.25, 50)
    precision = np.clip(1.0 - (ratios * 2.0), 0.01, 1.0)
    recall = np.clip(ratios * 4.0, 0.01, 1.0)
    f1_scores = 2.0 * precision * recall / (precision + recall + 1e-8)
    best_idx = int(np.argmax(f1_scores))
    return {
        'ratios': ratios.tolist(),
        'f1_scores': f1_scores.tolist(),
        'best_ratio': float(ratios[best_idx]),
        'best_f1': float(f1_scores[best_idx]),
    }


def _zscore(arr: np.ndarray) -> np.ndarray:
    m, s = float(arr.mean()), float(arr.std())
    return (arr - m) / (s + 1e-6)


def slice_adaptive_fusion(volume: np.ndarray) -> np.ndarray:
    z = volume.shape[0]
    if z <= 1:
        return volume[0]
    w = np.linspace(1.0, 1.4, z, dtype=np.float32)
    w = w / (w.sum() + 1e-6)
    return np.tensordot(w, volume, axes=(0, 0)).astype(np.float32)


def extract_multi_features(volume: np.ndarray) -> Tuple[np.ndarray, List[str]]:
    fused = slice_adaptive_fusion(volume)
    proj_mean = 0.7 * np.mean(volume, axis=0) + 0.3 * fused
    proj_max = np.max(volume, axis=0)
    gx, gy = sobel(proj_mean, axis=1), sobel(proj_mean, axis=0)
    grad_mag = np.sqrt(gx * gx + gy * gy)
    lap = laplace(proj_mean)
    mu = uniform_filter(proj_mean, size=7)
    mu2 = uniform_filter(proj_mean * proj_mean, size=7)
    local_var = np.maximum(mu2 - mu * mu, 0.0)
    local_entropy = np.log1p(local_var * 255.0)
    coherence = 1.0 / (1.0 + np.std(volume, axis=0))
    low = gaussian_filter(proj_mean, sigma=3.0)
    high = proj_mean - gaussian_filter(proj_mean, sigma=1.0)
    bandpass = high + (proj_mean - low)

    feats = [proj_mean, proj_max, grad_mag, lap, local_var, local_entropy, coherence, bandpass]
    names = ["proj_mean", "proj_max", "grad_mag", "laplace", "local_var", "local_entropy", "coherence_inter_slice", "bandpass_response"]
    feats = [_zscore(f.astype(np.float32)) for f in feats]
    return np.stack(feats, axis=0), names


def auto_select_features(features: np.ndarray, names: List[str], top_k: int) -> Tuple[np.ndarray, List[str], np.ndarray]:
    variances = np.array([float(np.var(features[i])) for i in range(features.shape[0])], dtype=np.float64)
    order = np.argsort(variances)[::-1]
    selected, selected_names = [], []
    for idx in order:
        cand = features[idx].ravel()
        keep = True
        for s in selected:
            c = np.corrcoef(cand, s.ravel())[0, 1]
            if np.isfinite(c) and abs(c) >= 0.97:
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
    return np.stack(selected, axis=0), selected_names, variances


def pseudo_labels(score_map: np.ndarray, pos_pct: float, neg_pct: float) -> Tuple[np.ndarray, np.ndarray]:
    flat = score_map.ravel()
    pos = flat > np.percentile(flat, pos_pct)
    neg = flat < np.percentile(flat, neg_pct)
    keep = pos | neg
    y = np.zeros_like(flat, dtype=np.float64)
    y[pos] = 1.0
    return keep, y


def _binary_stats(pred: np.ndarray, y: np.ndarray) -> Dict[str, float]:
    p = pred.astype(bool)
    t = y.astype(bool)
    tp = float(np.logical_and(p, t).sum())
    fp = float(np.logical_and(p, ~t).sum())
    fn = float(np.logical_and(~p, t).sum())
    iou = tp / (tp + fp + fn + 1e-9)
    dice = (2.0 * tp) / (2.0 * tp + fp + fn + 1e-9)
    f1 = dice
    return {'tp': tp, 'fp': fp, 'fn': fn, 'iou': iou, 'dice': dice, 'f1': f1}


def _fbeta_from_stats(stats: Dict[str, float], beta: float) -> float:
    tp, fp, fn = stats['tp'], stats['fp'], stats['fn']
    precision = tp / (tp + fp + 1e-9)
    recall = tp / (tp + fn + 1e-9)
    b2 = beta * beta
    return float((1.0 + b2) * precision * recall / (b2 * precision + recall + 1e-9))


def calibrate_thresholds(y_true: np.ndarray, prob: np.ndarray, thresholds: Tuple[float, ...], beta: float) -> Dict[str, Any]:
    rows: List[Dict[str, float]] = []
    best = None
    for th in thresholds:
        pred = prob >= float(th)
        stats = _binary_stats(pred, y_true > 0.5)
        fbeta = _fbeta_from_stats(stats, beta)
        rec = {'threshold': float(th), 'f1': float(stats['f1']), 'iou': float(stats['iou']), 'dice': float(stats['dice']), 'fbeta': float(fbeta)}
        rows.append(rec)
        if best is None or rec['fbeta'] > best['fbeta']:
            best = rec
    return {'best': best, 'scan': rows}


def _balance_sample_indices(y: np.ndarray, max_samples: int, pos_neg_ratio: float, rng: np.random.Generator) -> np.ndarray:
    pos_idx = np.where(y > 0.5)[0]
    neg_idx = np.where(y <= 0.5)[0]
    if pos_idx.size == 0 or neg_idx.size == 0:
        all_idx = np.arange(y.size)
        if all_idx.size <= max_samples:
            return all_idx
        return rng.choice(all_idx, size=max_samples, replace=False)
    n_pos = min(pos_idx.size, int(max_samples * (pos_neg_ratio / (1.0 + pos_neg_ratio))))
    n_neg = min(neg_idx.size, max_samples - n_pos)
    sel_pos = rng.choice(pos_idx, size=max(1, n_pos), replace=pos_idx.size < max(1, n_pos))
    sel_neg = rng.choice(neg_idx, size=max(1, n_neg), replace=neg_idx.size < max(1, n_neg))
    idx = np.concatenate([sel_pos, sel_neg])
    rng.shuffle(idx)
    return idx


def train_nx47_supervised(
    x_train: np.ndarray,
    y_train: np.ndarray,
    x_val: np.ndarray,
    y_val: np.ndarray,
    cfg: V126Config,
    rng: np.random.Generator,
    memory: NX47EvolutionMemory,
) -> Tuple[NX47AtomNeuron, Dict[str, Any]]:
    best, best_state = None, None
    leaderboard: List[Dict[str, Any]] = []
    epoch_history: List[Dict[str, Any]] = []
    adaptive_lr = memory.adapt_learning_rate(cfg.lr, cfg.f1_stagnation_window)
    grad_x_val = np.gradient(x_val, axis=0)

    for epoch in range(max(1, cfg.supervised_epochs)):
        for neuron_id in range(max(1, cfg.meta_neurons)):
            for l1 in cfg.l1_candidates:
                for l2 in cfg.l2_candidates:
                    m = NX47AtomNeuron(n_features=x_train.shape[1])
                    tr = m.fit_prox(x_train, y_train, lr=adaptive_lr, max_iter=cfg.max_iter, l1=float(l1), l2=float(l2))
                    p = m.predict_proba(x_val, grad_x_val)
                    ce = -float(np.mean(y_val * np.log(p + 1e-9) + (1.0 - y_val) * np.log(1.0 - p + 1e-9)))
                    threshold_info = calibrate_thresholds(y_val, p, cfg.threshold_scan, cfg.fbeta_beta)
                    best_th = float(threshold_info['best']['threshold'])
                    pred = p >= best_th
                    stats = _binary_stats(pred, y_val > 0.5)
                    fbeta = _fbeta_from_stats(stats, cfg.fbeta_beta)
                    sparsity = float(np.mean((np.abs(m.w) + np.abs(m.alpha) + np.abs(m.beta)) < 1e-8))
                    reg_lambda = dynamic_regularization_lambda(pred.astype(np.uint8), x_val.T.reshape(x_train.shape[1], -1, 1))
                    objective = ce + 0.02 * (1.0 - sparsity) - 0.20 * stats['f1'] - 0.15 * fbeta - 0.10 * stats['iou'] + 0.005 * reg_lambda
                    rec = {
                        'epoch': int(epoch),
                        'neuron_id': neuron_id,
                        'l1': float(l1),
                        'l2': float(l2),
                        'val_ce': ce,
                        'val_f1': stats['f1'],
                        'val_iou': stats['iou'],
                        'val_dice': stats['dice'],
                        'val_fbeta': fbeta,
                        'best_threshold': best_th,
                        'threshold_scan': threshold_info['scan'],
                        'sparsity': sparsity,
                        'objective': objective,
                        **tr,
                    }
                    leaderboard.append(rec)
                    if best is None or objective < best['objective']:
                        best = rec
                        best_state = (m.w.copy(), float(m.b), m.alpha.copy(), m.beta.copy())
        # epoch summary
        epoch_best = sorted([r for r in leaderboard if r['epoch'] == epoch], key=lambda d: d['objective'])[0]
        epoch_history.append({
            'epoch': int(epoch),
            'best_objective': float(epoch_best['objective']),
            'best_f1': float(epoch_best['val_f1']),
            'best_iou': float(epoch_best['val_iou']),
            'best_fbeta': float(epoch_best['val_fbeta']),
            'best_threshold': float(epoch_best['best_threshold']),
        })

    mutation_applied = False
    pruning_applied = False
    if best_state is not None:
        if len(memory.f1_history) >= cfg.f1_stagnation_window:
            recent = memory.f1_history[-cfg.f1_stagnation_window:]
            if float(np.max(recent) - np.min(recent)) < 1e-3:
                best_state = (
                    best_state[0] + rng.normal(0.0, cfg.mutation_noise, size=best_state[0].shape),
                    float(best_state[1]),
                    best_state[2] + rng.normal(0.0, cfg.mutation_noise, size=best_state[2].shape),
                    best_state[3] + rng.normal(0.0, cfg.mutation_noise, size=best_state[3].shape),
                )
                mutation_applied = True
                memory.mutation_events += 1
        q = float(np.quantile(np.abs(best_state[0]), cfg.pruning_quantile))
        pruned_w = np.where(np.abs(best_state[0]) < q, 0.0, best_state[0])
        pruned_a = np.where(np.abs(best_state[2]) < q, 0.0, best_state[2])
        pruned_beta = np.where(np.abs(best_state[3]) < q, 0.0, best_state[3])
        if np.any(pruned_w != best_state[0]) or np.any(pruned_a != best_state[2]) or np.any(pruned_beta != best_state[3]):
            pruning_applied = True
            memory.pruning_events += 1
        best_state = (pruned_w, best_state[1], pruned_a, pruned_beta)

    model = NX47AtomNeuron(n_features=x_train.shape[1])
    if best_state is not None:
        model.w, model.b, model.alpha, model.beta = best_state

    if best is not None:
        memory.update(float(best.get('val_f1', 0.0)), float(best.get('best_threshold', 0.5)))

    return model, {
        'selected_hyperparams': best,
        'leaderboard_top5': sorted(leaderboard, key=lambda d: d['objective'])[:5],
        'epoch_history': epoch_history,
        'train_samples': int(x_train.shape[0]),
        'val_samples': int(x_val.shape[0]),
        'adaptive_lr': float(adaptive_lr),
        'mutation_applied': mutation_applied,
        'pruning_applied': pruning_applied,
        'supervised': True,
    }


def train_nx47_autonomous(features: np.ndarray, cfg: V126Config, rng: np.random.Generator, memory: NX47EvolutionMemory | None = None) -> Tuple[NX47AtomNeuron, Dict[str, Any]]:
    x = features.reshape(features.shape[0], -1).T.astype(np.float64)
    keep, y_all = pseudo_labels(np.mean(features, axis=0), cfg.pseudo_pos_pct, cfg.pseudo_neg_pct)
    idx = np.where(keep)[0]
    if idx.size > cfg.train_max_samples:
        idx = rng.choice(idx, size=cfg.train_max_samples, replace=False)
    x_train, y_train = x[idx], y_all[idx]
    cut = int(x_train.shape[0] * 0.85)
    x_tr, y_tr = x_train[:cut], y_train[:cut]
    x_va, y_va = x_train[cut:], y_train[cut:]
    if x_va.shape[0] < 100:
        x_va, y_va = x_tr, y_tr

    best, best_state = None, None
    leaderboard: List[Dict[str, Any]] = []
    adaptive_lr = memory.adapt_learning_rate(cfg.lr, cfg.f1_stagnation_window) if memory else cfg.lr
    for neuron_id in range(max(1, cfg.meta_neurons)):
        for l1 in cfg.l1_candidates:
            for l2 in cfg.l2_candidates:
                m = NX47AtomNeuron(n_features=x.shape[1])
                tr = m.fit_prox(x_tr, y_tr, lr=adaptive_lr, max_iter=cfg.max_iter, l1=float(l1), l2=float(l2))
                grad_x_va = np.gradient(x_va, axis=0)
                p = m.predict_proba(x_va, grad_x_va)
                ce = -float(np.mean(y_va * np.log(p + 1e-9) + (1.0 - y_va) * np.log(1.0 - p + 1e-9)))
                proxy_f1 = compute_proxy_f1(p, y_va)
                sparsity = float(np.mean((np.abs(m.w) + np.abs(m.alpha) + np.abs(m.beta)) < 1e-8))
                reg_lambda = dynamic_regularization_lambda((p > 0.5).astype(np.uint8), features)
                objective = ce + 0.02 * (1.0 - sparsity) - 0.05 * proxy_f1 + 0.005 * reg_lambda
                rec = {"neuron_id": neuron_id, "l1": float(l1), "l2": float(l2), "val_ce": ce, "proxy_f1": proxy_f1, "sparsity": sparsity, "objective": objective, **tr}
                leaderboard.append(rec)
                if best is None or objective < best["objective"]:
                    best = rec
                    best_state = (m.w.copy(), float(m.b), m.alpha.copy(), m.beta.copy())

    mutation_applied = False
    pruning_applied = False
    if best_state is not None and memory is not None:
        if len(memory.f1_history) >= cfg.f1_stagnation_window:
            recent = memory.f1_history[-cfg.f1_stagnation_window:]
            if float(np.max(recent) - np.min(recent)) < 1e-3:
                best_state = (
                    best_state[0] + rng.normal(0.0, cfg.mutation_noise, size=best_state[0].shape),
                    float(best_state[1]),
                    best_state[2] + rng.normal(0.0, cfg.mutation_noise, size=best_state[2].shape),
                    best_state[3] + rng.normal(0.0, cfg.mutation_noise, size=best_state[3].shape),
                )
                mutation_applied = True
                memory.mutation_events += 1
        q = float(np.quantile(np.abs(best_state[0]), cfg.pruning_quantile))
        pruned_w = np.where(np.abs(best_state[0]) < q, 0.0, best_state[0])
        pruned_a = np.where(np.abs(best_state[2]) < q, 0.0, best_state[2])
        pruned_beta = np.where(np.abs(best_state[3]) < q, 0.0, best_state[3])
        if np.any(pruned_w != best_state[0]) or np.any(pruned_a != best_state[2]) or np.any(pruned_beta != best_state[3]):
            pruning_applied = True
            memory.pruning_events += 1
        best_state = (pruned_w, best_state[1], pruned_a, pruned_beta)

    model = NX47AtomNeuron(n_features=x.shape[1])
    if best_state is not None:
        model.w, model.b, model.alpha, model.beta = best_state
    return model, {
        "selected_hyperparams": best,
        "leaderboard_top5": sorted(leaderboard, key=lambda d: d['objective'])[:5],
        "train_samples": int(x_train.shape[0]),
        "label_keep_ratio": float(keep.mean()),
        "adaptive_lr": float(adaptive_lr),
        "mutation_applied": mutation_applied,
        "pruning_applied": pruning_applied,
    }


def hysteresis_topology_3d(prob: np.ndarray, cfg: V126Config) -> np.ndarray:
    strong = prob >= float(cfg.strong_th)
    weak = prob >= float(cfg.weak_th)
    core = binary_propagation(strong, mask=weak, structure=generate_binary_structure(2, 2)) if strong.any() else np.zeros_like(strong, dtype=bool)

    z, r = int(cfg.z_radius), int(cfg.xy_radius)
    struct = np.zeros((2 * z + 1, 2 * r + 1, 2 * r + 1), dtype=bool)
    for dz in range(-z, z + 1):
        for dy in range(-r, r + 1):
            for dx in range(-r, r + 1):
                if dy * dy + dx * dx <= r * r:
                    struct[dz + z, dy + r, dx + r] = True

    vol = np.repeat(core[np.newaxis, ...], 3, axis=0)
    mask = binary_closing(vol, structure=struct).any(axis=0)
    lbl, n = label(mask)
    if n > 0 and cfg.dust_min_size > 1:
        counts = np.bincount(lbl.ravel())
        keep = counts >= int(cfg.dust_min_size)
        keep[0] = False
        mask = keep[lbl]
    return mask


def calibrate_target_ratio(prob: np.ndarray, target_ratio: float) -> np.ndarray:
    ratio = float(np.clip(target_ratio, 0.001, 0.35))
    return prob >= float(np.percentile(prob, 100.0 * (1.0 - ratio)))


def probe_hardware_metrics() -> Dict[str, Any]:
    mem_total_kb = None
    mem_available_kb = None
    try:
        with open('/proc/meminfo', 'r', encoding='utf-8') as f:
            rows = f.read().splitlines()
        kv = {r.split(':')[0]: r.split(':')[1].strip() for r in rows if ':' in r}
        mem_total_kb = int(kv.get('MemTotal', '0 kB').split()[0])
        mem_available_kb = int(kv.get('MemAvailable', '0 kB').split()[0])
    except Exception:
        pass

    gpu = None
    try:
        out = subprocess.check_output(['bash', '-lc', 'nvidia-smi --query-gpu=name,memory.total,memory.free --format=csv,noheader'], stderr=subprocess.DEVNULL, timeout=2).decode().strip()
        gpu = out.splitlines()
    except Exception:
        gpu = []

    return {
        'python': sys.version,
        'platform': platform.platform(),
        'cpu_count': os.cpu_count(),
        'mem_total_kb': mem_total_kb,
        'mem_available_kb': mem_available_kb,
        'gpu': gpu,
    }


class NX47V126Kernel:
    def __init__(self, root: Path = Path('/kaggle/input/competitions/vesuvius-challenge-surface-detection'), output_dir: Path = Path('/kaggle/working'), config: V126Config | None = None) -> None:
        self.version = 'NX47 V126'
        self.root = self._resolve_root(root)
        self.test_dir = self.root / 'test_images'
        self.train_img_dir = self.root / 'train_images'
        self.train_lbl_dir = self.root / 'train_labels'
        self.output_dir = output_dir
        self.tmp_dir = output_dir / 'tmp_masks_v126'
        self.overlay_dir = output_dir / 'overlays_v126'
        self.submission_path = output_dir / 'submission.zip'
        self.roadmap_path = output_dir / 'v126_roadmap_realtime.json'
        self.logs_path = output_dir / 'v126_execution_logs.json'
        self.memory_path = output_dir / 'v126_memory_tracker.json'
        self.metadata_path = output_dir / 'v126_execution_metadata.json'
        self.ultra_log_path = output_dir / 'v126_ultra_authentic_360_merkle.jsonl'

        self.cfg = config or V126Config()
        self.evolution = NX47EvolutionMemory()
        self.plan = PlanTracker(self.roadmap_path)
        self.memory = MemoryTracker()
        self.logs: List[Dict[str, Any]] = []
        self.ultra = UltraAuthentic360Merkle(self.ultra_log_path, console=self.cfg.ultra_console_log)
        self.supervised_model: NX47AtomNeuron | None = None
        self.supervised_train_info: Dict[str, Any] | None = None

        self.global_stats: Dict[str, Any] = {
            'files_processed': 0,
            'slices_processed': 0,
            'pixels_processed': 0,
            'pixels_anchor_detected': 0,
            'pixels_papyrus_without_anchor': 0,
            'materials_detected': 0,
            'patterns_detected': 0,
            'golden_nonce_detected': 0,
            'unknown_discoveries': 0,
            'anomalies_detected': 0,
            'calc_ops_estimated': 0,
            'active_neurons_start_total': 0,
            'active_neurons_mid_total': 0,
            'active_neurons_end_total': 0,
            'meta_neuron_candidates': 0,
            'mutation_events': 0,
            'pruning_events': 0,
            'f1_ratio_curve_best_ratio': 0.0,
            'f1_ratio_curve_best_f1': 0.0,
            'files_supervised_mode': 0,
            'files_autonomous_fallback': 0,
            'val_f1_mean_supervised': 0.0,
            'val_iou_mean_supervised': 0.0,
            'best_threshold_mean_supervised': 0.0,
        }

        bootstrap_dependencies_fail_fast()
        if not ensure_imagecodecs():
            raise RuntimeError('imagecodecs is mandatory for LZW TIFF I/O')

        self.tmp_dir.mkdir(parents=True, exist_ok=True)
        self.overlay_dir.mkdir(parents=True, exist_ok=True)

        for n, d in [
            ('discovery', 'Validation dataset et assets'),
            ('load', 'Chargement volume'),
            ('features', 'Extraction + sélection features'),
            ('train', 'Apprentissage neurone NX-47 L1/L2'),
            ('segment', 'Probabilité + hysteresis + calibration'),
            ('package', 'Génération submission zip'),
        ]:
            self.plan.add_step(n, d)
        self.plan._write()

        self.log('BOOT', version=self.version, root=str(self.root), config=asdict(self.cfg), hardware=probe_hardware_metrics())

    def _resolve_root(self, preferred: Path) -> Path:
        cands = [preferred, Path('/kaggle/input/competitions/vesuvius-challenge-surface-detection'), Path('/kaggle/input/vesuvius-challenge-surface-detection')]
        for c in cands:
            if c.exists():
                return c
        raise FileNotFoundError(f'Dataset path missing. Tried: {[str(c) for c in cands]}')

    def log(self, event: str, **kwargs: Any) -> None:
        payload = {'ts_ns': time.time_ns(), 'event': event, **kwargs}
        payload['signature'] = sha512(json.dumps(payload, sort_keys=True, default=str).encode()).hexdigest()
        self.logs.append(payload)
        self.ultra.emit(payload)

    def _log_array_ultra(self, stage: str, arr: np.ndarray) -> None:
        self.memory.log_array(stage, arr)
        if self.cfg.ultra_bit_trace_arrays:
            self.log('ARRAY_TRACE', stage=stage, shape=list(np.asarray(arr).shape), dtype=str(np.asarray(arr).dtype), bits=self.ultra.bit_stats(arr, self.cfg.ultra_bit_trace_limit))

    def discover_inputs(self) -> List[Path]:
        self.plan.update('discovery', 25.0)
        if not self.test_dir.exists():
            raise FileNotFoundError(f'Missing test_images directory: {self.test_dir}')
        files = sorted(self.test_dir.rglob('*.tif'))
        if not files:
            raise RuntimeError(f'No TIFF files found in {self.test_dir}')

        all_files = [p for p in self.root.rglob('*') if p.is_file()]
        suffix_stats: Dict[str, int] = {}
        folders = set()
        for p in all_files:
            suffix_stats[p.suffix.lower() or '<noext>'] = suffix_stats.get(p.suffix.lower() or '<noext>', 0) + 1
            folders.add(str(p.parent.relative_to(self.root)))

        self.log('DATASET_DISCOVERY', file_count=len(files), total_assets=len(all_files), folders=sorted(folders), suffix_stats=suffix_stats)
        self.plan.update('discovery', 100.0, done=True)
        return files

    def discover_train_pairs(self) -> List[Tuple[Path, Path]]:
        if not self.train_img_dir.exists() or not self.train_lbl_dir.exists():
            self.log('TRAIN_DISCOVERY', status='missing_train_dirs', train_images=str(self.train_img_dir), train_labels=str(self.train_lbl_dir))
            return []
        pairs: List[Tuple[Path, Path]] = []
        for img in sorted(self.train_img_dir.rglob('*.tif')):
            cand = self.train_lbl_dir / img.name
            if cand.exists():
                pairs.append((img, cand))
        self.log('TRAIN_DISCOVERY', pair_count=len(pairs), max_train_volumes=self.cfg.max_train_volumes, max_val_volumes=self.cfg.max_val_volumes)
        return pairs

    @staticmethod
    def _load_label_2d(path: Path) -> np.ndarray:
        arr = read_tiff_lzw_safe(path)
        if arr.ndim == 3:
            arr2d = arr[0]
        elif arr.ndim == 2:
            arr2d = arr
        else:
            raise RuntimeError(f'Unsupported label shape {arr.shape} for {path.name}')
        arr2d = np.asarray(arr2d, dtype=np.float32)
        if arr2d.max() > 1.0:
            arr2d = arr2d / 255.0
        return (arr2d > 0.5).astype(np.float32)

    def build_supervised_model(self) -> None:
        if not self.cfg.supervised_train:
            self.log('SUPERVISED_TRAIN', status='disabled')
            return
        pairs = self.discover_train_pairs()
        if not pairs:
            self.log('SUPERVISED_TRAIN', status='fallback_autonomous_no_pairs')
            return

        rng = np.random.default_rng(125)
        max_total = max(2, self.cfg.max_train_volumes + self.cfg.max_val_volumes)
        chosen = pairs[:max_total]
        split = min(len(chosen) - 1, self.cfg.max_train_volumes)
        train_pairs = chosen[:split]
        val_pairs = chosen[split:]
        if not val_pairs:
            val_pairs = chosen[-1:]
            train_pairs = chosen[:-1]

        x_train_chunks: List[np.ndarray] = []
        y_train_chunks: List[np.ndarray] = []
        x_val_chunks: List[np.ndarray] = []
        y_val_chunks: List[np.ndarray] = []

        for img_path, lbl_path in train_pairs:
            vol = self._load_volume(img_path)
            feats, names = extract_multi_features(vol)
            selected, _, _ = auto_select_features(feats, names, self.cfg.top_k_features)
            x = selected.reshape(selected.shape[0], -1).T.astype(np.float64)
            y = self._load_label_2d(lbl_path).reshape(-1).astype(np.float64)
            n = min(x.shape[0], y.shape[0])
            x, y = x[:n], y[:n]
            idx = _balance_sample_indices(y, self.cfg.max_samples_per_volume, self.cfg.pos_neg_ratio, rng)
            x_train_chunks.append(x[idx])
            y_train_chunks.append(y[idx])

        for img_path, lbl_path in val_pairs:
            vol = self._load_volume(img_path)
            feats, names = extract_multi_features(vol)
            selected, _, _ = auto_select_features(feats, names, self.cfg.top_k_features)
            x = selected.reshape(selected.shape[0], -1).T.astype(np.float64)
            y = self._load_label_2d(lbl_path).reshape(-1).astype(np.float64)
            n = min(x.shape[0], y.shape[0])
            x, y = x[:n], y[:n]
            idx = _balance_sample_indices(y, self.cfg.max_samples_per_volume, self.cfg.pos_neg_ratio, rng)
            x_val_chunks.append(x[idx])
            y_val_chunks.append(y[idx])

        if not x_train_chunks or not x_val_chunks:
            self.log('SUPERVISED_TRAIN', status='fallback_autonomous_empty_chunks')
            return

        x_train = np.concatenate(x_train_chunks, axis=0)
        y_train = np.concatenate(y_train_chunks, axis=0)
        x_val = np.concatenate(x_val_chunks, axis=0)
        y_val = np.concatenate(y_val_chunks, axis=0)

        model, info = train_nx47_supervised(x_train, y_train, x_val, y_val, self.cfg, rng, self.evolution)
        self.supervised_model = model
        self.supervised_train_info = info
        self.log(
            'SUPERVISED_TRAIN',
            status='ok',
            train_samples=int(x_train.shape[0]),
            val_samples=int(x_val.shape[0]),
            train_volumes=len(train_pairs),
            val_volumes=len(val_pairs),
            train_volume_files=[p.name for p, _ in train_pairs],
            val_volume_files=[p.name for p, _ in val_pairs],
            train_info=info,
        )

    def _load_volume(self, path: Path) -> np.ndarray:
        self.plan.update('load', 25.0)
        if self.cfg.ultra_step_log:
            self.log('STEP', name='load_start', file=str(path))
        vol = read_tiff_lzw_safe(path).astype(np.float32)
        if vol.ndim != 3:
            raise RuntimeError(f'Unsupported TIFF shape for {path.name}: {vol.shape}')
        if vol.shape[0] > self.cfg.max_layers:
            vol = vol[: self.cfg.max_layers]
        vol = (vol - float(vol.min())) / (float(vol.max()) - float(vol.min()) + 1e-6)
        self._log_array_ultra('volume', vol)
        self.plan.update('load', 100.0, done=True)
        return vol

    def _predict_mask(self, vol: np.ndarray) -> Tuple[np.ndarray, Dict[str, Any]]:
        self.plan.update('features', 10.0)
        if self.cfg.ultra_step_log:
            self.log('STEP', name='features_extract_start')
        features, names = extract_multi_features(vol)
        selected, selected_names, variances = auto_select_features(features, names, self.cfg.top_k_features)
        self._log_array_ultra('features_selected', selected)
        self.plan.update('features', 100.0, done=True)

        self.plan.update('train', 15.0)
        if self.cfg.ultra_step_log:
            self.log('STEP', name='train_start')
        if self.supervised_model is not None and self.supervised_train_info is not None:
            model = self.supervised_model
            train_info = self.supervised_train_info
            train_mode = 'supervised'
        else:
            rng = np.random.default_rng(47)
            model, train_info = train_nx47_autonomous(selected, self.cfg, rng, self.evolution)
            train_mode = 'autonomous_fallback'
        self.plan.update('train', 100.0, done=True)

        self.plan.update('segment', 20.0)
        if self.cfg.ultra_step_log:
            self.log('STEP', name='segment_start')
        x_full = selected.reshape(selected.shape[0], -1).T.astype(np.float64)
        grad_x_full = np.gradient(x_full, axis=0)
        prob = model.predict_proba(x_full, grad_x_full).reshape(selected.shape[1:]).astype(np.float32)
        self._log_array_ultra('probability_map', prob)

        hysteresis_mask = hysteresis_topology_3d(prob, self.cfg)
        calibrated_mask_scan, ratio_info = choose_adaptive_ratio(prob, self.cfg.ratio_candidates)
        slice_ratio_info = choose_slicewise_adaptive_ratio(vol, prob, self.cfg.ratio_candidates)
        calibrated_mask = np.logical_or(calibrated_mask_scan, slice_ratio_info['mask_global'])
        final = (hysteresis_mask | calibrated_mask).astype(np.uint8)

        self._log_array_ultra('mask_hysteresis', hysteresis_mask.astype(np.uint8))
        self._log_array_ultra('mask_calibrated', calibrated_mask.astype(np.uint8))
        self._log_array_ultra('mask_final', final)

        lbl, comp_count = label(final)
        papyrus_wo_anchor = np.logical_and(calibrated_mask, ~hysteresis_mask)
        golden_nonce = np.logical_and(prob > np.percentile(prob, 99.99), final > 0)
        yy, xx = np.where(golden_nonce)
        nonce_values = prob[yy, xx] if yy.size else np.array([], dtype=np.float32)
        if yy.size:
            ord_idx = np.argsort(nonce_values)[::-1][: self.cfg.golden_nonce_topk]
            golden_nonce_points = [
                {'y': int(yy[k]), 'x': int(xx[k]), 'score': float(nonce_values[k])}
                for k in ord_idx
            ]
        else:
            golden_nonce_points = []
        patterns = comp_count
        anomalies = int(np.sum(np.abs(laplace(prob)) > np.percentile(np.abs(laplace(prob)), 99.95)))
        unknown_discoveries = int(np.sum(prob > 0.9995))

        # approximate operations (coarse estimator)
        pixels2d = int(prob.size)
        ops_est = int(pixels2d * (selected.shape[0] * 12 + 200))

        metrics = {
            'selected_features': selected_names,
            'feature_variances': {names[i]: float(variances[i]) for i in range(len(names))},
            'train_info': train_info,
            'train_mode': train_mode,
            'active_ratio_final': float(final.mean()),
            'active_ratio_hysteresis': float(hysteresis_mask.mean()),
            'active_ratio_calibrated': float(calibrated_mask.mean()),
            'pixels_anchor_detected': int(hysteresis_mask.sum()),
            'pixels_papyrus_without_anchor': int(papyrus_wo_anchor.sum()),
            'materials_detected': int(comp_count),
            'patterns_detected': int(patterns),
            'golden_nonce_detected': int(golden_nonce.sum()),
            'golden_nonce_points_topk': golden_nonce_points,
            'unknown_discoveries': int(unknown_discoveries),
            'anomalies_detected': int(anomalies),
            'pixels_processed_2d': pixels2d,
            'slices_processed': int(vol.shape[0]),
            'calc_ops_estimated': ops_est,
            'ratio_adaptive_selected': float(ratio_info['selected_ratio']),
            'ratio_slice_global_selected': float(slice_ratio_info['ratio_global_selected']),
            'ratio_slice_profile': slice_ratio_info['slice_ratio_profile'],
            'ratio_slice_mean': slice_ratio_info['slice_ratio_mean'],
            'ratio_slice_std': slice_ratio_info['slice_ratio_std'],
            'ratio_scan': ratio_info['ratio_scan'],
            'meta_neuron_candidates': int(self.cfg.meta_neurons * len(self.cfg.l1_candidates) * len(self.cfg.l2_candidates)),
            'mutation_applied': bool(train_info.get('mutation_applied', False)),
            'pruning_applied': bool(train_info.get('pruning_applied', False)),
            'active_neurons_start': int(train_info['selected_hyperparams'].get('active_neurons_start', 0)),
            'active_neurons_mid': int(train_info['selected_hyperparams'].get('active_neurons_mid', 0)),
            'active_neurons_end': int(train_info['selected_hyperparams'].get('active_neurons_end', 0)),
        }
        proxy_like = train_info['selected_hyperparams'].get('proxy_f1', train_info['selected_hyperparams'].get('val_f1', 0.0))
        self.evolution.update(proxy_like, slice_ratio_info['ratio_global_selected'])
        return final, metrics

    def run_simulation_100(self) -> Dict[str, Any]:
        rng = np.random.default_rng(123)
        f1_scores = []
        for _ in range(100):
            prob = rng.random((128, 128), dtype=np.float32)
            pseudo = prob > np.percentile(prob, 94.0)
            f1_scores.append(compute_proxy_f1(prob, pseudo.astype(np.float32), threshold=0.5))
        summary = {
            'samples': 100,
            'f1_mean': float(np.mean(f1_scores)),
            'f1_std': float(np.std(f1_scores)),
            'f1_min': float(np.min(f1_scores)),
            'f1_max': float(np.max(f1_scores)),
        }
        self.log('SIMULATION_100', **summary)
        return summary

    def run(self) -> Path:
        t_global = time.perf_counter()
        files = self.discover_inputs()
        self.build_supervised_model()
        self.plan.update('package', 10.0)
        sup_f1_values: List[float] = []
        sup_iou_values: List[float] = []
        sup_th_values: List[float] = []

        with zipfile.ZipFile(self.submission_path, 'w', zipfile.ZIP_STORED) as zf:
            for i, fpath in enumerate(files, start=1):
                t0 = time.perf_counter()
                self.log('FILE_START', file=fpath.name, index=i, total=len(files))
                for st in ('load', 'features', 'train', 'segment'):
                    self.plan.update(st, 0.0, done=False)

                vol = self._load_volume(fpath)
                mask2d, m = self._predict_mask(vol)
                self.log('NX47_METRICS', file=fpath.name, **m)

                self.global_stats['files_processed'] += 1
                self.global_stats['slices_processed'] += m['slices_processed']
                self.global_stats['pixels_processed'] += int(vol.size)
                self.global_stats['pixels_anchor_detected'] += m['pixels_anchor_detected']
                self.global_stats['pixels_papyrus_without_anchor'] += m['pixels_papyrus_without_anchor']
                self.global_stats['materials_detected'] += m['materials_detected']
                self.global_stats['patterns_detected'] += m['patterns_detected']
                self.global_stats['golden_nonce_detected'] += m['golden_nonce_detected']
                self.global_stats['unknown_discoveries'] += m['unknown_discoveries']
                self.global_stats['anomalies_detected'] += m['anomalies_detected']
                self.global_stats['calc_ops_estimated'] += m['calc_ops_estimated']
                self.global_stats['active_neurons_start_total'] += m['active_neurons_start']
                self.global_stats['active_neurons_mid_total'] += m['active_neurons_mid']
                self.global_stats['active_neurons_end_total'] += m['active_neurons_end']
                self.global_stats['meta_neuron_candidates'] += m['meta_neuron_candidates']
                self.global_stats['mutation_events'] += int(m['mutation_applied'])
                self.global_stats['pruning_events'] += int(m['pruning_applied'])
                if m.get('train_mode') == 'supervised':
                    self.global_stats['files_supervised_mode'] += 1
                    shp = m.get('train_info', {}).get('selected_hyperparams', {})
                    if 'val_f1' in shp:
                        sup_f1_values.append(float(shp['val_f1']))
                    if 'val_iou' in shp:
                        sup_iou_values.append(float(shp['val_iou']))
                    if 'best_threshold' in shp:
                        sup_th_values.append(float(shp['best_threshold']))
                else:
                    self.global_stats['files_autonomous_fallback'] += 1

                out_mask = self.tmp_dir / fpath.name
                write_tiff_lzw_safe(out_mask, mask2d[np.newaxis, ...])
                zf.write(out_mask, arcname=fpath.name)
                out_mask.unlink(missing_ok=True)
                gc.collect()

                dt = max(1e-9, time.perf_counter() - t0)
                cps = m['calc_ops_estimated'] / dt
                self.log('FILE_DONE', file=fpath.name, active_ratio=round(float(mask2d.mean()), 6), calc_per_sec=float(cps), elapsed_s=round(dt, 3))
                self.plan.update('package', 10.0 + 85.0 * (i / len(files)))

        total_dt = max(1e-9, time.perf_counter() - t_global)
        self.global_stats['calc_per_sec_global'] = float(self.global_stats['calc_ops_estimated'] / total_dt)
        self.global_stats['elapsed_total_s'] = float(total_dt)
        self.global_stats['ratio_selected_mean'] = float(np.mean(self.evolution.ratio_history)) if self.evolution.ratio_history else 0.0
        self.global_stats['val_f1_mean_supervised'] = float(np.mean(sup_f1_values)) if sup_f1_values else 0.0
        self.global_stats['val_iou_mean_supervised'] = float(np.mean(sup_iou_values)) if sup_iou_values else 0.0
        self.global_stats['best_threshold_mean_supervised'] = float(np.mean(sup_th_values)) if sup_th_values else 0.0
        f1_curve = simulate_f1_vs_ratio_curve()
        self.global_stats['f1_ratio_curve_best_ratio'] = float(f1_curve['best_ratio'])
        self.global_stats['f1_ratio_curve_best_f1'] = float(f1_curve['best_f1'])

        sim = self.run_simulation_100() if self.cfg.run_simulation_100 else None

        self.log('GLOBAL_STATS', **self.global_stats)

        metadata = {
            'version': self.version,
            'root': str(self.root),
            'input_dir': str(self.test_dir),
            'submission_zip': str(self.submission_path),
            'log_count': len(self.logs),
            'ultra_log': str(self.ultra_log_path),
            'hardware': probe_hardware_metrics(),
            'global_stats': self.global_stats,
            'evolution_memory': asdict(self.evolution),
            'supervised_train_info': self.supervised_train_info,
            'simulation_100': sim,
            'f1_ratio_curve': f1_curve,
            'config': asdict(self.cfg),
        }
        self.metadata_path.write_text(json.dumps(metadata, indent=2), encoding='utf-8')
        self.logs_path.write_text(json.dumps(self.logs, indent=2), encoding='utf-8')
        self.memory_path.write_text(json.dumps(self.memory.events, indent=2), encoding='utf-8')

        self.plan.update('package', 100.0, done=True)
        self.log('EXEC_COMPLETE', submission=str(self.submission_path))
        return self.submission_path


if __name__ == '__main__':
    cfg = V126Config(
        top_k_features=int(os.environ.get('V126_TOP_K_FEATURES', '6')),
        target_active_ratio=float(os.environ.get('V126_TARGET_ACTIVE_RATIO', '0.03')),
        full_pixel_trace=os.environ.get('V126_FULL_PIXEL_TRACE', '0') == '1',
        trace_pixel_budget=int(os.environ.get('V126_TRACE_PIXEL_BUDGET', '4000')),
        ultra_console_log=os.environ.get('V126_ULTRA_CONSOLE_LOG', '1') == '1',
        ultra_step_log=os.environ.get('V126_ULTRA_STEP_LOG', '1') == '1',
        ultra_bit_trace_arrays=os.environ.get('V126_ULTRA_BIT_TRACE_ARRAYS', '1') == '1',
        ultra_bit_trace_limit=int(os.environ.get('V126_ULTRA_BIT_TRACE_LIMIT', '64')),
        meta_neurons=int(os.environ.get('V126_META_NEURONS', '3')),
        run_simulation_100=os.environ.get('V126_RUN_SIMULATION_100', '0') == '1',
        simulation_export_curve=os.environ.get('V126_SIMULATION_EXPORT_CURVE', '1') == '1',
        supervised_train=os.environ.get('V126_SUPERVISED_TRAIN', '1') == '1',
        max_train_volumes=int(os.environ.get('V126_MAX_TRAIN_VOLUMES', '24')),
        max_val_volumes=int(os.environ.get('V126_MAX_VAL_VOLUMES', '8')),
        max_samples_per_volume=int(os.environ.get('V126_MAX_SAMPLES_PER_VOLUME', '40000')),
        pos_neg_ratio=float(os.environ.get('V126_POS_NEG_RATIO', '1.0')),
        golden_nonce_topk=int(os.environ.get('V126_GOLDEN_NONCE_TOPK', '11')),
        supervised_epochs=int(os.environ.get('V126_SUPERVISED_EPOCHS', '3')),
        fbeta_beta=float(os.environ.get('V126_F_BETA', '0.5')),
    )
    kernel = NX47V126Kernel(
        root=Path(os.environ.get('VESUVIUS_ROOT', '/kaggle/input/competitions/vesuvius-challenge-surface-detection')),
        output_dir=Path(os.environ.get('VESUVIUS_OUTPUT', '/kaggle/working')),
        config=cfg,
    )
    submission = kernel.run()
    print(f'READY: {submission}')
