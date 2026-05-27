# 🔬 RAPPORT C443 - ANALYSE ORDRE COMMANDES BATCH BUFFER

**Session**: C443  
**Date**: 2026-05-23 17:30 UTC  
**Analyste**: Bob (Advanced Mode)  
**Problème**: Output reste 0x00000000 malgré MEDIA_VFE_STATE + MEDIA_INTERFACE_DESCRIPTOR_LOAD

---

## 📋 DIAGNOSTIC

### Séquence OpenCL (FONCTIONNE ✅)
```
1. PIPELINE_SELECT
2. MEDIA_VFE_STATE
3. STATE_BASE_ADDRESS
4. MEDIA_CURBE_LOAD
5. MEDIA_INTERFACE_DESCRIPTOR_LOAD
6. GPGPU_WALKER
```

### Séquence i915 Native C442 (ÉCHOUE ❌)
```
1. PIPELINE_SELECT
2. MEDIA_VFE_STATE        ← Ajouté C442
3. STATE_BASE_ADDRESS
4. MEDIA_VFE_STATE        ← DUPLIQUÉ! (lignes 355-364)
5. MEDIA_CURBE_LOAD
6. MEDIA_STATE_FLUSH
7. MEDIA_INTERFACE_DESCRIPTOR_LOAD
8. GPGPU_WALKER
```

## 🎯 PROBLÈME IDENTIFIÉ

**MEDIA_VFE_STATE EST PROGRAMMÉ 2 FOIS!**

1. **Ligne 288-327**: MEDIA_VFE_STATE ajouté C442 (CORRECT)
2. **Ligne 355-364**: MEDIA_VFE_STATE ancien code (INCORRECT - doit être supprimé!)

**CONSÉQUENCE**: Le 2ème MEDIA_VFE_STATE écrase le 1er avec des valeurs incorrectes!

### Comparaison Valeurs

**C442 (CORRECT)**:
```c
batch[offset++] = (168 << 0) | (64 << 16);  // Max threads=168, URB=64
batch[offset++] = (1 << 0) | (2 << 16);     // CURBE=1 block, URB=2 blocks
```

**Ancien code ligne 359-361 (INCORRECT)**:
```c
batch[offset++] = (24 << 16) | (24 << 8);   // ??? Valeurs incorrectes
batch[offset++] = 1;                         // CURBE=1 (correct)
```

## 🔧 SOLUTION C443

**Supprimer lignes 355-364** (ancien MEDIA_VFE_STATE dupliqué)

---

**FIN RAPPORT C443**