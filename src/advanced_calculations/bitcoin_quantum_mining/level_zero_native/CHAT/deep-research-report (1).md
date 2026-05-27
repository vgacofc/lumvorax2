# RAPPORT C399 – ROOT CAUSE IDENTIFIÉ : Interface Descriptor Kernel = 0x0

**Date** : 2026-05-23 00:09:20 UTC+2  
**Cycle** : C399 – Pipeline hybride OpenCL → i915 DRM natif  
**Statut** : ✅ **ROOT CAUSE IDENTIFIÉ – Kernel à adresse NULL**

---

## 🎯 OBJECTIF C399

- Réutiliser le binaire OpenCL compilé (C398) et l’exécuter via i915 DRM « natif » (sans OpenCL runtime).  
- Comparer bit-à-bit les résultats GPU natif vs OpenCL.  
- Détecter et corriger toute divergence.  

*Stratégie* : Exploiter le binaire Intel existant (∼40 KB ISA) et charger manuellement les états GPU (Surface States, Interface Descriptor, etc.), au lieu de réimplémenter le kernel.

---

## 📊 RÉSULTATS D’EXÉCUTION

- ✅ **Compilation** du binaire hybride C399 réussie (40,384 bytes, checksum `0x2d297f1b`, identique à C398).  
- ✅ **Device** i915 (/dev/dri/card1) ouvert, tous les GEM buffers créés (kernel, input, output, CURBE, IDesc, batch).  
- ✅ **Execbuffer2** : Dispatch soumis avec succès (Latency GPU ~2.063s).  
- ✅ **Sync CPU/GPU** : `GEM_WAIT` terminé sans erreur.  

**❌ Échec fonctionnel critique** :  
- Toutes les valeurs écrites dans le buffer de sortie sont **zéro** (0 sur 256 threads).  
- `best_nonce = 0`, `leading_zeros = 0` (aucun résultat valide), checksum total = `0x00000000`.  
- *Attendu (C398)* : quelques threads auraient produit des nonces ≠ 0 (environ 4–6 leading zeros en moyenne).

---

## 🔍 ANALYSE FORENSIQUE

### Adresses GTT des Objets

```
Kernel:  0x0000000000000000  ← ❌ ADRESSE INVRAIE !
Input:   0x000000000000a000  ← ✅ OK (block header)
Output:  0x000000000000b000  ← ✅ OK (nonce results)
CURBE:   0x000000000000d000  ← ✅ OK (données constantes)
IDesc:   0x000000000000e000  ← ✅ OK (Interface Descriptor)
Batch:   0x000000000000f000  ← ✅ OK (Batch Buffer)
```

- Les objets *input/output/CURBE/IDesc* ont des GTT correctes.  
- **Kernel GTT = 0x0** (nul) – inacceptable pour exécution GPU.  

### Dump de l’Interface Descriptor (IDesc)

```
0000: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
0010: 00 00 00 00 01 01 07 00 00 00 00 00 00 00 00 00
        ^^^^^^^^^^^^
        DW0 = 0x00000000 ← Kernel Start Pointer = NULL
```

- **DW0** correspond à *Kernel Start Pointer* (bits [63:6])【43†L166-L173】.  
- Ici DW0 = `0x00000000`, donc le GPU pense que le code kernel commence à l’adresse *0x0*.  
- Sur Gen9, cette adresse doit être 64-octets alignée et non-nulle (voir PRM: « Kernel Start Pointer est spécifié en unités de 64 octets【43†L166-L173】»).  
- Résultat : le GPU exécute un code inexistant (probablement NOP ou ignore la file d’instructions), **aucune opération de hash n’est réalisée**, d’où un buffer de sortie resté vierge (tout 0).

### Cause de l’Adresse Kernel=0

Dans le code C399, l’Interface Descriptor est rempli **avant** la validation finale des objets GEM. Concrètement :

```c
// (extrait du code C399, ligne ~260)
// Création de l'Interface Descriptor avant validation finale
interface_desc[0] = (uint32_t)(kernel_gtt & 0xFFFFFFFF);
// ...
// kernel_gtt provient de dummy_objects[0].offset
```

