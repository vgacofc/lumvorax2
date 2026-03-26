# Prototype NQubit NX

Ce dossier contient un prototype C autonome pour simuler un NQubit NX base sur le bruit de jonction transistor.

## Contenu
- `nqbit_nx.h/.c` : modele de simulation chaos guide par Lyapunov.
- `nx_forensic.h/.c` : journalisation forensique nanoseconde.
- `main.c` : execute 360 scenarios, detecte CPU/RAM, exporte rapports.
- `tests/test_nqbit_nx.c` : suite unitaire 360 cas.
- `results/` : logs, CSV et rapport final generes.

## Build
```bash
make clean && make all && make test && make run
```

## Sorties
- `results/nqbit_forensic_ns.log`
- `results/nqbit_benchmark.csv`
- `results/rapport_final.md`
