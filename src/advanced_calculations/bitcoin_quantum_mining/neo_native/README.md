# PROJET A : NEO WRAPPER — Solution GPU Immédiate

## 🎯 Objectif

Wrapper Intel NEO (runtime OpenCL) avec logging bit-level LumVorax pour Bitcoin mining stable sur Intel UHD 620 Gen9.

## 📊 Statut : EN DÉVELOPPEMENT

**Timeline** : 2 semaines  
**Priorité** : HAUTE (Production ready)

## 🏗️ Architecture

```
btc_neo_wrapper.c (Notre code)
    ↓ Logging forensique bit-level
Intel NEO Runtime (libOpenCL.so)
    ↓
Kernel i915 Gen9
    ↓
Intel UHD 620 GPU
```

## 📁 Structure

```
neo_native/
├── src/
│   ├── btc_neo_wrapper.c      # Wrapper principal
│   ├── btc_neo_wrapper.h      # API publique
│   └── btc_neo_logger.c       # Logging bit-level
├── tests/
│   ├── test_neo_basic.c       # Tests basiques
│   └── test_neo_10k.c         # Tests 10000 dispatches
├── docs/
│   ├── API.md                 # Documentation API
│   └── INTEGRATION.md         # Guide intégration
└── Makefile
```

## 🚀 Prochaines Étapes

### Semaine 1
- [ ] Implémenter `btc_neo_wrapper.c`
- [ ] Intégrer `btc_gpu_async_logger`
- [ ] Tests basiques

### Semaine 2
- [ ] Tests 10000 dispatches
- [ ] Documentation
- [ ] **PRODUCTION READY**

## 📝 Notes

- Utilise NEO comme dépendance native (déjà installé)
- Traçabilité bit-level via notre logging
- Solution stable immédiate