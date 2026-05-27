# RAPPORT C277 — TESTS HYPOTHÈSES ET RÉSULTATS
## Validation Empirique Format Intel VM

**Date**: 2026-05-18 20:50  
**Objectif**: Tester toutes les hypothèses immédiatement  
**Statut**: Tests partiels effectués, conclusions importantes

---

## 1. TESTS EFFECTUÉS

### Test 1: Création/Destruction Contextes (test_vm_format.c)

**Hypothèse**: Format Intel avec VM permet plus de créations/destructions que format simple.

**Configuration**:
- Test A: 10 créations/destructions avec `DRM_IOCTL_I915_GEM_CONTEXT_CREATE` (simple)
- Test B: 10 créations/destructions avec `DRM_IOCTL_I915_GEM_VM_CREATE` + `CREATE_EXT` + `SETPARAM`

**Résultats**:
```
Format SIMPLE:  10/10 créations réussies (100%)
Format INTEL:   10/10 créations réussies (100%)
```

**Conclusion**: ❌ **HYPOTHÈSE RÉFUTÉE**
- Les deux formats fonctionnent parfaitement pour création/destruction
- Pas de limitation détectée à 10 itérations
- Le problème N'EST PAS dans la création/destruction de contextes

### Test 2: Réutilisation Contexte (test_context_reuse.c)

**Hypothèse**: Format Intel avec VM permet plus de réutilisations du même contexte.

**Configuration**:
- Test A: 30 dispatches avec MÊME contexte simple
- Test B: 30 dispatches avec MÊME contexte + VM

**Résultats**:
```
Format SIMPLE:  0/30 dispatches (errno=22 Invalid argument)
Format INTEL:   0/30 dispatches (errno=22 Invalid argument)
```

**Conclusion**: ⚠️ **TEST INVALIDE**
- Batch buffer invalide (errno=22)
- Problème technique dans construction batch
- Test ne peut pas valider/réfuter l'hypothèse

**Cause technique**:
- Utilisation `DRM_I915_GEM_MMAP_OFFSET` au lieu de `DRM_I915_GEM_MMAP`
- Batch minimal (MI_BATCH_BUFFER_END seul) peut être insuffisant
- Nécessite batch plus complet comme dans Bitcoin miner

---

## 2. ANALYSE RÉSULTATS

### 2.1 Découverte Majeure

**Le problème N'EST PAS dans la création/destruction de contextes.**

**Preuves**:
1. ✅ Format simple: 10 créations/destructions OK
2. ✅ Format Intel VM: 10 créations/destructions OK
3. ✅ Aucune limitation détectée

**Implication**: L'hypothèse initiale "VM élimine limitation 3 utilisations" est **PARTIELLEMENT FAUSSE** pour le cas création/destruction.

### 2.2 Hypothèse Révisée

**Le problème est probablement dans la RÉUTILISATION du même contexte pour EXECBUFFER2, pas dans la création/destruction.**

**Différence critique**:
- **Création/Destruction**: Créer contexte → Détruire → Créer nouveau → OK ∞
- **Réutilisation**: Créer contexte → Dispatch 1 → Dispatch 2 → ... → Dispatch N → Limite ?

**Bitcoin miner fait**:
```
Contexte 0: Dispatch 1, 10, 19, 28 (4 utilisations)
                                 ↑
                              FAIL ici
```

### 2.3 Pourquoi Test 2 A Échoué

**Problème technique**: Batch buffer invalide (errno=22)

**Causes possibles**:
1. API mmap incorrecte (MMAP_OFFSET vs MMAP)
2. Batch trop minimal (juste MI_BATCH_BUFFER_END)
3. Flags EXECBUFFER2 manquants
4. Contexte pas correctement configuré

**Solution**: Utiliser code existant qui fonctionne (test_j ou Bitcoin miner).

---

## 3. ANALYSE LOGS BITCOIN MINER (Rappel)

### 3.1 Pattern Observé

**Logs réels** (gen9_native_production.log):
```
Dispatch 1-9:   Contextes 0-8 (1ère utilisation) → OK
Dispatch 10-18: Contextes 0-8 (2ème utilisation) → OK
Dispatch 19-27: Contextes 0-8 (3ème utilisation) → OK
Dispatch 28:    Contexte 0 (4ème utilisation) → FAIL (errno=5)
```

**Formule validée**:
```
max_dispatches = nb_contexts × MAX_REUSE_PER_CTX
               = 9 × 3
               = 27 dispatches
```

### 3.2 Erreur Au Dispatch 28

**Log ligne 472** (gen9_native_production.log):
```
[3163.686222461] DRM_REOPEN_CTX_CREATE_FAILED: index=0 errno=5
```

**Contexte**:
- Tentative de RECRÉER contexte 0 (après 3 utilisations)
- Échec AVANT premier dispatch du nouveau contexte
- errno=5 (EIO - Input/output error)

**Interprétation**:
- Sans VM, ressources contexte précédent pas libérées
- i915 refuse création nouveau contexte
- Problème dans CRÉATION après réutilisations, pas dans réutilisation elle-même

---

## 4. HYPOTHÈSE FINALE RÉVISÉE

### 4.1 Scénario Réel

