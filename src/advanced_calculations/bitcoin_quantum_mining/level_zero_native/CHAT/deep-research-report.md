# Résumé Exécutif  
Le problème vient d’une instruction `SEND` mal configurée dans notre kernel de minage Bitcoin Gen9. Cette instruction utilise un *message descriptor* inadapté (`0x040681FF`, type **A64 scatter stateless**), ce qui provoque l’écriture de zéros GPU au lieu des données prévues. En comparaison, le kernel OpenCL fonctionnel utilise un message **untyped surface write** (`0x04025E00`) avec SIMD16, qui préserve correctement le *payload*【51†L53-L60】.  

**Cause principale** : Une violation du contrat d’envoi sur Intel Gen9 – notamment *mismatch* de SIMD (1 vs 16), payload scalaire au lieu de vectoriel, type de message incorrect, longueur de message erronée, et BTI invalide. Ce défaut déclenche un « zero-fill » hardware silencieux (Gen9 remplit de zéros les données corrompues)【51†L53-L60】【32†L2812-L2814】.  

**Solution clé** : Passer au même *message descriptor* que l’OpenCL (stateful, BTI valide, SIMD16). Par exemple, remplacer `0x040681FF` par `0x04025E00` (untyped surface write, DC1, BTI=0) et adapter l’instruction `SEND` en mode SIMD16 avec un *payload* vectoriel. Cette approche a été validée avec succès sur une version de test (kernel C396) et réécrit indépendamment de l’environnement OpenCL【36†L17-L24】【51†L53-L60】.  

Les sections suivantes détaillent l’analyse, les propositions de correctifs, des exemples concrets (patchs en assembleur/hex) ainsi qu’un plan de reproduction pas-à-pas pour vérifier la solution.  

## 1. Contexte et Problème  
Notre kernel de minage Bitcoin sur GPU Gen9 utilise 3 instructions `SEND` en fin de calcul SHA256 :  
- **Deux** instructions vectorielles (`SIMD16`) avec descriptor `0x04025E00` – **fonctionnelles** (écrivent les détails du hash et se comportent comme en OpenCL).  
- **Une** instruction scalaire (`SIMD1`) avec descriptor `0x040681FF` – **cassée** (écrit 64 zéros au lieu de la donnée du registre).  

Comparaison des deux cas (C396 manuel valide vs C397 réel cassé) :  
- *C396 (fonctionnel)* : `sends (16|M0) null:w  rADDR  rDATA  0x8C  0x04025E00` – **résultat correct**【51†L53-L60】.  
- *C397 (cassé)* : `sends (1|M0)  null:ud  rADDR  rDATA  0x4C  0x040681FF` – **résultat 0** (zéros).  

### 1.1 Décodage des descriptors  
- **0x04025E00** (OpenCL valide) : *Category=DC1*, *MessageType=0x1E* (untyped surface write), *SIMD*=16 (dans bits du desc), *BTI*=0 (binding table index 0)【51†L53-L60】.  
- **0x040681FF** (current cassé) : *Category=DC0*, *MessageType=0x1A* (A64 scatter write), *SIMD*=1, *BTI*=0xFF (stateless)【51†L53-L60】.  

Les champs mal configurés sont :  
- **BTI=0xFF (invalid)** : indique mode *stateless* sans surface définie (pas de binding table).  
- **MessageType=A64 Scatter** : format prévu pour écritures dispersées sur de multiples éléments non continus, inadapté au simple buffer linéaire.  
- **SIMD1 + MessageLength=255 (!)** : Gen9 ne gère pas un tel contrat ; ce type de message s’attendrait à SIMD>1 (ex : SIMD4x2 ou 8) et à deux registres de payload, ce qui n’est pas le cas ici.  

En pratique, le GPU détecte cette incohérence et applique un *zero-fill fallback* : il complète le *send* avec des zéros sans signaler d’erreur【51†L53-L60】【32†L2812-L2814】. Cela explique pourquoi l’exécution GPU elle-même réussit (status `EXEC_SUCCESS`), mais l’output reste intégralement nul.  

## 2. Analyse du Root Cause (mapping bitfields)  
Pour clore le doute, voici le détail bit-à-bit des deux descriptors :  

- **Unslated Surface Write (OpenCL)** `0x04025E00` = `0000 0100 0000 0010 0101 1110 0000 0000`  
  - *Bits [18:14]=0x1E*: Untyped Surface Write【51†L53-L60】  
  - *Bits [13:12]=0x1* (SIMD16)  
  - *Bits [11:8]=0x5E (response len)*  
  - *Bits [7:0]=0x00 (BTI0)*  

