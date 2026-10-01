# RAPPORT 125 — RÉCONCILIATION FORENSIQUE DE L'ÉTAT DISTANT ET VALIDATION DES ANOMALIES C1–C5

**Dépôt audité :** vgacofc/lumvorax2  
**Branche :** main  
**Révision distante observée :** 2ea3a87e23ef49cc7285010bc6fc527faf119564  
**Date :** 2026-10-01  
**Mode :** lecture distante, recherche GitHub et inspection statique  
**Modification du code :** AUCUNE

## 1. EXPERTISES ACTIVÉES
- Audit forensique Git/GitHub.
- Traçabilité documentaire et contrôle de numérotation.
- Analyse statique C.
- Validation des preuves d'exécution et détection des métriques codées en dur.
- Analyse numérique/scientifique de modèles physiques.
- Audit du pipeline ML Python NX47.
- Contrôle de cohérence code ↔ logs ↔ rapports.
- Contrôle d'intégrité des conflits Git.

## 2. MÉTHODE
Une affirmation historique n'est considérée comme établie que si elle peut être reliée à un fichier, une révision ou un artefact actuellement présent dans le dépôt. Le contrôle sépare donc : confirmé, partiellement confirmé, non retrouvé et contradictoire.

## 3. ÉTAT DU DÉPÔT
La branche main est accessible. La dernière révision observée est 2ea3a87e23ef49cc7285010bc6fc527faf119564, avec le commit intitulé chatC138. Les commits précédents observés comprennent ubuntu run C137 resultat et chatC134.

Le résumé de mission ne doit donc pas être utilisé comme photographie exacte du dépôt : il doit être réconcilié avec GitHub.

## 4. NUMÉROTATION DES RAPPORTS
Le dossier RAPPORT/ contient actuellement une série structurée qui s'arrête au rapport 124 : 124_RAPPORT_CORRECTION_FINALE_TOUS_MODULES_CONFORMITE_PROMPT_TXT_20250925_234900.md.

Les fichiers RAPPORT_EXPLICATION_V125_SUPERVISE.md, RAPPORT_EXPLICATION_V126_AUDIT_FORENSIC_COMPLET.md et RAPPORT_126_AUDIT_FORENSIQUE_ULTRA_EXHAUSTIF_LIGNE_PAR_LIGNE_TOUS_MODULES_20250926_223500.md existent ailleurs, mais ne sont pas les rapports 125 et 126 de la série RAPPORT/.

Le présent document est donc le RAPPORT 125 de la série RAPPORT/. Les versions techniques V125, V126, V138 restent des artefacts distincts.

## 5. C1 — LATENCES CODÉES EN DUR
### Processus
Une mesure de performance authentique doit être produite par une horloge d'exécution autour de l'opération réellement mesurée.

### Preuve
Le fichier RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle/src/tests/nx42_30_problems_execution.c contient une fonction log_problem qui reçoit directement les latences en arguments.

Le problème 5 utilise baseline = 2500 ns et optimisé = 1800 ns. Les autres problèmes utilisent également des couples de valeurs constantes.

Le programme possède un timestamp global, mais ne mesure pas la durée réelle de chaque calcul comparé.

### Problème
Un timestamp global n'est pas une mesure de latence algorithmique. Les valeurs affichées sont des valeurs déclarées dans le programme.

### Solution
Mesurer début et fin avec une horloge monotone, calculer le delta, répéter les essais et publier au minimum médiane, moyenne, dispersion et conditions matérielles.

**Statut C1 : CONFIRMÉ.**

## 6. C2 — LYAPUNOV P9
### Preuve
Le fichier logs_AIMO3/nx/NX-35/NX35_LOG_P9.csc existe et contient metric_lyapunov = 0.0254219.

### Problème
Le résumé de mission affirme une contradiction avec une étiquette STABLE. La valeur 0.0254219 est confirmée, mais la recherche actuelle ne retrouve pas dans le dépôt une déclaration STABLE directement reliée à cette ligne.

### Solution
Identifier la définition mathématique du signe de l'exposant, l'endroit où STABLE est attribué et la tolérance utilisée. Une contradiction ne doit être déclarée qu'après établissement de cette convention.

**Statut C2 : PARTIELLEMENT CONFIRMÉ.**

## 7. C3 — MODÈLE PHYSIQUE
### Preuve
Le fichier RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle/src/sch/nx/sch_nx_v11.c contient une dynamique NX11 avec position x, vitesse vx, bruit pseudo-aléatoire, ATP et hystérésis.

La fonction nx11_physics ajoute un bruit aléatoire à la vitesse puis met à jour la position avec vitesse × pas de temps.

### Problème
Ce mécanisme ne constitue pas un solveur Navier–Stokes identifiable. Il ne présente pas explicitement les champs de vitesse et de pression ni une discrétisation des termes convectifs et diffusifs des équations de conservation.

### Solution
Créer un solveur scientifique séparé avec domaine, conditions aux limites, viscosité/Reynolds, discrétisation documentée, convergence mesurable, conservation et benchmark indépendant.

