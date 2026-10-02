# Rapport 156 — Audit critique S155 : Richardson adaptatif et portée réelle de FORENSIC-UNIF-001

**Date :** 2026-10-02  
**Dépôt :** vgacofc/lumvorax2  
**Branche :** main  
**HEAD audité :** `45fdd3092b4c3db2d08441e1b8901d575866a6b5`  
**Session auditée :** 155  
**CERTIFIED_100=false**  
**unique_human_proven=false**  
**Mode DEBUG actif**

---

## 1. Expertises activées

- Audit forensique Git/GitHub
- Analyse statique C99
- CFD / Navier–Stokes 2D
- Analyse de convergence stationnaire
- Méthodes de Richardson
- Séparation erreur spatiale / erreur temporelle
- Métrologie et reproductibilité
- Analyse bit-level / IEEE 754
- Traçabilité LUM et identifiants
- Analyse de timestamps et concurrence
- Validation expérimentale et falsifiabilité des critères
- Audit documentaire et continuité des chantiers

---

# 2. Synchronisation Git — état réellement observé

Le HEAD distant de `main` est bien :

**`45fdd3092b4c3db2d08441e1b8901d575866a6b5`**

Le commit porte le message :

**Add S155 Richardson-PROTOCOL-002 adaptatif + FORENSIC-UNIF-001 100% coverage**

Il ajoute trois éléments pertinents :

1. `RAPPORT/155_RICHARDSON_PROTOCOL_002_ADAPTATIF_FORENSIC_UNIF_001_20261002.md`
2. `src/validation/ns_richardson_adaptive.c`
3. `src/validation/ns_forensic_unif.c`

Aucun code n'est modifié par le présent audit. Ce rapport constitue uniquement l'artefact documentaire de la session 156.

---

# 3. Richardson-PROTOCOL-002 — ce qui est réellement amélioré

## Processus

Le nouveau programme ne considère plus 5 000 pas comme une fin scientifique.

Il exécute le solveur par blocs de 50 pas, mesure :

- (u_{max}) ;
- énergie cinétique ;
- résidu Poisson ;

puis conserve une fenêtre de 200 mesures.

La convergence est déclarée seulement lorsque :

- variation relative de (u_{max}) < 1e-3 ;
- variation relative de l'énergie < 1e-3 ;
- résidu Poisson < 1e-4.

La limite de 500 000 pas est explicitement une limite de sécurité et son atteinte ne produit jamais un PASS.

C'est une architecture nettement plus correcte que l'ancien arrêt fixe à 5 s.

## Problème

Le critère de stationnarité est encore un **critère empirique interne au solveur**.

Il démontre :

> « les observables choisies sont suffisamment stables pendant la fenêtre choisie »

mais pas automatiquement :

> « la solution numérique est mathématiquement proche de la solution stationnaire exacte ».

La fenêtre représente 200 checkpoints × 50 pas = **10 000 pas**. Ce choix doit donc être documenté comme paramètre expérimental, et non comme une propriété universelle de convergence.

Autre point : le résidu Poisson utilisé dans le test est celui du dernier pas contrôlé. Il n'est pas soumis à la même fenêtre de stabilité que (u_{max}) et l'énergie.

## Solution

Conserver le mécanisme adaptatif, mais distinguer explicitement trois niveaux :

1. **stationnarité numérique observée** ;
2. **résolution des équations suffisamment faible** ;
3. **convergence vers une solution de référence indépendante**.

La fermeture scientifique Richardson doit utiliser les trois.

---

# 4. Résultat S155 — les L2 à convergence sont maintenant informatifs

Les valeurs annoncées pour le protocole A sont :

- 32×32 : L2 = 0.011173 ;
- 64×64 : L2 = 0.011122 ;
- 128×128 : L2 = 0.011229.

Les temps de convergence annoncés sont environ :

- 32×32 : 21,85 s ;
- 64×64 : 21,80 s ;
- 128×128 : 21,75 s.

Les ordres observés sont :

- 32→64 : +0,066 ;
- 64→128 : −0,139.

T01 et T02 restent donc FAIL.

## Ce que cela prouve

Cela montre que supprimer la limite de 5 000 pas a supprimé une source importante de contamination transitoire.

C'est-à-dire : on ne compare plus simplement trois grilles arrêtées arbitrairement à 5 s.

