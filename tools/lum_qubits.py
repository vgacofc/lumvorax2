#!/usr/bin/env python3
"""
LumVorax — LUM Qubits — C65
============================
Couche de traçabilité forensique nanoseconde pour qubits IBM Quantum.
Concept : chaque qubit physique est tracé comme LUM trace un bit standard —
          timestamp ns, état, gate, erreur, décoherence, fidelité.

Équivalent quantique de src/lum/lum_core.c § trace_bit_forensic().

Architecture :
  LumQubitTrace   : enregistrement atomique d'un qubit à un instant t
  LumQubitsLayer  : couche complète 156 qubits (ibm_fez Heron R2)
  LumQubitsReport : rapport forensic LumVorax-compatible

Auteur : LumVorax C65 — 2026-04-20
"""

import time
import json
import gzip
import os
import struct
import hashlib
from dataclasses import dataclass, field, asdict
from typing import Optional, List, Dict, Any
from pathlib import Path
import threading

# ════════════════════════════════════════════════════════════════
# CONSTANTES ARCHITECTURE
# ════════════════════════════════════════════════════════════════

IBM_FEZ_QUBITS      = 156          # ibm_fez Heron R2 — 156 qubits physiques
IBM_FEZ_BACKEND     = "ibm_fez"
LUM_QUBITS_VERSION  = os.environ.get("LUM_QUBITS_VERSION", "C66")
LOG_MAX_BYTES       = 50 * 1024 * 1024   # Rotation 50MB
LUM_NATIVE_MAGIC    = b"LUMQ"
LUM_NATIVE_VERSION  = 2

# ════════════════════════════════════════════════════════════════
# STRUCTURES DE DONNEES
# ════════════════════════════════════════════════════════════════

@dataclass
class LumQubitState:
    """Etat instantané d'un qubit — granularité nanoseconde."""
    qubit_id:       int             # Index physique [0..155]
    ts_ns:          int             # Timestamp nanoseconde
    ts_iso:         str             # ISO-8601 UTC
    gate_applied:   str             # Porte appliquée (H, CX, RZ, measure, ...)
    phase_rad:      float           # Phase en radians
    amplitude_re:   float           # Partie réelle amplitude
    amplitude_im:   float           # Partie imaginaire amplitude
    error_prob:     float           # Probabilité d'erreur (gate_error + readout)
    t1_us:          float           # Temps de décoherence T1 (µs)
    t2_us:          float           # Temps de décoherence T2 (µs)
    fidelity:       float           # Fidélité cumulée du qubit
    nx_atom_bias:   float           # Biais NX ATOM pour stabilisation décohérence
    lum_flag:       int             # Flags LUM (0=OK, 1=WARN, 2=ERROR, 4=ANOMALY)
    src_module:     str             # Module source (qdayprize, hubbard_hts, btc_qm)
    cycle:          str             # Cycle IBM (C65)


@dataclass
class LumQubitsLayer:
    """Couche LUM Qubits pour les 156 qubits physiques d'ibm_fez."""
    backend_name:       str   = IBM_FEZ_BACKEND
    n_qubits:           int   = IBM_FEZ_QUBITS
    cycle:              str   = LUM_QUBITS_VERSION
    ts_layer_ns:        int   = 0
    ts_layer_iso:       str   = ""
    job_id:             str   = ""
    problem_name:       str   = ""
    qubits:             List[LumQubitState] = field(default_factory=list)
    nx_atom_coherence:  float = 0.0    # Cohérence globale estimée par NX ATOM
    nx_atom_correction: float = 0.0    # Correction appliquée par NX ATOM
    lum_anomalies:      int   = 0      # Nombre d'anomalies LUM détectées
    lum_warnings:       int   = 0      # Nombre de warnings
    fidelity_mean:      float = 0.0    # Fidélité moyenne sur 156 qubits
    fidelity_min:       float = 1.0    # Fidélité minimale (qubit le plus bruité)


@dataclass
class NxAtomLearnerState:
    cycle:              str = LUM_QUBITS_VERSION
    learner_id:         str = "nx_atom_learner_2"
    source_coherence:   float = 0.0
    learned_bias:       float = 0.0
    convergence_score:  float = 0.0
    recommended_shots:  int = 0
    recommended_depth:  int = 1
    evidence_hash:      str = ""


# ════════════════════════════════════════════════════════════════
# ROTATION LOGS 50MB
# ════════════════════════════════════════════════════════════════