**Statut C3 : ANOMALIE SCIENTIFIQUE CONFIRMÉE.** La formulation exacte « Brownian » du résumé est remplacée ici par la description plus précise « dynamique stochastique aléatoire simplifiée ».

## 8. C4 — 30 PROBLÈMES
### Preuve
Dans nx42_30_problems_execution.c, les cinq premiers problèmes appellent log_problem avec des valeurs prédéfinies. Pour les problèmes 6 à 30, une boucle affiche essentiellement le nom du problème et VALIDATED.

### Problème
L'affichage VALIDATED ne démontre pas qu'un calcul scientifique correspondant a été exécuté.

### Solution
Chaque problème doit disposer d'une fonction de calcul identifiable, d'entrées définies, d'une sortie mesurée et d'un critère de validation indépendant. Le statut VALIDATED doit être interdit lorsque le calcul n'a pas été exécuté.

**Statut C4 : CONFIRMÉ.**

## 9. C5 — ISOLATION LUMVORAX / ARTCB
### Preuve recherchée
Le résumé affirme que .gitignore contient LVX&ARTCB. La recherche directe dans le .gitignore de la révision main actuellement auditée ne retrouve pas cette chaîne ni les termes ARTCB ou lumvorax.

### Problème
Cette absence de correspondance ne prouve pas que l'isolation n'existe pas. Elle signifie seulement que l'affirmation précise du résumé n'est pas vérifiable dans la révision distante actuelle.

### Solution
Contrôler .gitignore distant, arborescence Git, chemins LVX/ARTCB, branches, sous-modules et historique des commits.

**Statut C5 : NON CONFIRMÉ SUR LA RÉVISION DISTANTE ACTUELLE.**

## 10. ANOMALIE P0 — CONFLIT GIT DANS LE RAPPORT V138
Le fichier RAPPORT V138 — Cartographie technologique exhaustive, écarts V125→V137 et plan d’intégration verrouillé.md contient réellement les marqueurs <<<<<<<, ======= et >>>>>>>.

### Problème
Ce sont des marqueurs de conflit Git non résolus. Le document ne peut donc pas être considéré comme un artefact documentaire final propre.

### Solution
Identifier les deux versions, sélectionner le contenu retenu, supprimer les marqueurs, puis rechercher à nouveau ces marqueurs dans le document final.

**Statut : CONFIRMÉ — P0 DOCUMENTAIRE.**

## 11. BUG V137 ET CORRECTION V138
Le noyau nx47_vesu_kernel_v137.py contient une lecture de epoch_best['best_objective'], alors que le résultat d'époque utilise objective.

Le noyau nx47_vesu_kernel_v138.py contient la correction correspondante : la comparaison utilise epoch_best['objective'].

**Statut : bug V137 confirmé historiquement ; correction présente dans V138.**

## 12. SYNTHÈSE
| Élément | Statut | Preuve |
|---|---|---|
| C1 latences codées en dur | CONFIRMÉ | code C direct |
| C2 Lyapunov 0.0254219 | CONFIRMÉ ; contradiction STABLE non prouvée | log direct |
| C3 absence de solveur NS dans fichier ciblé | CONFIRMÉ | code C direct |
| C4 problèmes 6–30 sans calcul spécifique | CONFIRMÉ | code C direct |
| C5 règle LVX&ARTCB | NON CONFIRMÉ actuellement | .gitignore distant |
| Conflit Git V138 | CONFIRMÉ | rapport direct |
| Bug best_objective V137 | CONFIRMÉ historiquement | code V137 |
| Correctif correspondant V138 | CONFIRMÉ | code V138 |

## 13. PRIORITÉS
### P0
1. Nettoyer le conflit documentaire V138.
2. Séparer définitivement les numérotations RAPPORT/ et Vxxx.

### P1
3. Remplacer les latences déclaratives par des mesures runtime.
4. Supprimer les validations décoratives des problèmes 6–30.
5. Formaliser la convention Lyapunov de P9.

### P2
6. Construire un vrai solveur Navier–Stokes 2D isolé et benchmarkable.

## 14. ROADMAP CONSOLIDÉE
Phase 0 : assainissement documentaire et preuves runtime.
Phase 1 : solveur Navier–Stokes 2D 64×64, conditions aux limites, convergence et benchmark.
Phase 2 : intégration dans NX après validation scientifique indépendante.
Phase 3 : extension 3D et optimisation SIMD.
Phase 4 : formalisation Lean de propriétés clairement définies.

## 15. CONCLUSION FORENSIQUE
Le statut 100 % du résumé initial ne peut pas être repris comme état factuel du dépôt distant sans réserve.

Les preuves directes établissent C1 et C4. C2 possède une valeur de log vérifiable, mais sa prétendue contradiction avec STABLE doit encore être reliée à une convention mathématique explicite. C3 révèle une dynamique qui n'est pas un solveur Navier–Stokes dans le fichier ciblé. C5 n'est pas confirmé par le .gitignore distant actuel. Le rapport V138 contient des conflits Git non résolus. Le bug V137 best_objective est confirmé historiquement et son correctif est présent dans V138.

**Aucun code source n'a été modifié pendant cet audit.**
**Seul ce rapport 125 est ajouté à la série RAPPORT/.**