## Ce que cela ne prouve pas

Il n'est pas encore démontré que la référence Ghia est la cause dominante de la saturation de L2.

Le rapport 155 formule cette explication comme une interprétation. Les données actuelles permettent de dire :

**hypothèse plausible, non démontrée.**

Pour démontrer cette hypothèse, il faut une référence indépendante dont l'erreur d'interpolation et de tabulation soit négligeable devant l'erreur numérique mesurée.

---

# 5. Point critique : le calcul L2 contre Ghia doit être renforcé

Le code utilise 17 points Ghia et interpole linéairement entre eux.

Le L2 est ensuite calculé sur les cellules de la grille.

## Problème

Une erreur mesurée par rapport à une référence discrète/interpolée mélange plusieurs contributions :

1. erreur du solveur ;
2. erreur de discrétisation spatiale ;
3. erreur temporelle résiduelle ;
4. erreur d'interpolation de la référence ;
5. différence de localisation entre les points de la grille et les points de référence.

Il est donc incorrect de conclure directement :

> « les trois L2 sont presque identiques, donc l'erreur spatiale est inférieure à la résolution Ghia ».

La conclusion correcte est :

> « la mesure actuelle ne permet pas de séparer suffisamment l'erreur du solveur de celle introduite par la référence Ghia ».

---

# 6. Richardson-PROTOCOL-003 — prochaine étape scientifique

## Processus

Il faut maintenant utiliser une solution de référence analytique ou manufacturée dont la valeur exacte est connue partout.

Le principe est :

**solution exacte → solution numérique → erreur réelle → raffinement de grille → ordre observé**

Cela supprime la dépendance aux 17 points Ghia.

## Problème

Le protocole actuel essaie de déduire l'ordre spatial à partir d'une référence qui n'est pas assez dense pour isoler proprement l'erreur.

Modifier seulement le seuil T02 ne résoudrait rien.

## Solution

Créer **Richardson-PROTOCOL-003** avec au minimum :

### A. Référence exacte

Utiliser un problème pour lequel la solution exacte est connue et compatible avec le solveur et ses conditions aux limites.

### B. Séparation temporelle

Mesurer l'erreur en faisant varier indépendamment :

- résolution spatiale ;
- pas temporel.

### C. Raffinement

Au minimum :

- 32×32 ;
- 64×64 ;
- 128×128 ;
- idéalement 256×256 si le coût reste acceptable.

### D. Mesures

Pour chaque combinaison :

- L1 ;
- L2 ;
- L∞ ;
- divergence maximale ;
- résidu Poisson ;
- temps physique ;
- nombre de pas ;
- temps machine ;
- critère de stationnarité.

### E. Conclusion

Ne déclarer un ordre spatial que lorsque l'étude de sensibilité temporelle montre que l'erreur temporelle n'est plus dominante.

---

# 7. FORENSIC-UNIF-001 — le PASS 100 % doit être correctement interprété

Le programme S155 obtient :

**33 920 bits attendus = 33 920 événements tracés = 100 %.**

Ce résultat est reproductible au niveau du **nombre d'événements générés par le test**.

Mais il ne faut pas transformer ce résultat en :

> « 100 % de tous les bits de toutes les opérations Navier–Stokes sont désormais prouvés ».

Ce n'est pas ce que le code démontre.

---

# 8. Anomalie forensic n°1 — la valeur du bit n'est pas réellement journalisée

Le code copie bien le `double` dans un `uint64_t` avec `memcpy`.

Mais la variable `raw` ainsi obtenue n'est ensuite pas utilisée pour enregistrer la valeur de chaque bit.

Le programme génère simplement 64 événements correspondant aux positions 0 à 63.

## C'est-à-dire

Le système démontre :

**« j'ai généré un événement pour chacun des 64 emplacements de bits »**

mais pas :

**« voici la valeur 0/1 du bit et son parcours vérifiable »**.

Le PASS actuel est donc un **PASS de couverture d'emplacements/événements**, pas encore un PASS de provenance bit-valeur.

## Ordre de correction

Ajouter dans le protocole forensic une représentation vérifiable de :

- BIT_ID ;
- valeur du bit ;
- valeur brute 64 bits du double ;
- LUM_ID ;
- opération ;
- cellule ;
- étape ;
- timestamp ;
- relation entrée → sortie.