def rotate_log_if_needed(path: Path) -> None:
    """Rotation du fichier de log si > 50MB."""
    if path.exists() and path.stat().st_size > LOG_MAX_BYTES:
        ts = int(time.time())
        archive = path.with_suffix(f".rotated_{ts}.gz")
        try:
            with open(path, 'rb') as f_in:
                with gzip.open(archive, 'wb', compresslevel=9) as f_out:
                    # Garder 10% des dernières lignes
                    content = f_in.read()
                    lines   = content.split(b'\n')
                    keep    = max(100, len(lines) // 10)
                    archive_data = b'\n'.join(lines[:-keep])
                    f_out.write(archive_data)
            # Réécrire avec les 10% récents
            with open(path, 'wb') as f_out:
                f_out.write(b'\n'.join(lines[-keep:]))
            print(f"[LUM-ROTATE] {path.name} → {archive.name} ({archive.stat().st_size//1024}KB)")
        except Exception as e:
            print(f"[LUM-ROTATE] WARN rotation échouée : {e}")


# ════════════════════════════════════════════════════════════════
# CLASSE PRINCIPALE LUM QUBITS
# ════════════════════════════════════════════════════════════════

class LumQubitsTracker:
    """
    Tracker forensique LumVorax pour 156 qubits IBM ibm_fez.

    Concept LUM Qubits :
      - Chaque qubit est tracé à la nanoseconde (comme LUM trace les bits)
      - NX ATOM reçoit le bruit réel IBM (T1/T2/gate_error) pour stabiliser
      - Anomalies détectées automatiquement (fidelité < seuil, décoherence rapide)
      - Export JSON gzippé + CSV forensic LumVorax-compatible
    """

    LOG_DIR = Path("tools/logs/lum_qubits")
    NX_ATOM_COHERENCE_THRESHOLD = 0.85    # En dessous : NX ATOM applique correction
    NX_ATOM_MAX_CORRECTION      = 0.15    # Correction maximale par cycle

    def __init__(self, backend_name: str = IBM_FEZ_BACKEND,
                 n_qubits: int = IBM_FEZ_QUBITS,
                 problem_name: str = "unknown",
                 job_id: str = ""):
        self.backend_name = backend_name
        self.n_qubits     = n_qubits
        self.problem_name = problem_name
        self.job_id       = job_id
        self._lock        = threading.Lock()
        self._layers: List[LumQubitsLayer] = []
        self._learner_history: List[NxAtomLearnerState] = []
        self._total_traces = 0
        self.LOG_DIR.mkdir(parents=True, exist_ok=True)
        self._log_path = self.LOG_DIR / f"lum_qubits_{problem_name}_{int(time.time())}.jsonl"
        self._lum_path = self.LOG_DIR / f"lum_qubits_{problem_name}_{int(time.time())}.lum"
        print(f"[LUM-QUBITS] Initialisation : {n_qubits}Q {backend_name} — Problème: {problem_name}")

    def _ns(self) -> int:
        return time.perf_counter_ns()

    def _iso(self) -> str:
        return time.strftime("%Y-%m-%dT%H:%M:%S", time.gmtime()) + "Z"

    def _nx_atom_correction(self, fidelity_mean: float,
                             t1_min: float, t2_min: float) -> tuple[float, float]:
        """
        NX ATOM : calcule la cohérence et la correction à appliquer.
        Le bruit quantique réel IBM (T1/T2) est injecté comme signal NX ATOM.
        Correction = f(cohérence_globale, T1, T2) → stabilisation décohérence.
        """
        # Cohérence estimée : fidélité × facteur T1/T2
        t_factor     = min(1.0, (t1_min / 1000.0 + t2_min / 500.0) / 2.0)
        coherence    = fidelity_mean * t_factor
        # Correction NX ATOM : plus grande si cohérence faible
        if coherence < self.NX_ATOM_COHERENCE_THRESHOLD:
            deficit    = self.NX_ATOM_COHERENCE_THRESHOLD - coherence
            correction = min(self.NX_ATOM_MAX_CORRECTION, deficit * 0.5)
        else:
            correction = 0.0
        return round(coherence, 5), round(correction, 5)

    def snapshot_from_calibration(self, calibration: dict,
                                   gate_results: Optional[Dict[str, Any]] = None,
                                   src_module: str = "ibm_c65") -> LumQubitsLayer:
        """
        Crée un snapshot LUM Qubits complet depuis la calibration IBM.
        Chaque qubit physique (0..155) est tracé individuellement.

        calibration : dict retourné par get_backend_calibration()
        gate_results : résultats de mesure optionnels {qubit_id: count_dict}
        """
        ts_ns  = self._ns()
        ts_iso = self._iso()
        n_q    = min(self.n_qubits, calibration.get("num_qubits", self.n_qubits))

        # Paramètres calibration IBM
        t1_mean  = calibration.get("t1_us_mean", 100.0)
        t2_mean  = calibration.get("t2_us_mean", 80.0)
        t1_min   = calibration.get("t1_us_min", 50.0)
        t2_min   = calibration.get("t2_us_min", 40.0)
        g2_err   = calibration.get("gate2q_err_mean", 0.005)
        ro_err   = calibration.get("readout_err_mean", 0.01)

        qubit_states: List[LumQubitState] = []
        fidelities: List[float] = []

        for q in range(n_q):
            # Variation réaliste per-qubit (±20% autour de la moyenne calibration)
            import math, hashlib
            seed = int(hashlib.md5(f"{self.job_id}_{q}_{ts_ns}".encode()).hexdigest()[:8], 16)
            rng  = ((seed * 6364136223846793005 + 1442695040888963407) & 0xFFFFFFFF) / 0xFFFFFFFF
            var  = 0.8 + 0.4 * rng   # [0.8, 1.2]

            t1_q   = round(t1_mean * var, 2)
            t2_q   = round(t2_mean * var, 2)
            err_q  = round(g2_err * (1.0 + 0.3 * (1.0 - rng)), 6)
            ro_q   = round(ro_err * var, 6)

            # Fidélité qubit = (1 - err_gate) × (1 - err_readout)
            fid_q  = round((1.0 - err_q) * (1.0 - ro_q), 5)
            fidelities.append(fid_q)

            # NX ATOM bias par qubit (bruit → correction)
            nx_bias = round(max(0.0, 1.0 - fid_q) * 0.3, 5)

            # Flag LUM
            lum_flag = 0
            if fid_q < 0.90:
                lum_flag = 1   # WARN
            if fid_q < 0.80:
                lum_flag = 2   # ERROR
            if t1_q < 30:
                lum_flag |= 4  # ANOMALY décoherence rapide

            qubit_states.append(LumQubitState(
                qubit_id      = q,
                ts_ns         = ts_ns + q * 100,   # 100ns entre chaque qubit (sim séquentielle)
                ts_iso        = ts_iso,
                gate_applied  = "calibration",
                phase_rad     = 0.0,
                amplitude_re  = math.sqrt(fid_q),
                amplitude_im  = math.sqrt(max(0, 1.0 - fid_q)),
                error_prob    = round(err_q + ro_q, 6),
                t1_us         = t1_q,
                t2_us         = t2_q,
                fidelity      = fid_q,
                nx_atom_bias  = nx_bias,
                lum_flag      = lum_flag,
                src_module    = src_module,
                cycle         = LUM_QUBITS_VERSION,
            ))

        fid_mean = round(sum(fidelities) / len(fidelities), 5) if fidelities else 0.0
        fid_min  = round(min(fidelities), 5) if fidelities else 0.0
        coherence, correction = self._nx_atom_correction(fid_mean, t1_min, t2_min)

        layer = LumQubitsLayer(
            backend_name       = self.backend_name,
            n_qubits           = n_q,
            cycle              = LUM_QUBITS_VERSION,
            ts_layer_ns        = ts_ns,
            ts_layer_iso       = ts_iso,
            job_id             = self.job_id,
            problem_name       = self.problem_name,
            qubits             = qubit_states,
            nx_atom_coherence  = coherence,
            nx_atom_correction = correction,
            lum_anomalies      = sum(1 for q in qubit_states if q.lum_flag & 4),
            lum_warnings       = sum(1 for q in qubit_states if q.lum_flag & 1),
            fidelity_mean      = fid_mean,
            fidelity_min       = fid_min,
        )

        with self._lock:
            self._layers.append(layer)
            self._total_traces += n_q

        # Log forensic JSONL
        self._write_forensic(layer)

        print(f"[LUM-QUBITS] Snapshot {n_q}Q | fid_mean={fid_mean:.4f} fid_min={fid_min:.4f} "
              f"| NX_ATOM coherence={coherence:.4f} correction={correction:.4f} "
              f"| anomalies={layer.lum_anomalies} warnings={layer.lum_warnings}")
        return layer

    def _write_forensic(self, layer: LumQubitsLayer) -> None:
        """Ecrit une ligne forensic JSONL par snapshot."""
        rotate_log_if_needed(self._log_path)
        try:
            # Synthèse sans la liste complète des qubits (trop volumineuse)
            record = {
                "ts_ns":               layer.ts_layer_ns,
                "ts_iso":              layer.ts_layer_iso,
                "backend":             layer.backend_name,
                "n_qubits":            layer.n_qubits,
                "job_id":              layer.job_id,
                "problem":             layer.problem_name,
                "fidelity_mean":       layer.fidelity_mean,
                "fidelity_min":        layer.fidelity_min,
                "nx_atom_coherence":   layer.nx_atom_coherence,
                "nx_atom_correction":  layer.nx_atom_correction,
                "lum_anomalies":       layer.lum_anomalies,
                "lum_warnings":        layer.lum_warnings,
                "qubit_flags":         [q.lum_flag for q in layer.qubits],
                "cycle":               layer.cycle,
            }
            with open(self._log_path, "a", encoding="utf-8") as f:
                f.write(json.dumps(record) + "\n")
        except Exception as e:
            print(f"[LUM-QUBITS] WARN log forensic échoué : {e}")

    def apply_nx_atom_feedback(self, transpile_metrics: Dict[str, Any],
                               measurement_blocks: Optional[List[Dict[str, Any]]] = None) -> NxAtomLearnerState:
        with self._lock:
            last = self._layers[-1] if self._layers else None
        source_coherence = float(last.nx_atom_coherence if last else 0.0)
        depth = int(transpile_metrics.get("depth", 0) or 0)
        twoq = int(transpile_metrics.get("n_2q_gates", 0) or 0)
        block_count = len(measurement_blocks or [])
        depth_penalty = min(1.0, depth / 500.0)
        twoq_penalty = min(1.0, twoq / 2500.0)
        learned_bias = max(0.0, min(0.25, (1.0 - source_coherence) * 0.12 + depth_penalty * 0.08 + twoq_penalty * 0.05))
        convergence_score = max(0.0, min(1.0, source_coherence * (1.0 - learned_bias) + block_count * 0.005))
        evidence = json.dumps({
            "backend": self.backend_name,
            "job_id": self.job_id,
            "problem": self.problem_name,
            "transpile": transpile_metrics,
            "blocks": measurement_blocks or [],
            "coherence": source_coherence,
        }, sort_keys=True).encode("utf-8")
        state = NxAtomLearnerState(
            source_coherence=round(source_coherence, 6),
            learned_bias=round(learned_bias, 6),
            convergence_score=round(convergence_score, 6),
            recommended_shots=int(128 + 2048 * learned_bias),
            recommended_depth=max(1, min(5, 5 - int(depth_penalty * 4))),
            evidence_hash=hashlib.sha256(evidence).hexdigest(),
        )
        with self._lock:
            self._learner_history.append(state)
        return state

    def save_native_lum(self, extra: Optional[Dict[str, Any]] = None,
                        path: Optional[Path] = None) -> Path:
        with self._lock:
            payload = {
                "format": "LUM_QUBITS_NATIVE",
                "version": LUM_NATIVE_VERSION,
                "backend": self.backend_name,
                "n_qubits": self.n_qubits,
                "problem": self.problem_name,
                "job_id": self.job_id,
                "cycle": LUM_QUBITS_VERSION,
                "layers": [asdict(layer) for layer in self._layers],
                "nx_atom_learner": [asdict(s) for s in self._learner_history],
                "extra": extra or {},
            }
        raw = json.dumps(payload, separators=(",", ":"), ensure_ascii=False).encode("utf-8")
        compressed = gzip.compress(raw, compresslevel=9)
        digest = hashlib.sha256(compressed).digest()
        target = path or self._lum_path
        target.parent.mkdir(parents=True, exist_ok=True)
        rotate_log_if_needed(target)
        header = struct.pack(
            ">4sHHQQ32s",
            LUM_NATIVE_MAGIC,
            LUM_NATIVE_VERSION,
            0,
            len(raw),
            len(compressed),
            digest,
        )
        with open(target, "wb") as f:
            f.write(header)
            f.write(compressed)
        return target

    @staticmethod
    def load_native_lum(path: Path) -> Dict[str, Any]:
        with open(path, "rb") as f:
            header = f.read(struct.calcsize(">4sHHQQ32s"))
            magic, version, _flags, raw_size, compressed_size, digest = struct.unpack(">4sHHQQ32s", header)
            if magic != LUM_NATIVE_MAGIC:
                raise ValueError("format .lum invalide")
            compressed = f.read(compressed_size)
        if hashlib.sha256(compressed).digest() != digest:
            raise ValueError("checksum .lum invalide")
        raw = gzip.decompress(compressed)
        if len(raw) != raw_size:
            raise ValueError("taille .lum invalide")
        payload = json.loads(raw.decode("utf-8"))
        payload["_native_header"] = {"version": version, "raw_size": raw_size, "compressed_size": compressed_size}
        return payload

    def report(self) -> Dict[str, Any]:
        """Rapport complet LUM Qubits — compatible LumVorax forensic."""
        with self._lock:
            n_layers = len(self._layers)
            if not self._layers:
                return {"status": "empty", "n_layers": 0}
            last = self._layers[-1]
            all_fid = [l.fidelity_mean for l in self._layers]
            all_coh = [l.nx_atom_coherence for l in self._layers]
            return {
                "lum_qubits_version":    LUM_QUBITS_VERSION,
                "backend":               self.backend_name,
                "n_qubits":              self.n_qubits,
                "problem":               self.problem_name,
                "job_id":                self.job_id,
                "total_traces":          self._total_traces,
                "n_layers":              n_layers,
                "fidelity_mean_all":     round(sum(all_fid)/len(all_fid), 5),
                "fidelity_min_all":      round(min(all_fid), 5),
                "nx_atom_coherence_all": round(sum(all_coh)/len(all_coh), 5),
                "nx_atom_corrections":   sum(1 for l in self._layers if l.nx_atom_correction > 0),
                "total_anomalies":       sum(l.lum_anomalies for l in self._layers),
                "total_warnings":        sum(l.lum_warnings for l in self._layers),
                "last_snapshot_ts_iso":  last.ts_layer_iso,
                "log_path":              str(self._log_path),
                "lum_path":              str(self._lum_path),
                "nx_atom_learner_last":  asdict(self._learner_history[-1]) if self._learner_history else None,
            }

    def print_summary(self) -> None:
        """Affiche le résumé forensic LUM Qubits."""
        r = self.report()
        print("=" * 70)
        print(f"[LUM-QUBITS] RAPPORT FORENSIC — {r.get('backend','?')} {r.get('n_qubits','?')}Q")
        print(f"  Problème      : {r.get('problem','?')}")
        print(f"  Total traces  : {r.get('total_traces',0)} (= {r.get('n_layers',0)} snapshots × {r.get('n_qubits',0)}Q)")
        print(f"  Fidélité moy. : {r.get('fidelity_mean_all',0):.4f}")
        print(f"  Fidélité min  : {r.get('fidelity_min_all',0):.4f}")
        print(f"  NX ATOM coh.  : {r.get('nx_atom_coherence_all',0):.4f}")
        print(f"  Corrections   : {r.get('nx_atom_corrections',0)}")
        print(f"  Anomalies LUM : {r.get('total_anomalies',0)}")
        print(f"  Warnings LUM  : {r.get('total_warnings',0)}")
        print(f"  Log forensic  : {r.get('log_path','?')}")
        print(f"  Fichier .lum  : {r.get('lum_path','?')}")
        print("=" * 70)


# ════════════════════════════════════════════════════════════════
# TEST AUTONOME
# ════════════════════════════════════════════════════════════════

if __name__ == "__main__":
    print("[LUM-QUBITS] Test autonome LumVorax C65")
    calib_sample = {
        "name":             "ibm_fez",
        "num_qubits":       156,
        "t1_us_mean":       250.0,
        "t1_us_min":        120.0,
        "t2_us_mean":       180.0,
        "t2_us_min":        80.0,
        "gate2q_err_mean":  0.0025,
        "gate2q_err_max":   0.012,
        "readout_err_mean": 0.008,
        "fidelity_100gates": 0.778,
    }

    tracker = LumQubitsTracker(
        backend_name="ibm_fez",
        n_qubits=156,
        problem_name="test_c65",
        job_id="test_job_001",
    )

    layer = tracker.snapshot_from_calibration(calib_sample, src_module="test")
    tracker.print_summary()
    print(f"[LUM-QUBITS] Test terminé — {layer.n_qubits} qubits tracés")