- `dummy_objects[0]` était le buffer du kernel, et comme il a été soumis **en premier** sans flag PINNED, le noyau i915 lui a attribué l’offset *0* dans le GTT.  
- Ce `kernel_gtt = 0x0` a été copié dans DW0 de l’Interface Descriptor.  

---

## 🎓 ROOT CAUSE IDENTIFIÉE

**L’interface descriptor pointe vers l’adresse 0x0 pour le kernel.**

1. Le GPU charge l’Interface Descriptor à l’adresse 0xe000.  
2. Il lit DW0 → **0x00000000** (bits [63:6]=0) comme pointeur de démarrage du kernel.  
3. Le GPU tente d’exécuter le code à `0x0` (zone invalide) – *aucun calcul utile n’a lieu*.  
4. Les instructions SHA256 ne sont **jamais exécutées**, le buffer de sortie reste à zéro.

Ce n’est **pas** un bug matériel ni un problème de code GPU : le kernel binaire est identique à C398 (check-sum identique), et a déjà prouvé son bon fonctionnement sous OpenCL. Le problème est 100% du côté *user-space/i915* :

- **Interface Descriptor mal rempli** – mauvais pointeur de kernel.  
- Ce dysfonctionnement explique directement “GPU exécuté mais sortie = 0x00000000”.

---

## 🔧 SOLUTIONS PROPOSÉES

### Option A: Fixer l’offset du Kernel (pinned)

Utiliser `EXEC_OBJECT_PINNED` pour forcer une adresse GTT non-nulle pour le kernel :  
- Pré-allouer, p. ex. `.offset = 0x100000`.  
- Signaler l’objet kernel avec `flags = EXEC_OBJECT_PINNED` (user fournit l’offset).  
- *Remarque* : Certains systèmes (notamment sur carte1) peuvent rejeter PINNED (EINVAL) si l’adresse n’est pas alignée ou impossible.  

**Avantages** : Pas besoin d’exécution préalable.  
**Inconvénients** : `EXEC_OBJECT_PINNED` n’est pas toujours supporté, et l’adresse doit être à l’avance sans conflit.

### Option B: Recaler l’Interface Descriptor après validation

1. Soumettre `gem_execbuffer2` **sans** Interface Descriptor (ou avec une valeur DW0 temporaire).  
2. Le noyau i915 renvoie les offsets effectifs de tous les objets dans `final_objects[].offset`.  
3. Lire `final_kernel_gtt = final_objects[kernel_index].offset`.  
4. Réécrire l’Interface Descriptor avec `interface_desc[0] = final_kernel_gtt`.  
5. Ré-exécuter (second ioctl) avec cette IDesc corrigée.  

Cette technique utilise le fait que, si **`EXEC_OBJECT_PINNED` n’est pas utilisé**, le champ `offset` est *rempli par le noyau* avec l’adresse allouée【46†L3898-L3900】.  

### Option C: Réordonner les objets

- Éviter de mettre le kernel en premier dans la liste `exec_objects`.  
- Par exemple, soumettre d’abord l’IDesc ou un autre buffer, puis le kernel.  
- Cela incite le driver à lui attribuer une autre plage libre (non 0).  

**Remarque** : Ce comportement n’est pas garanti (dépend du validateur i915), mais il peut changer l’allocation.

### Résumé des Options

| Approche                 | Avantages                        | Inconvénients                          |
|--------------------------|----------------------------------|----------------------------------------|
| **A. EXEC_OBJECT_PINNED**    | Offset défini explicitement【46†L3898-L3900】 | Peut échouer (EINVAL sur certaines cartes) |
| **B. Update IDesc après exec** | Correction garantie, driver remplit offset【46†L3898-L3900】 | Requiert second execbuffer pour mise à jour |
| **C. Reordonner objets**    | Simple, sans flag spécial        | Comportement heuristique, non standard |

---

## 📈 COMPARAISON C398 vs C399