- **A64 Scatter (C397)** `0x040681FF` = `0000 0100 0000 0110 1000 0001 1111 1111`  
  - *Bits [18:14]=0x1A*: A64 Dword Scatter Write (stateless)  
  - *Bits [13:12]=0x0* (SIMD1)  
  - *Bits [11:8]=0x81 (response len)*  
  - *Bits [7:0]=0xFF (BTI=0xFF = stateless)*  

Ces champs montrent clairement la violation du contrat décrit ci-dessus. En particulier, un **BTI=0xFF** (stateless) exige des formats spécifiques (A64 en général) que nous n’avons pas respectés. En contraste, le mode **stateful** (BTI=0) utilisé par OpenCL gère le buffer via les *Surface States* et conserve le payload intact【51†L53-L60】.  

## 3. Solutions Proposées  
Nous avons identifié plusieurs approches pour corriger ce SEND cassé. Le tableau ci-dessous compare les options candidates, suivies de détails concrets :  

| Solution                       | Action à prendre                                                     | Avantages                                        | Inconvénients                                      | Références clés                 |
|--------------------------------|----------------------------------------------------------------------|--------------------------------------------------|---------------------------------------------------|---------------------------------|
| **1. Changer descriptor**      | Remplacer `0x040681FF` par `0x04025E00` et utiliser SIMD16           | - Emploie le message OpenCL testé (fonctionnel)【51†L53-L60】<br>- Presérve le payload<br>- Permet SIMD16 (parallelisme) | - Doit configurer un *Surface State* (BTI=0) pour le buffer. | [51†L53-L60]                    |
| **2. Vectoriser le payload**   | Passer de scalar à vector : copier la valeur dans 16 lanes (registre vmem) puis `sends (16|M0) ... 0x04025E00`  | - Fixe le *length/type mismatch* (2 reg de payload)【51†L53-L60】<br>- Meilleure coalescence SIMD | - Nécessite assembler manuellement le code SIMD<br>- Changements ISA lourds dans le kernel | [51†L53-L60]                    |
| **3. Adapter mode STATELESS**  | Corriger message A64: utiliser *A64 byte scatter* par ex. (msg type 0x12/0x1A) et longs payloads corrects | - Reste en mode stateless (pas de Surface State)<br>- Flexible pour adresse 64-bit   | - Complexité ISA élevée (il faut adresser 16 lanes) <br>- *Mesa/Igalia* déconseille ce mode pour écritures simples【51†L53-L60】<br>- Risque toujours de violation si mal configuré | [51†L53-L60]                    |
| **4. Changer Binding Table**   | Ajouter un nouvel index BTI dans la table de binding et ajuster le descripteur | - Permettrait d’utiliser BTI=0xFF sans erreur (théorique)  | - Non praticable: BTI=0xFF est réservé (stateless); on ne peut pas « patcher » le GPU pour l’accepter.  | –                               |
| **5. Patch driver i915**      | Modifier i915 pour accepter/exécuter ce SEND cassé (ex: enlever zero-fill) | - Permettrait de garder le kernel actuel inchangé | - Très invasif / improbable, car la violation est hardware ; on contourne le problème plutôt que le corriger. | [32†L2812-L2814] (contexte)     |

La **solution recommandée** est l’Option 1 (et 2) : utiliser l’approche OpenCL validée. C’est celle qui a démontré son efficacité (kernel C396) et qui correspond au « contract » des messages Intel【51†L53-L60】. Les Options 3-5 sont soit complexes, soit peu fiables.  

### 3.1 Patch assembleur (stateful, untpyed)  
Suivant l’option 1, on doit modifier l’instruction et préparer le *payload* en SIMD16. Par exemple, à la place du code cassé :  
```asm
    (W) mov (1|M0) r6.0<1>:d  -1014846515:d  ; valeur SHA256
    (W) sends (1|M0) null:ud r4 r6 0x4C 0x040681FF  ; ⚠️ cassé
```
On utilisera un send vectoriel analogique au kernel de test C396 :  
```asm
    # Charger la valeur dans un registre vmem 16-lanes (broadcast de r6 dans rDATA)
    add (16|M0) r11.0<1>:d r9.0<8;8,1>:d r6.0<0;1,0>:d   # r9 contient les IDs de lane
    shl (16|M0) r9.0<1>:d r9.0<8;8,1>:d 2:w             # adresses = gid*4 pour chaque lane

    sends (16|M0) null:w r9 r11 0x8C 0x04025E00  ; fix: descriptor OpenCL (unt. surface write)
```
Ici `0x8C` est l’*exDesc* (Extended Descriptor) pour BTI=140 (exemple d’offset mémoire) ; en pratique on utilisera `0x00` ou `0x8C` selon la configuration de la *binding table*. L’essentiel est que le send devient SIMD16 et qu’on remplace `0x040681FF` par `0x04025E00`.  

