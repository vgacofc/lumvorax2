# Correctif Kaggle Notebook V6 — Problème `__file__` non défini

## Problème rencontré
Dans Kaggle Notebook (exécution Papermill/Jupyter), la variable `__file__` n’existe pas dans une cellule Python.
Ancien comportement: `ROOT = Path(__file__).resolve().parent` provoquait:
- `NameError: name '__file__' is not defined`

## Cause
Un script Python classique peut compter sur `__file__`, mais une cellule notebook non.

## Solution implémentée
Le kernel V6 utilise désormais `resolve_root()` avec fallback robuste:
1. `explicit_root` argument,
2. variable d’environnement `NQUBIT_V6_ROOT`,
3. `__file__` si disponible,
4. heuristiques `Path.cwd()` + chemins Kaggle standards.

Conséquence:
- compatibilité script **et** notebook,
- exécution possible en **une seule cellule** (copier-coller du kernel `.py`).

## Vérification
- Test unitaire ajouté `tests/test_kernel_v6.py`:
  - run normal,
  - simulation mode notebook via appel `main(<root>)` sans dépendance `__file__`.

## Commande de validation
```bash
cd 'src/projetx_NQubit NX/NQubit_v6'
make test_unit
```