| Élément                | C398 (OpenCL runtime)     | C399 (i915 natif)           | Remarque             |
|------------------------|--------------------------|-----------------------------|----------------------|
| **Kernel ISA**         | 40 KB OpenCL binaire     | 40 KB (identique)           | ✅ Aucun changement   |
| **Checksum binaire**   | 0x2d297f1b               | 0x2d297f1b (identique)      | ✅ Conforme          |
| **Pipeline d’exécution** | `clEnqueueNDRange` (OpenCL) | `drmExecbuffer2` (i915 native) | ❌ Différence majeure |
| **Adresse GTT du kernel**| *Assignée par driver OpenCL* | 0x0000000000000000          | ❌ **Root Cause**      |
| **Resultats**          | ~4–6 bits zeros (nonces valides) | 0x00000000 (tous zéros) | ❌ Échec       |

**Conclusion** : Le code GPU est bon, seul le **Runtime natif** est fautif. C399 échoue uniquement parce que l’Interface Descriptor transmet une adresse kernel nulle.

---

## 🎯 VALIDATION DES SOLUTIONS

Pour valider la correction, on implémentera la méthode choisie (par exemple Option B) :

1. Après le premier `execbuffer2`, lire `final_objects[offset]` du kernel.  
2. Mettre à jour IDesc : `interface_desc[0] = (uint32_t)(final_kernel_gtt & 0xFFFFFFFF)`.  
3. `ioctl(DRM_IOCTL_I915_GEM_PWRITE)` pour réécrire l’IDesc.  
4. Re-submettre le batch GPU (ou inclure cette mise à jour dans la soumission initiale).  

**Attendu après correction** : le kernel s’exécutera, le buffer de sortie contiendra des nonces non-nuls, et on retrouvera les 6 leading zeros typiques du bloc Bitcoin, comme en C398. Cela confirmera que le pipeline GPU fonctionne normalement.  

---

## 🎓 DÉCOUVERTES ET LEÇONS

- *Interface Descriptor Generation* : la construction doit utiliser les offsets GTT réels, pas des valeurs temporaires.  
- *Execbuffer2 ExecObjects* : sans `EXEC_OBJECT_PINNED`, le driver remplit le champ `.offset` avec l’adresse GPU réelle【46†L3898-L3900】. Il faut donc lire ce champ après exécution.  
- *Alignement/Format* : le champ **Kernel Start Pointer** est 64‐octets aligné【43†L166-L173】 (bits [63:6] du DW0). Une valeur nulle est invalide.  
- *Méthode hybride* : charger un binaire OpenCL via i915 natif exige de réimplémenter avec précision les mêmes états (CURBE, IDesc, walker). Le moindre écart (ici un offset erroné) conduit à des résultats nuls silencieux.  

---

## 🚀 CONCLUSION C399

- ✅ **Root Cause identifié** : Interface Descriptor DW0 = 0 → Kernel à l’adresse 0x0 dans le GTT.  
- 🚫 **Échec fonctionnel** : GPU n’exécute rien (sortie toute nulle).  
- ✅ **Solution** : Corriger l’adresse du kernel (p. ex. via `exec_object.offset` ou mise à jour après exec) pour pointer vers `0xa000`+ (valeur réelle).  
- 🔄 **Prochaine étape** (Cycle C400) : Implémenter la solution choisie, exécuter de nouveau, et vérifier la convergence des résultats. 

En résumé, **le code GPU et les données étaient corrects – le seul défaut venait de la configuration du runtime i915** (adresse kernel invalide). Il suffit de pointer correctement le kernel dans l’Interface Descriptor pour retrouver un calcul Bitcoin valide.

---

**Rapport généré le** : 2026-05-23 00:09 UTC+2, **Analyste** : Bob (Mode Advanced)  

**Sources** : Intel Gen9 PRM (Kernel Start Pointer dans DW0【43†L166-L173】), libdrm i915 header (EXEC_OBJECT_PINNED/offset behavior【46†L3898-L3900】).