**Sans VM**:
```
1. Créer contexte 0
2. Dispatch 1 sur contexte 0 → OK
3. Dispatch 2 sur contexte 0 → OK
4. Dispatch 3 sur contexte 0 → OK
5. Détruire contexte 0
6. Créer nouveau contexte 0 → FAIL (errno=5)
   ↑
   Ressources pas libérées, i915 refuse
```

**Avec VM**:
```
1. Créer VM 0
2. Créer contexte 0 (associé VM 0)
3. Dispatch 1 sur contexte 0 → OK
4. Dispatch 2 sur contexte 0 → OK
5. Dispatch 3 sur contexte 0 → OK
6. Détruire contexte 0
7. Détruire VM 0 → Nettoyage complet
8. Créer nouvelle VM 0
9. Créer nouveau contexte 0 → OK
   ↑
   Ressources libérées, i915 accepte
```

### 4.2 Pourquoi Test 1 N'A Pas Détecté Le Problème

**Test 1 faisait**: Créer → Détruire → Créer → Détruire (10 fois)

**Mais ne faisait PAS**: Créer → Dispatch → Dispatch → Dispatch → Détruire → Créer

**Différence critique**: Test 1 ne faisait AUCUN dispatch, donc pas de "pollution" des ressources.

### 4.3 Test Manquant

**Ce qu'il faut tester**:
```c
for (int cycle = 0; cycle < 10; cycle++) {
    // Créer contexte
    ctx_id = create_context();
    
    // Faire 3 dispatches (limite observée)
    for (int i = 0; i < 3; i++) {
        execbuffer2(ctx_id, batch);
    }
    
    // Détruire contexte
    destroy_context(ctx_id);
    
    // Tenter de recréer → Devrait échouer au cycle 10 sans VM
}
```

---

## 5. CONCLUSIONS

### 5.1 Tests Validés

1. ✅ **Test création/destruction**: Les deux formats fonctionnent
2. ⚠️ **Test réutilisation**: Invalide (problème technique batch)
3. ❌ **Test cycle complet**: Pas effectué

### 5.2 Hypothèses

**Hypothèse initiale**: "VM élimine limitation 3 utilisations"
- ❌ Partiellement fausse pour création/destruction simple
- ⚠️ Non testée pour cycle complet (créer → dispatcher × 3 → détruire → recréer)

**Hypothèse révisée**: "VM permet nettoyage complet ressources après destruction contexte"
- ✅ Cohérent avec logs Bitcoin miner
- ⚠️ Non testée empiriquement
- 📊 Nécessite test cycle complet

### 5.3 Prochaines Étapes

**Option A**: Corriger test_context_reuse.c
- Utiliser DRM_I915_GEM_MMAP au lieu de MMAP_OFFSET
- Copier batch du Bitcoin miner (72 commandes)
- Tester cycle complet (créer → dispatch × 3 → détruire × 10)

**Option B**: Modifier directement Bitcoin miner
- Implémenter format Intel avec VM
- Tester 100 dispatches
- Mesurer hashrate

**Option C**: Analyser test_j existant
- Corriger pour utiliser renderD128
- Comparer comportement avec/sans VM
- Valider hypothèse empiriquement

**Recommandation**: **Option B** (modification Bitcoin miner)
- Plus direct
- Teste système réel
- Valide solution complète

---

## 6. LEÇONS APPRISES

### 6.1 Tests Unitaires vs Système Réel

**Leçon**: Tests unitaires simples ne reproduisent pas toujours le problème réel.

**Exemple**:
- Test création/destruction: OK
- Système Bitcoin miner: FAIL au dispatch 28

**Raison**: Système réel a pattern d'utilisation complexe (rotation contextes + dispatches multiples).

### 6.2 Importance Logs Production

**Leçon**: Logs production contiennent la vérité absolue.

**Exemple**:
- Hypothèse: "25 états GPU manquants"
- Logs: "27 dispatches OK, aucun GPU hang"
- Conclusion: Hypothèse réfutée

### 6.3 Reverse Engineering Efficace

**Leçon**: strace OpenCL révèle format Intel correct.

**Découverte**:
- VM_CREATE + CONTEXT_CREATE_EXT + SETPARAM
- Format non documenté
- Essentiel pour fonctionnement correct

---

## 7. RÉSUMÉ EXÉCUTIF

### Tests Effectués
- ✅ Test 1: Création/destruction contextes (2 formats)
- ⚠️ Test 2: Réutilisation contexte (invalide)

### Résultats
- Format simple: 10/10 créations OK
- Format Intel VM: 10/10 créations OK
- Réutilisation: 0/30 (problème technique)

### Conclusions
- Problème N'EST PAS dans création/destruction simple
- Problème PROBABLEMENT dans cycle complet (créer → dispatcher × N → détruire → recréer)
- VM permet nettoyage complet ressources (hypothèse non testée)

### Recommandation
**Implémenter format Intel dans Bitcoin miner et tester directement.**

---

**FIN RAPPORT C277 — TESTS HYPOTHÈSES ET RÉSULTATS**

**Statut**: Tests partiels, hypothèse révisée, implémentation recommandée  
**Prochaine action**: Modifier btc_gen9_native_runner.c avec format Intel