---

# 9. Anomalie forensic n°2 — le timestamp « +1 ns » est artificiel

Le programme prend un timestamp de base puis produit :

**ts_base + bit_pos**

Cela garantit des valeurs différentes dans le log, mais ne signifie pas que 64 événements ont réellement été exécutés à 1 ns d'intervalle.

Le commentaire du code reconnaît correctement que la résolution réelle de l'horloge dépend de l'OS et du matériel.

## Conclusion

Il faut conserver la distinction :

- **unité : nanoseconde** ;
- **valeur numérique : exprimée en ns** ;
- **résolution réelle de l'horloge : indépendante** ;
- **ordre logique des bits : peut être imposé artificiellement**.

Pour un audit forensic, cette distinction est obligatoire.

---

# 10. Anomalie forensic n°3 — couverture des opérations NS encore partielle

Le test trace cinq catégories :

1. u entrant ;
2. v entrant ;
3. p entrant ;
4. u après correction ;
5. résidu Poisson.

Mais cela ne correspond pas à la totalité des états intermédiaires d'un pas Navier–Stokes.

Notamment, le test ne fournit pas une chaîne complète indépendante pour chaque opération intermédiaire telle que :

- calcul d'advection ;
- diffusion ;
- état intermédiaire ;
- sortie v ;
- sortie p ;
- correction complète ;
- dépendances entre entrées et sorties.

Le commentaire du fichier annonce une ambition plus large que ce que le test instrumente réellement.

## Verdict

**FORENSIC-UNIF-001 : PASS pour la couverture des cinq catégories explicitement instrumentées sur la campagne 4×4 × 10.**

**FORENSIC NS universel : encore OPEN.**

---

# 11. Anomalie forensic n°4 — portée de l'identifiant LUM_ID

L'encodage réserve :

- module : 4 bits ;
- step : 8 bits ;
- i : 6 bits ;
- j : 6 bits ;
- bit : 8 bits.

Cela suffit pour la campagne 4×4 × 10.

Mais l'identifiant n'est pas universel pour toutes les campagnes futures :

- step est limité à 255 ;
- i est limité à 63 ;
- j est limité à 63.

Une grille 128×128 dépasse donc directement les champs i/j disponibles.

## Solution

Passer à un identifiant structuré ou à un encodage 64 bits comprenant explicitement :

- projet ;
- protocole ;
- run_id ;
- module ;
- step ;
- i ;
- j ;
- bit_pos ;
- génération.

L'identifiant doit rester unique entre plusieurs runs, plusieurs protocoles et plusieurs exécutions.

---

# 12. Anomalie forensic n°5 — FORENSIC dans Richardson n'est pas encore bit-level

Dans `ns_richardson_adaptive.c`, le forensic de Richardson journalise un événement par checkpoint.

Cela donne une traçabilité :

**grille → checkpoint → timestamp**

mais pas :

**chaque bit du champ numérique → LUM_ID → transformation → sortie**.

Il faut donc maintenir deux statuts séparés :

- FORENSIC-UNIF-001 NS ciblé : PASS sur la campagne définie ;
- FORENSIC-UNIF-001 universel : OPEN.

---

# 13. Ordre de travail verrouillé après S155

L'ordre suivant doit être conservé afin de ne pas abandonner les chantiers précédents.

## Étape 1 — FORENSIC-UNIF-002

Fermer les faiblesses du forensic bit-level :

1. valeur réelle de chaque bit ;
2. BIT_ID explicite ;
3. LUM_ID globalement unique ;
4. run_id/protocole ;
5. entrée et sortie ;
6. chaîne parent → enfant ;
7. timestamps réels ;
8. détection de perte et duplication.

**Objectif : transformer le PASS « nombre d'événements » en preuve de provenance.**

## Étape 2 — Richardson-PROTOCOL-003

Construire la validation sur solution exacte/manufacturée.

Ne pas chercher à faire passer artificiellement T01/T02.

**Objectif : mesurer réellement l'ordre spatial.**

## Étape 3 — Validation temporelle

Pour les mêmes grilles :

- dt constant ;
- dt proportionnel à dx ;
- dt proportionnel à dx² ;

puis étude de sensibilité permettant d'isoler l'erreur temporelle.

## Étape 4 — T04 renforcé

