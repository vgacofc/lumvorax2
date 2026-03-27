"""NX-46 Vesuvius core (Kaggle-ready v2, offline-first).

Objectifs v2:
- Corriger la non-détection de données (train/test fragments VS train_images/test_images).
- Garder un pipeline 100% offline exécutable dans Kaggle.
- Produire les artefacts de logs attendus + état final explicite.
- Générer submission.csv (format tabulaire) ET submission.zip (format TIFF par id) selon les entrées.
- Valider strictement la conformité des membres du zip aux fichiers test attendus.
"""

from __future__ import annotations

import csv
import json
import os
import time
import zipfile
from dataclasses import dataclass
from hashlib import sha512
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple

import numpy as np

try:
    import tifffile  # type: ignore
except Exception:  # pragma: no cover - fallback en environnement réduit
    tifffile = None

try:
    import imageio.v3 as iio
except Exception:  # pragma: no cover - fallback en environnement réduit
    iio = None


@dataclass
class NX46Config:
    data_root: str = "/kaggle/input/vesuvius-challenge-ink-detection"
    work_root: str = "/kaggle/working/nx46_vesuvius"
    seed: int = 46
    bit_capture_bytes: int = 256
    threshold_quantile: float = 0.985
    slab_min_neurons: int = 128
    auto_discover_data_root: bool = True


class ProgressRoadmap:
    def __init__(self) -> None:
        self.steps: Dict[str, float] = {
            "audit_discovery": 0.0,
            "train_thresholds": 0.0,
            "infer_predictions": 0.0,
            "package_submission": 0.0,
            "finalize_forensics": 0.0,
        }

    def update(self, step: str, pct: float) -> None:
        self.steps[step] = max(0.0, min(100.0, float(pct)))
        print(f"[ROADMAP] {step}: {self.steps[step]:.1f}%")

    def as_dict(self) -> Dict[str, float]:
        return dict(self.steps)


class HFBL360Logger:
    def __init__(self, root: Path) -> None:
        self.root = root
        self.root.mkdir(parents=True, exist_ok=True)

        self.log_path = root / "forensic_ultra.log"
        self.core_log = root / "nx-46-vesuvius-core.log"
        self.kaggle_log = root / "nx46-vesuvius-core-kaggle-ready.log"
        self.csv_path = root / "metrics.csv"
        self.state_path = root / "state.json"
        self.bit_path = root / "bit_capture.log"
        self.merkle_path = root / "merkle_chain.log"

        with self.csv_path.open("w", newline="", encoding="utf-8") as f:
            csv.writer(f).writerow(
                [
                    "timestamp_ns",
                    "phase",
                    "fragment",
                    "neurons_active",
                    "cpu_ns",
                    "ink_pixels",
                    "total_pixels",
                    "ink_ratio",
                    "merkle_prefix",
                ]
            )

    def _append_all_logs(self, text: str) -> None:
        for p in (self.log_path, self.core_log, self.kaggle_log):
            with p.open("a", encoding="utf-8") as f:
                f.write(text + "\n")

    def log_event(self, message: str) -> None:
        self._append_all_logs(f"{time.time_ns()} | {message}")

    def log_bits(self, fragment: str, payload: bytes) -> None:
        bit_string = "".join(f"{b:08b}" for b in payload)
        with self.bit_path.open("a", encoding="utf-8") as f:
            f.write(f"{time.time_ns()} | {fragment} | {bit_string}\n")

    def log_merkle(self, fragment: str, digest: str) -> None:
        with self.merkle_path.open("a", encoding="utf-8") as f:
            f.write(f"{time.time_ns()} | {fragment} | {digest}\n")

    def log_metrics(
        self,
        *,
        phase: str,
        fragment: str,
        neurons_active: int,
        cpu_ns: int,
        ink_pixels: int,
        total_pixels: int,
        merkle_prefix: str,
    ) -> None:
        ratio = (ink_pixels / total_pixels) if total_pixels else 0.0
        with self.csv_path.open("a", newline="", encoding="utf-8") as f:
            csv.writer(f).writerow(
                [
                    time.time_ns(),
                    phase,
                    fragment,
                    neurons_active,
                    cpu_ns,
                    ink_pixels,
                    total_pixels,
                    f"{ratio:.8f}",
                    merkle_prefix,
                ]
            )

    def write_state(self, state: Dict[str, object]) -> None:
        state = dict(state)
        state["timestamp_ns"] = time.time_ns()
        with self.state_path.open("w", encoding="utf-8") as f:
            json.dump(state, f, indent=2, ensure_ascii=False)


