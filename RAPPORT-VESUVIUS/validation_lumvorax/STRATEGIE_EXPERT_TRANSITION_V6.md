# Plan de Correction et de Transition LUMVORAX (Expert)

## 1. Analyse des Anomalies et État des Lieux
L'échec de compilation (`physics/kerr_metric.h` manquant) est dû à une structure de dossiers incohérente. Le fichier existe dans `trou_noir_sim/physics/kerr_metric.h` mais le `Makefile` cherche dans `src/`.
**Correction Immédiate :** Créer un lien symbolique ou copier le dossier `physics` dans `src/`.

## 2. Stratégie Kaggle : Pourquoi le .whl est la solution
Exécuter du `.c` et `.h` directement dans Kaggle est possible via `gcc`, mais c'est **inefficace** et **fragile** pour des compétitions à itérations rapides.
- **Le problème des stubs :** L'agent précédent a tenté d'injecter des "faux" fichiers C, ce qui corrompt la validation.
- **La solution .whl :** Transformer le système LUMVORAX en une bibliothèque Python compilée (`.so` via `setuptools` ou `scikit-build`).
  - Permet d'importer `import lumvorax` directement.
  - Cache la complexité de compilation.
  - Compatible avec le système de "Dataset" Kaggle comme dépendance binaire.

## 3. Réponses aux Questions Expert
- **Exécuter du .c/.h sur Kaggle ?** Oui, mais l'environnement est "stateless" à chaque run. Compiler à chaque fois perd 30-60s.
- **Pourquoi pas .whl ?** C'est la recommandation standard. Un `Dataset` Kaggle peut contenir des fichiers `.whl` que l'on installe via `pip install /kaggle/input/mon-dataset/package.whl`.
- **Besoin des .c/.h si on a le .so ?** Non, si les interfaces (bindings) sont stables. On garde les sources pour le développement, mais on distribue le binaire.

## 4. Nouveau Prompt pour l'Agent (PROMPT_EXPERT_LUMVORAX_V6.md)
Le nouveau prompt doit se concentrer sur la **production d'un bundle binaire certifié** plutôt que sur la compilation à la volée.

## 5. Actions Immédiates (Build Mode)
1. Fix du header `kerr_metric.h`.
2. Préparation du script `produce_wheels_v5.py`.
3. Mise à jour du rapport de certification V5.