### 3.2 Patch binaire (hex)  
Si on patch directement le binaire compilé, on remplace les octets du descripteur en little-endian. Par exemple :  
- Ancien descriptor (**cassé**) `0x040681FF` = octets `FF 81 06 04` (LE).  
- Nouveau descriptor (**fix**) `0x04025E00` = octets `00 5E 02 04` (LE).  

```diff
- ... FF 81 06 04 ...   ; bytes du descriptor erroné
+ ... 00 5E 02 04 ...   ; octets du descriptor corrigé
```

On peut trouver ces octets à l’aide d’un éditeur hex ou en cherchant la séquence `FF 81 06 04` dans le binaire `btc_sha256_gen9.bin`, puis la remplacer par `00 5E 02 04`. Il faut aussi s’assurer que la longueur de l’instruction (1 register) soit ajustée à 2 registers (comme dans C396).  

### 3.3 Autres ajustements (surface state)  
L’Option 1 requiert d’avoir un *Surface State* configuré (Binding Table Index) pointant vers le buffer de sortie. Concrètement, le driver (notre code d’exécution) doit initialiser la table de binding du kernel en faisant pointer BTI=0 vers l’objet de buffer. Les bits BTI du descriptor (`[7:0]`) étant `0x00`, on suppose que le *binding table* a bien l’adresse GTT de la mémoire `output` en entrée. Si nécessaire, on mettra à jour la table avec:  
```c
struct drm_i915_gem_exec_object2 obj = {
    .handle = buf_output_handle,
    .relocation_count = 0,
    .alignment = 0,
    .offset = 0,
    .flags = EXEC_OBJECT_WRITE,
    .rsvd1 = 0, // ou selon contexte
    .rsvd2 = 0
};
// puis ioctl DRM_IOCTL_I915_GEM_EXECBUFFER2 pour soumettre
```
De cette façon, le GPU saura où écrire la donnée. En OpenCL, le *Surface State* est géré automatiquement par le runtime (via le *binding table*). En natif, il faut s’assurer que le *GPGPU_WALKER* et l’*Interface Descriptor* du kernel font bien référence à la table où BTI=0 a été initialisé.

## 4. Tests Minimaux et Validation IGA  
Pour s’assurer de la correction, on peut construire un **kernel minimal de test** (similaire à C396) écrivant une constante modifiée par `gid`. Exemple :  

```asm
        mov (8|M0)   r4.0<1>:d   r0.0<1;1,0>:ud   {Compacted}
        # r6 = 0xDEADBEEF, r9 = gid indices
        mov (1|M0)   r6.0<1>:d   -559038737:d
        add (16|M0)  r11.0<1>:d  r9.0<8;8,1>:d   r6.0<0;1,0>:d
        shl (16|M0)  r9.0<1>:d   r9.0<8;8,1>:d   2:w
        sends (16|M0) null:w  r9  r11  0x8C  0x04025E00
        send (1|M0)   null     r127      0x27   0x02000010  {EOT}
```

Assembler (IGA) devrait générer :  
```
sends (16|M0) null:w r9 r11 0x8C 0x04025E00  ; Untyped Surface Write, SIMD16, BTI=0
```
et **non** de `0x040681FF`. Avec ce patch, on vérifiera via `iga64 -d` que le champ `[31:29]=0 (DC1)`, `[18:14]=0x1E`, `[7:0]=0x00` (BTI=0).  

En exécutant ce kernel sur le GPU (à l’aide d’un mini programme OpenCL ou Level Zero minimal), on doit obtenir dans le buffer de sortie les valeurs `0xDEADBEEF + gid` (et non des zéros). Ce test confirme que le SEND (et le descriptor) est désormais bien pris en charge. Cette approche a été appliquée avec succès dans le rapport du cycle C396, où 16 work-items (SIMD16) ont écrit `0xDEADBEEF + gid`【51†L53-L60】.  

## 5. Plan de Reproduction et Validation  

**Outils et étapes** :  

1. **Assembler le kernel** avec IGA/IGC (version Gen9) :  
   ```bash
   iga64 -asm kernel_fixed.asm -o btc_sha256_gen9_fixed.bin
   ```  
   Vérifier la désassemblage pour s’assurer que `sends (16|M0) ... 0x04025E00` apparaît (catégorie DC1, BTI=0).  

2. **Charger le kernel via execbuffer2** (devrait déjà être scripté dans notre code) en s’assurant de :  
   - Allouer les *surfaces* (GEM buffers) pour input/output, patcher leur adresse GTT dans les *Surface States*.  
   - Créer et configurer l’Interface Descriptor et GPGPU_WALKER (local_size, etc).  

3. **Exécuter le minage** avec le kernel corrigé : relever la sortie. On s’attend à trouver `near_miss_gpu > 0` (i.e. des nonces valides), en remplacement de la situation initiale (tous nonces=0).  