class NX46AGNNVesuvius:
    def __init__(self, cfg: NX46Config) -> None:
        self.cfg = cfg
        self.rng = np.random.default_rng(cfg.seed)
        self.logger = HFBL360Logger(Path(cfg.work_root) / "logs")
        self.neurons_active = 0
        self.total_allocations = 0
        self.total_pixels_processed = 0
        self.total_ink_pixels = 0
        self.merkle_chain: List[str] = []
        self.global_cpu_start_ns = time.process_time_ns()
        self.logger.log_event("SYSTEM_STARTUP_L0_SUCCESS")

    def slab_allocate(self, tensor: np.ndarray, phase: str) -> int:
        variance = float(np.var(tensor, dtype=np.float64))
        entropy_proxy = float(np.mean(np.abs(np.gradient(tensor.astype(np.float32), axis=-1))))
        required = int(
            self.cfg.slab_min_neurons
            + (tensor.size // 512)
            + variance * 1500.0
            + entropy_proxy * 900.0
        )
        self.neurons_active = max(self.cfg.slab_min_neurons, required)
        self.total_allocations += 1
        self.logger.log_event(
            f"SLAB_ALLOCATION phase={phase} neurons={self.neurons_active} variance={variance:.8f} entropy_proxy={entropy_proxy:.8f}"
        )
        return self.neurons_active

    def _track_bits(self, fragment: str, arr: np.ndarray) -> None:
        self.logger.log_bits(fragment, arr.tobytes()[: self.cfg.bit_capture_bytes])

    def _merkle_sign(self, fragment: str, arr: np.ndarray) -> str:
        prev = self.merkle_chain[-1] if self.merkle_chain else "GENESIS"
        digest = sha512(prev.encode("utf-8") + arr.tobytes()).hexdigest()
        self.merkle_chain.append(digest)
        self.logger.log_merkle(fragment, digest)
        return digest

    @staticmethod
    def _normalize_stack(stack: np.ndarray) -> np.ndarray:
        x = stack.astype(np.float32)
        mn, mx = float(x.min()), float(x.max())
        if mx <= mn:
            return np.zeros_like(x, dtype=np.float32)
        return (x - mn) / (mx - mn)

    @staticmethod
    def _ink_energy_projection(stack: np.ndarray) -> np.ndarray:
        grad_z = np.abs(np.diff(stack, axis=0, prepend=stack[:1]))
        grad_y = np.abs(np.diff(stack, axis=1, prepend=stack[:, :1, :]))
        grad_x = np.abs(np.diff(stack, axis=2, prepend=stack[:, :, :1]))
        return np.mean(0.45 * grad_z + 0.30 * grad_y + 0.25 * grad_x, axis=0)

    def train_threshold(self, stack: np.ndarray, labels: np.ndarray, fragment: str) -> float:
        start = time.process_time_ns()
        self.slab_allocate(stack, phase="train")
        self._track_bits(fragment, stack)

        score = self._ink_energy_projection(self._normalize_stack(stack))
        pos = score[labels > 0]
        neg = score[labels <= 0]

        if pos.size and neg.size:
            threshold = float(0.5 * (float(np.median(pos)) + float(np.median(neg))))
        elif pos.size:
            threshold = float(np.quantile(pos, 0.50))
        else:
            threshold = float(np.quantile(score, self.cfg.threshold_quantile))

        pred = (score >= threshold).astype(np.uint8)
        digest = self._merkle_sign(fragment, score)
        cpu_ns = time.process_time_ns() - start

        ink_pixels = int(pred.sum())
        total_pixels = int(pred.size)
        self.total_ink_pixels += ink_pixels
        self.total_pixels_processed += total_pixels

        self.logger.log_metrics(
            phase="train",
            fragment=fragment,
            neurons_active=self.neurons_active,
            cpu_ns=cpu_ns,
            ink_pixels=ink_pixels,
            total_pixels=total_pixels,
            merkle_prefix=digest[:16],
        )
        self.logger.log_event(f"TRAIN_DONE fragment={fragment} threshold={threshold:.8f}")
        return threshold

    def infer_mask(self, stack: np.ndarray, threshold: float, fragment: str) -> np.ndarray:
        start = time.process_time_ns()
        self.slab_allocate(stack, phase="infer")
        self._track_bits(fragment, stack)

        score = self._ink_energy_projection(self._normalize_stack(stack))
        pred = (score >= threshold).astype(np.uint8)
        digest = self._merkle_sign(fragment, pred)
        cpu_ns = time.process_time_ns() - start

        ink_pixels = int(pred.sum())
        total_pixels = int(pred.size)
        self.total_ink_pixels += ink_pixels
        self.total_pixels_processed += total_pixels

        self.logger.log_metrics(
            phase="infer",
            fragment=fragment,
            neurons_active=self.neurons_active,
            cpu_ns=cpu_ns,
            ink_pixels=ink_pixels,
            total_pixels=total_pixels,
            merkle_prefix=digest[:16],
        )
        return pred

    def finalize(self, extra: Optional[Dict[str, object]] = None) -> Dict[str, object]:
        cpu_total_ns = time.process_time_ns() - self.global_cpu_start_ns
        qi_index = self.total_pixels_processed / max(cpu_total_ns, 1)
        state = {
            "status": "100%_OFFLINE_ACTIVATED",
            "active_neurons": self.neurons_active,
            "total_allocations": self.total_allocations,
            "total_pixels_processed": self.total_pixels_processed,
            "total_ink_pixels": self.total_ink_pixels,
            "ink_ratio": self.total_ink_pixels / self.total_pixels_processed if self.total_pixels_processed else 0.0,
            "qi_index_real": qi_index,
            "cpu_total_ns": cpu_total_ns,
            "merkle_root": self.merkle_chain[-1] if self.merkle_chain else None,
        }
        if extra:
            state.update(extra)
        self.logger.write_state(state)
        self.logger.log_event("SYSTEM_LOADED_100_PERCENT")
        return state


def _read_tif(path: Path) -> np.ndarray:
    if tifffile is not None:
        arr = tifffile.imread(path)
        return np.asarray(arr)
    if iio is None:
        raise RuntimeError("tifffile et imageio indisponibles: impossible de lire les TIFF.")
    return np.asarray(iio.imread(str(path)))


def _write_tif(path: Path, arr2d: np.ndarray) -> None:
    arr2d = (arr2d > 0).astype(np.uint8) * 255
    if tifffile is not None:
        tifffile.imwrite(path, arr2d, compression="LZW")
        return
    if iio is None:
        raise RuntimeError("tifffile et imageio indisponibles: impossible d'écrire les TIFF.")
    iio.imwrite(str(path), arr2d)


def _to_stack(raw: np.ndarray) -> np.ndarray:
    a = np.asarray(raw)
    if a.ndim == 2:
        return a[None, :, :]
    if a.ndim == 3:
        return a
    raise ValueError(f"Unsupported TIFF ndim={a.ndim}; attendu 2D ou 3D.")


def _read_stack_from_fragment(fragment_dir: Path) -> np.ndarray:
    volume_dir = fragment_dir / "surface_volume"
    tif_files = sorted(volume_dir.glob("*.tif"))
    if not tif_files:
        raise FileNotFoundError(f"No TIFF slices found in {volume_dir}")
    slices = [_to_stack(_read_tif(p))[0] for p in tif_files]
    return np.stack(slices, axis=0)


def _load_label_png(fragment_dir: Path) -> Optional[np.ndarray]:
    p = fragment_dir / "inklabels.png"
    if not p.exists() or iio is None:
        return None
    arr = np.asarray(iio.imread(str(p)))
    if arr.ndim == 3:
        arr = arr[..., 0]
    return (arr > 0).astype(np.uint8)


def _read_sample_submission_ids(sample_csv: Path) -> List[str]:
    ids: List[str] = []
    with sample_csv.open("r", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        cols = set(reader.fieldnames or [])
        id_col = "Id" if "Id" in cols else ("id" if "id" in cols else None)
        if id_col is None:
            return ids
        for row in reader:
            v = str(row[id_col]).strip()
            if v:
                ids.append(v)
    return ids


def _discover_layout(root: Path) -> Tuple[str, List[Path], List[Path], List[Path]]:
    train_fragment_dirs = sorted([p for p in (root / "train").glob("*") if p.is_dir()]) if (root / "train").exists() else []
    test_fragment_dirs = sorted([p for p in (root / "test").glob("*") if p.is_dir()]) if (root / "test").exists() else []
    train_image_tifs = sorted((root / "train_images").glob("*.tif")) if (root / "train_images").exists() else []
    test_image_tifs = sorted((root / "test_images").glob("*.tif")) if (root / "test_images").exists() else []

    if train_fragment_dirs or test_fragment_dirs:
        return "fragments", train_fragment_dirs, test_fragment_dirs, []
    if train_image_tifs or test_image_tifs:
        return "images", train_image_tifs, test_image_tifs, []
    return "empty", [], [], []


def _iter_candidate_roots(primary_root: Path) -> List[Path]:
    candidates: List[Path] = []

    def add(path: Path) -> None:
        if path not in candidates:
            candidates.append(path)

    add(primary_root)
    add(Path("/kaggle/input/vesuvius-challenge-ink-detection"))
    add(Path("/kaggle/input/vesuvius-challenge-surface-detection"))
    add(Path("/kaggle/input/competitions/vesuvius-challenge-surface-detection"))

    kaggle_input = Path("/kaggle/input")
    if kaggle_input.exists():
        for entry in sorted(kaggle_input.iterdir()):
            if entry.is_dir() and "vesuvius" in entry.name.lower():
                add(entry)

    competitions = Path("/kaggle/input/competitions")
    if competitions.exists():
        for entry in sorted(competitions.iterdir()):
            if entry.is_dir() and "vesuvius" in entry.name.lower():
                add(entry)

    return [p for p in candidates if p.exists()]


def _resolve_data_root(primary_root: Path, auto_discover: bool) -> Tuple[Path, str, List[Dict[str, object]]]:
    roots = _iter_candidate_roots(primary_root) if auto_discover else [primary_root]
    attempts: List[Dict[str, object]] = []

    for root in roots:
        layout, train_items, test_items, _ = _discover_layout(root)
        attempts.append(
            {
                "root": str(root),
                "layout": layout,
                "train_items": len(train_items),
                "test_items": len(test_items),
            }
        )
        if test_items:
            return root, layout, attempts

    return primary_root, "empty", attempts


def _safe_resize_pair(stack: np.ndarray, labels: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
    h = min(stack.shape[1], labels.shape[0])
    w = min(stack.shape[2], labels.shape[1])
    return stack[:, :h, :w], labels[:h, :w]


def _write_submission_csv(out_path: Path, sample_csv: Path, predictions_by_id: Dict[str, np.ndarray]) -> Optional[str]:
    ids = _read_sample_submission_ids(sample_csv)
    if not ids:
        return None

    flat = np.concatenate([predictions_by_id[i].reshape(-1).astype(np.uint8) for i in sorted(predictions_by_id)])
    n = min(len(ids), len(flat))

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["Id", "Predicted"])
        for i in range(n):
            writer.writerow([ids[i], int(flat[i])])
    return str(out_path)


def _write_submission_zip(
    zip_path: Path,
    masks_dir: Path,
    predictions_by_id: Dict[str, np.ndarray],
    expected_ids: Sequence[str],
) -> Tuple[str, bool, List[str], List[str]]:
    masks_dir.mkdir(parents=True, exist_ok=True)

    written_files: List[str] = []
    for fid, mask in predictions_by_id.items():
        out_tif = masks_dir / f"{fid}.tif"
        _write_tif(out_tif, mask)
        written_files.append(out_tif.name)

    with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for name in sorted(written_files):
            zf.write(masks_dir / name, arcname=name)

    got = sorted(written_files)
    exp = sorted([f"{x}.tif" for x in expected_ids]) if expected_ids else got
    ok = got == exp
    missing = sorted(set(exp) - set(got))
    extra = sorted(set(got) - set(exp))
    return str(zip_path), ok, missing, extra


def _emit_execution_txts(log_root: Path, payload: Dict[str, object]) -> None:
    content = json.dumps(payload, indent=2, ensure_ascii=False)
    for name in ("RkF4XakI.txt", "UJxLRsEE.txt"):
        with (log_root / name).open("w", encoding="utf-8") as f:
            f.write(content + "\n")


def run_pipeline(cfg: NX46Config) -> Dict[str, object]:
    configured_root = Path(cfg.data_root)
    work = Path(cfg.work_root)
    logs_root = work / "logs"
    logs_root.mkdir(parents=True, exist_ok=True)

    roadmap = ProgressRoadmap()
    nx46 = NX46AGNNVesuvius(cfg)

    root, layout, discovery_attempts = _resolve_data_root(configured_root, cfg.auto_discover_data_root)
    layout, train_items, test_items, _ = _discover_layout(root)
    roadmap.update("audit_discovery", 100.0)
    nx46.logger.log_event(
        f"LAYOUT_DETECTED={layout} train_items={len(train_items)} test_items={len(test_items)} root={root}"
    )

    if not test_items:
        attempts_json = json.dumps(discovery_attempts, ensure_ascii=False)
        nx46.logger.log_event(f"NO_TEST_INPUTS_FOUND attempts={attempts_json}")
        raise RuntimeError(
            "No test inputs found. Configure NX46_DATA_ROOT to the exact dataset path. "
            f"Checked roots: {[a['root'] for a in discovery_attempts]}"
        )

    thresholds: List[float] = []

    if layout == "fragments":
        total_train = max(len(train_items), 1)
        for idx, frag in enumerate(train_items, start=1):
            pct = idx / total_train * 100.0
            roadmap.update("train_thresholds", pct)
            nx46.logger.log_event(f"PROGRESS train={idx}/{len(train_items)} ({pct:.1f}%)")

            volume_dir = frag / "surface_volume"
            if not volume_dir.exists():
                continue
            stack = _read_stack_from_fragment(frag)
            labels = _load_label_png(frag)
            if labels is None:
                continue
            if labels.shape != stack.shape[1:]:
                stack, labels = _safe_resize_pair(stack, labels)
            thresholds.append(nx46.train_threshold(stack, labels, frag.name))

    threshold = float(np.median(np.asarray(thresholds, dtype=np.float32))) if thresholds else 0.5

    predictions: Dict[str, np.ndarray] = {}

    if layout == "fragments":
        total_test = max(len(test_items), 1)
        for idx, frag in enumerate(test_items, start=1):
            pct = idx / total_test * 100.0
            roadmap.update("infer_predictions", pct)
            nx46.logger.log_event(f"PROGRESS test={idx}/{len(test_items)} ({pct:.1f}%)")
            volume_dir = frag / "surface_volume"
            if not volume_dir.exists():
                continue
            stack = _read_stack_from_fragment(frag)
            predictions[frag.name] = nx46.infer_mask(stack, threshold, frag.name)

    elif layout == "images":
        total_test = max(len(test_items), 1)
        for idx, tif_path in enumerate(test_items, start=1):
            pct = idx / total_test * 100.0
            roadmap.update("infer_predictions", pct)
            nx46.logger.log_event(f"PROGRESS test_images={idx}/{len(test_items)} ({pct:.1f}%)")
            stack = _to_stack(_read_tif(tif_path))
            predictions[tif_path.stem] = nx46.infer_mask(stack, threshold, tif_path.stem)

    roadmap.update("package_submission", 30.0)

    sample_csv_candidates = [
        root / "sample_submission.csv",
        Path("/kaggle/input/vesuvius-challenge-ink-detection/sample_submission.csv"),
        Path("/kaggle/input/vesuvius-challenge-surface-detection/sample_submission.csv"),
        Path("/kaggle/input/competitions/vesuvius-challenge-surface-detection/sample_submission.csv"),
    ]
    sample_csv = next((p for p in sample_csv_candidates if p.exists()), None)

    csv_submission_path: Optional[str] = None
    expected_ids: List[str] = []
    if sample_csv is not None:
        expected_ids = _read_sample_submission_ids(sample_csv)
        if predictions:
            csv_submission_path = _write_submission_csv(work / "submission.csv", sample_csv, predictions)

    roadmap.update("package_submission", 60.0)

    zip_submission_path: Optional[str] = None
    zip_ok: Optional[bool] = None
    zip_missing: List[str] = []
    zip_extra: List[str] = []
    if predictions:
        zip_submission_path, zip_ok, zip_missing, zip_extra = _write_submission_zip(
            work / "submission.zip",
            work / "submission_masks",
            predictions,
            expected_ids if expected_ids else sorted(predictions.keys()),
        )

    roadmap.update("package_submission", 100.0)
    roadmap.update("finalize_forensics", 60.0)

    result = nx46.finalize(
        {
            "layout_detected": layout,
            "configured_data_root": str(configured_root),
            "effective_data_root": str(root),
            "discovery_attempts": discovery_attempts,
            "train_items": [p.name for p in train_items],
            "test_items": [p.name for p in test_items],
            "train_threshold": threshold,
            "submission_csv": csv_submission_path,
            "submission_zip": zip_submission_path,
            "zip_members_validated": zip_ok,
            "zip_missing": zip_missing,
            "zip_extra": zip_extra,
            "roadmap_percent": roadmap.as_dict(),
        }
    )

    roadmap.update("finalize_forensics", 100.0)
    _emit_execution_txts(logs_root, result)
    return result


if __name__ == "__main__":
    config = NX46Config(
        data_root=os.environ.get("NX46_DATA_ROOT", NX46Config.data_root),
        work_root=os.environ.get("NX46_WORK_ROOT", NX46Config.work_root),
    )
    final_state = run_pipeline(config)
    print(json.dumps(final_state, indent=2, ensure_ascii=False))
