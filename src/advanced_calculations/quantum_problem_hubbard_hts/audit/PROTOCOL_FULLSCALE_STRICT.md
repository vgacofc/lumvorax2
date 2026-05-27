# Protocole Full-Scale Strict — Implémentation locale

Ce module implémente une base exécutable pour:
- preuve de pipeline (`formal_pipeline_proof/specs`)
- détection anti-proxy (`tools/fullscale_strict_protocol_runner.py`)
- validation inter-centres + consensus (hash majoritaire >= 2/3)
- audit cryptographique (`run_signature.json`, hashes code/deps/results)
- rapport final (`audit/final_scientific_audit.md`)

Exécution:
```bash
./run_fullscale_strict_protocol.sh <run_dir>
```