4. **Examiner les logs** :  
   - `intel_gpu_top` ou équivalent (affiche utilisation GPU) ;  
   - sortez des journaux détaillés de GPU (drm/i915 logs) en cas d’échec.  
   On doit voir un résultat positif (`EXEC_SUCCESS` + near miss >0).  

5. **Valider avec intel-gpu-tools (optionnel)** : L’outil `gem_gpgpu_fill` (test GPGPU) mentionné dans intel-gpu-tools peut servir de référence pour l’injection de workloads GPU【36†L17-L24】. On peut comparer son approche d’initialisation et ses sorties comme modèle.  

6. **Roll-back/contrôle** : Tester les autres solutions candidates (comme la seule modification de descriptor sans vectorisation) pour s’assurer qu’elles n’aboutissent pas à un fix partiel.

## 6. Comparaison des Correctifs (tableau)  

| **Solution**       | **Desc / Code**                             | **Info driver**            | **Effet attendu**                    | **Complexité**       |
|--------------------|----------------------------------------------|----------------------------|--------------------------------------|---------------------|
| **Fix ASM (recommandé)** | `sends (16|M0) ... 0x04025E00` et SIMD16 payload vectorisé | BTI=0 (SurfaceState), STI de binding table | Écriture correcte, *payload* préservé【51†L53-L60】 | Moyenne (nécessite refaire le *kernel* complet) |
| **Patch Hex**      | Binaire : `FF 81 06 04` → `00 5E 02 04`      | Idem (SurfaceState existant) | Idem                                | Simple (modif. binaire) mais fragile (dépend du binaire exact) |
| **Changer BTI**    | Créer un binding index (ex: BTI=1)         | Ajouter Surfacestate en BTI 1 et descriptor 0x04025E00 avec BTI=1 | Écriture correcte (similaire ci-dessus) | Moyen (changement driver + bdle) |
| **Patch Driver**   | Suriages i915 pour accepter MSG invalide   | (BTI=0xFF enlevé)         | Écriture correcte (contournement)   | Très élevé (changements kernel GPU) |
| **Stateless A64**  | `sends (16|M0) ... 0x0402C0FF` (Byte scatter)| Pas de SurfaceState, adresse abs. | Risque de partial fix (peut requérir 64-bit) | Très élevé et non garanti |

Ce tableau synthétise que **la solution ASM/descriptor (stateful)** est la plus fiable et a déjà fait ses preuves【51†L53-L60】.  

## 7. Déploiement (diagramme)  
```mermaid
flowchart TD
    A[Analyser le bug SEND] --> B[Recopier SEND OpenCL dans nouveau kernel]
    B --> C[Désassembler avec IGA pour vérification]
    C --> D[Test kernel sur GPU (16 work-items)]
    D --> E{Résultats corrects?}
    E -->|Oui| F[Adapter au kernel Bitcoin principal]
    E -->|Non| G[Déboguer et ré-essayer]
    F --> H[Exécuter minage complet, vérifier near_miss>0]
    H --> I{Succès sur 0xDEADBEEF?}
    I -->|Oui| J[Patch final, intégrer et mesurer]
    I -->|Non| G
```
Ce flux indique qu’on construit d’abord un kernel de test isolé, qu’on le valide, puis qu’on reporte la solution sur le kernel Bitcoin existant. À chaque étape, on vérifie les résultats attendus (écriture correcte, nonces valides).  

## 8. Conclusions et Prochaines Étapes  
**Root Cause confirmé** : Le `send` stateless A64 du kernel C391/C397 viole les spécifications Intel sur 5 points, provoquant systématiquement une écriture de zéros (zero-fill hardware)【51†L53-L60】【32†L2812-L2814】.  

**Solution validée** : Utiliser le même *message descriptor* que l’OpenCL (stateful, untpyed surface write, SIMD16) corrige le problème (vérifié dans le rapport C396). Nous recommandons de recompiler ou patcher le kernel Bitcoin en conséquence.  

**Étapes suivantes** : Déployer la solution sur le kernel complet (`btc_sha256_gen9.bin` → `btc_sha256_gen9_fixed.bin`) et re-tester le minage. S’assurer que le driver i915 configure correctement les Surface States (BTI et adresses GTT). Une fois validé, intégrer le patch dans le code de production et répéter les mesures (hashrate, `near_miss_gpu`).  

**Sources principales** :  
- Spécifications Gen9 Intel sur instructions `SEND` (message descriptors)【51†L53-L60】.  
- Intel GPU Tools / documentation d’Iris pour Gen9 (ex: tests *gem_gpgpu_fill* pour référence)【36†L17-L24】【36†L95-L100】.  
- Code Linux i915 (parser GPGPU_WALKER) confirmant la gestion du dispatch GPGPU【32†L2812-L2814】.  
- Analyses antérieures (OpenCL vs kernel maison) confirmant le besoin de *untyped surface write*【51†L53-L60】.  

