# RAPPORT — INTÉGRATION DES 9 CHEMINS MODÈLES FOURNIS
Date: 2026-02-24

## Entrées ajoutées dans NX46 VX V1
Les 9 chemins fournis ont été injectés dans `RAPPORT-VESUVIUS/NX46-VX/nx46-vx-unified-kaggle-V1.ipynb` dans:
- `USER_PROVIDED_MODEL_INPUT_PATHS`
- `TEACHER_MODELS_REGISTRY` (teacher_01..teacher_09)

### Liste exacte intégrée
1. `/kaggle/input/models/ipythonx/vsd-model/keras/segformer.mit.b2/1`
2. `/kaggle/input/models/ipythonx/vsd-model/keras/default/1`
3. `/kaggle/input/models/ipythonx/vsd-model/keras/transunetseresnext/1`
4. `/kaggle/input/models/ipythonx/vsd-model/keras/segformer.mit.b4/1`
5. `/kaggle/input/models/ipythonx/vsd-model/keras/segformer.mit.b2/2`
6. `/kaggle/input/models/ipythonx/vsd-model/keras/transunetseresnext/2`
7. `/kaggle/input/models/ipythonx/vsd-model/keras/default/2`
8. `/kaggle/input/models/ipythonx/vsd-model/keras/transunet/2`
9. `/kaggle/input/models/ipythonx/vsd-model/keras/transunet/3`

## Installer offline ajouté
- `OFFLINE_INSTALLER_INPUT = /kaggle/input/notebooks/ipythonx/vsdetection-packages-offline-installer-only`

## Découverte automatique runtime
Le notebook V1 inclut maintenant `discover_teacher_model_paths(...)` qui vérifie automatiquement l’existence de ces chemins à l’exécution Kaggle pour les retrouver comme les autres inputs.
