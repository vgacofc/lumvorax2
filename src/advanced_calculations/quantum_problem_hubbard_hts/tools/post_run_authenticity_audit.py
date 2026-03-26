#!/usr/bin/env python3
import csv
import re
import sys
from pathlib import Path

SCAN_EXTS = {".c", ".h", ".py", ".sh"}
PATTERNS = [
    ("TODO", re.compile(r"\bTODO\b", re.IGNORECASE), "warning"),
    ("FIXME", re.compile(r"\bFIXME\b", re.IGNORECASE), "warning"),
    ("PLACEHOLDER", re.compile(r"\bPLACEHOLDER\b", re.IGNORECASE), "risk"),
    ("STUB", re.compile(r"\bSTUB\b|\bstub\b", re.IGNORECASE), "risk"),
    ("MOCK", re.compile(r"\bmock\b", re.IGNORECASE), "risk"),
    # Detect concrete hardcoded runtime/model markers, avoid meta self-references.
    ("HARDCODING", re.compile(r"advanced_fullscale_deterministic|periodic_fullscale|problem_t probs\[", re.IGNORECASE), "review"),
]

EXCLUDED_FILES = {
    "post_run_authenticity_audit.py",
    "inspect_quantum_simulator_stacks.py",
}


def is_comment_only(line: str) -> bool:
    s = line.strip()
    return s.startswith("#") or s.startswith("//") or s.startswith("*")


def write_csv(path: Path, header, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(header)
        w.writerows(rows)


def main():
    if len(sys.argv) != 3:
        print("Usage: post_run_authenticity_audit.py <module_root> <run_dir>", file=sys.stderr)
        return 2

    module_root = Path(sys.argv[1]).resolve()
    run_dir = Path(sys.argv[2]).resolve()

    files = []
    for p in module_root.rglob("*"):
        if not p.is_file() or p.suffix not in SCAN_EXTS:
            continue
        s = str(p)
        if "/results/" in s or "/backups/" in s:
            continue
        if p.name in EXCLUDED_FILES:
            continue
        in_code_scope = any(token in s for token in ["/tools/", "/src/", "/include/"]) or p.name.startswith("run_")
        if not in_code_scope:
            continue
        files.append(p)

    findings = []
    hardcoding = []
    for p in files:
        try:
            txt = p.read_text(errors="ignore")
        except Exception:
            continue
        lines = txt.splitlines()
        for i, line in enumerate(lines, start=1):
            if is_comment_only(line):
                continue
            for name, rx, sev in PATTERNS:
                if rx.search(line):
                    rel = p.relative_to(module_root)
                    findings.append([name, sev, str(rel), i, line.strip()[:240]])
                    if name == "HARDCODING":
                        hardcoding.append([str(rel), i, line.strip()[:240], "REVIEW_REQUIRED"])

    audit_path = run_dir / "tests" / "integration_code_authenticity_audit.csv"
    write_csv(audit_path, ["pattern", "severity", "file", "line", "snippet"], findings)

    hard_path = run_dir / "tests" / "integration_hardcoding_risk_register.csv"
    write_csv(hard_path, ["file", "line", "snippet", "status"], hardcoding)

    # command-by-command traceability template
    cmd_path = run_dir / "logs" / "process_trace_commands_history.md"
    cmd_path.write_text(
        "# Process Trace — Command-by-command\n\n"
        "## Pré-exécution locale\n"
        "1. `git fetch origin --prune`\n"
        "2. `bash src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh`\n"
        "3. `python3 src/advanced_calculations/quantum_problem_hubbard_hts/tools/post_run_authenticity_audit.py src/advanced_calculations/quantum_problem_hubbard_hts <RUN_DIR>`\n"
        "\n## Vérifications de validation\n"
        "4. `cat <RUN_DIR>/tests/integration_gate_summary.csv`\n"
        "5. `cat <RUN_DIR>/tests/integration_physics_gate_summary.csv`\n"
        "6. `cat <RUN_DIR>/tests/integration_code_authenticity_audit.csv`\n"
        "7. `cat <RUN_DIR>/tests/integration_hardcoding_risk_register.csv`\n"
        "8. `sha256sum -c <RUN_DIR>/logs/checksums.sha256`\n"
    )

    q_path = run_dir / "tests" / "integration_questions_traceability.csv"
    q_rows = [
        ["Q1", "Le résultat est-il reproductible inter-run ?", "integration_run_drift_monitor.csv", "AUTO"],
        ["Q2", "Les métadonnées physiques sont-elles complètes ?", "integration_physics_gate_summary.csv", "AUTO"],
        ["Q3", "Des placeholders/stubs/hardcoding compromettent-ils l'authenticité ?", "integration_code_authenticity_audit.csv", "AUTO"],
        ["Q4", "Les observables avancées manquantes sont-elles tracées ?", "cycle21_counter_response_matrix_*.csv", "MANUAL_REVIEW"],
    ]
    write_csv(q_path, ["question_id", "question", "evidence_file", "validation_mode"], q_rows)

    print(f"[auth-audit] generated: {audit_path}")
    print(f"[auth-audit] generated: {hard_path}")
    print(f"[auth-audit] generated: {cmd_path}")
    print(f"[auth-audit] generated: {q_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