Conserver le PASS actuel comme PASS du critère défini, mais ajouter une fenêtre complète de quasi-stationnarité :

- moyenne ;
- min/max ;
- écart-type ;
- pente ;
- variation maximale.

## Étape 5 — Lyapunov robuste

Balayer réellement les valeurs annoncées d'epsilon et tester :

- epsilon ;
- warmup ;
- intervalle de renormalisation ;
- nombre de renormalisations ;
- résolution.

Séparer :

**robustesse du signe**

de

**robustesse quantitative de λ**.

## Étape 6 — BUILD-THREAD-001

Validation ThreadSanitizer de FL-005.

## Étape 7 — BUILD-PROOF-001

CI C indépendante et reproductible.

## Étape 8 — BUILD-PORT-002

Exécution réelle sur une cible sans les extensions ISA concernées.

## Étape 9 — BL-003 → BL-012

Poursuite du registre blockchain historique.

## Étape 10 — C3

Démonstration de l'intégration réelle NS → NX-42.

## Étape 11 — C4

Remplacement progressif des `STUB_MEASURED` des problèmes 6–30 par des calculs réellement démontrés.

---

# 14. Ce qu'il ne faut surtout pas faire maintenant

Ne pas :

- déclarer Richardson fermé parce que le solveur atteint 21,8 s ;
- déclarer l'ordre spatial démontré à partir de Ghia ;
- déclarer « 100 % bit-level universel » à partir de 33 920 événements ;
- considérer un timestamp artificiellement incrémenté de 1 ns comme une mesure physique de 1 ns ;
- supprimer les FAIL T01/T02 ;
- abandonner les tâches BUILD, blockchain, Lyapunov, NX et historiques.

Le rôle du protocole est précisément de produire un FAIL lorsque la preuve n'est pas suffisante.

---

# 15. État de vérité S156

| Chantier | Verdict |
|---|---|
| Suppression du plafond scientifique 5 000 pas | **VALIDÉE** |
| Convergence adaptative S155 | **VALIDÉE comme mécanisme expérimental** |
| Stationnarité mathématique exacte | **NON DÉMONTRÉE** |
| Richardson T01 | **FAIL honnête** |
| Richardson T02 | **FAIL honnête** |
| Richardson-PROTOCOL-003 | **OPEN — priorité scientifique** |
| FORENSIC-UNIF-001 campagne NS 4×4×10 | **PASS 100 % événements attendus** |
| Provenance de la valeur réelle de chaque bit | **OPEN** |
| Forensic de toutes les opérations NS | **OPEN** |
| Forensic Richardson/Lyapunov/NX-42 | **OPEN** |
| BUILD-THREAD-001 | **OPEN** |
| BUILD-PROOF-001 | **OPEN** |
| BUILD-PORT-002 | **OPEN** |
| BL-003 → BL-012 | **OPEN** |
| C3 NS → NX-42 | **OPEN** |
| C4 NX-42 6–30 | **OPEN / STUB_MEASURED** |
| T04 | **PASS selon critère actuel ; renforcement recommandé** |
| Lyapunov | **PASS selon critère actuel ; robustesse quantitative OPEN** |
| V138 | **CLÔTURÉ** |
| CERTIFIED_100 | **false** |
| unique_human_proven | **false** |

---

# 16. Conclusion

S155 a effectivement franchi une étape importante : le solveur n'est plus artificiellement arrêté à 5 000 pas pour la campagne adaptative, et les résultats à environ 21,8 s montrent que le régime transitoire était une variable importante à éliminer.

Mais S155 révèle aussi précisément où se trouve maintenant le prochain verrou scientifique.

**Richardson n'est pas encore démontré.**

La prochaine preuve doit utiliser une référence exacte ou manufacturée et séparer proprement l'erreur spatiale de l'erreur temporelle.

De même, le **100 % forensic actuel doit être qualifié correctement** : il démontre 100 % des événements bit-position attendus dans les cinq catégories instrumentées sur une campagne 4×4×10. Il ne démontre pas encore la provenance universelle de la valeur de chaque bit à travers toutes les opérations NS.

Le prochain objectif est donc **FORENSIC-UNIF-002 puis Richardson-PROTOCOL-003**, sans retirer aucune tâche historique du registre.

**Aucun code source n'a été modifié pendant cet audit.**
