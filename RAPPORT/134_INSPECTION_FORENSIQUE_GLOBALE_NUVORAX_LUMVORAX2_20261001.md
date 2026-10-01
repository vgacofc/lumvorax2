# RAPPORT 134 — INSPECTION FORENSIQUE GLOBALE NUVORAX / LUMVORAX2
## Cahier des charges forensic exhaustif, cartographie initiale et protocole de vérité indépendante
### Révision auditée : main @ cf897f8e64cb269f79aecb46111ea8ca4a17cf39
### Date : 2026-10-01

## 0. Objet exact de la mission

La mission demandée est une inspection forensic générale, contradictoire et indépendante de l'ensemble du dépôt.

L'objectif n'est pas de démontrer que le système fonctionne. L'objectif est de déterminer, preuve à l'appui, ce qui fonctionne réellement, ce qui fonctionne seulement en apparence, ce qui est incomplet, ce qui est un stub, un smoke test, un mock, un placeholder, une démonstration, une métrique artificielle, une donnée historique non reproductible, un résultat recyclé, une validation insuffisante ou une chaîne causale non démontrée.

La question centrale devient donc :

« Le logiciel exécuté produit-il réellement les résultats et les capacités qu'il affirme produire, avec les mécanismes réellement annoncés, ou certaines sorties peuvent-elles être obtenues sans que la technologie revendiquée ait effectivement réalisé le calcul correspondant ? »

Aucune conclusion positive d'un ancien rapport n'est considérée comme une preuve en elle-même.

C'est-à-dire : un rapport disant « authentique », « réel », « validé », « 100 % », « ligne par ligne » ou « complet » constitue une déclaration à vérifier, pas une autorité.

## 1. Expertises activées

- Forensique Git/GitHub et reconstruction de l'état exact du dépôt.
- Analyse statique C/C++/Fortran/Python/JavaScript/TypeScript/Shell et autres langages présents.
- Architecture logicielle et cartographie des dépendances.
- Analyse des chaînes d'appel et de causalité.
- Détection des stubs, mocks, smoke tests, placeholders, fallbacks, chemins morts et calculs décoratifs.
- Ingénierie inverse des pipelines d'exécution.
- Validation numérique et métrologie.
- Analyse des erreurs numériques, stabilité, convergence et conditionnement.
- CFD / Navier–Stokes / méthodes de projection / Poisson / CFL.
- Dynamique non linéaire et calcul d'exposants de Lyapunov.
- Analyse mathématique des invariants et des critères de validation.
- Vérification statistique et reproductibilité expérimentale.
- Audit de provenance des données et des résultats.
- Audit des logs, hashes, timestamps et chaînes de custody.
- Audit de compilation, édition de liens, ABI et dépendances.
- Analyse mémoire, concurrence, UB, overflow, durée de vie et erreurs C.
- Analyse de sécurité et surfaces d'attaque lorsque pertinentes.
- Analyse des notebooks, scripts d'exécution et environnements embarqués.
- Analyse des artefacts binaires et distinction source / build / résultat.
- Audit documentaire contradictoire : comparaison rapports ↔ code ↔ logs ↔ résultats.
- Analyse des contradictions inter-versions et des duplications.
- Audit de reproductibilité : capacité d'un tiers à reconstruire exactement une affirmation.
- Analyse des biais de validation et des critères pouvant laisser passer un calcul faux.
- Analyse des tests positifs, négatifs, limites, adversariaux et de non-régression.

## 2. Règle fondamentale de preuve

Chaque affirmation importante devra être classée selon une chaîne de preuve :

source → fonction/module → appel réel → données d'entrée → transformation → sortie → mesure → log/artefact → rapport → conclusion.

Une étape manquante ne sera pas implicitement considérée comme vraie.

C'est-à-dire : la présence d'une fonction « solveur Navier–Stokes » prouve seulement l'existence du code. Elle ne prouve pas que le pipeline testé l'appelle. L'appel ne prouve pas que ses sorties sont utilisées. Une sortie ne prouve pas qu'elle est physiquement correcte. Un log ne prouve pas à lui seul que le calcul annoncé a produit la valeur loggée.

## 3. Niveau d'inspection demandé

L'inspection doit couvrir, sans exception conceptuelle :

1. tous les fichiers source ;
2. tous les headers ;
3. tous les scripts ;
4. tous les notebooks ;
5. tous les fichiers de configuration ;
6. tous les manifests ;
7. tous les fichiers de données utilisés par les calculs ;
8. tous les logs et résultats lorsque leur contenu participe à une affirmation ;
9. tous les rapports Markdown ;
10. tous les rapports non-Markdown ;
11. tous les fichiers de spécification et prompts lorsqu'ils définissent le comportement attendu ;
12. tous les tests ;
13. tous les benchmarks ;
14. tous les exécutables et artefacts binaires lorsqu'ils sont nécessaires pour déterminer ce qui a réellement été exécuté ;
15. tous les wrappers et adaptateurs ;
16. tous les chemins fallback ;
17. tous les mécanismes de génération ou import de résultats ;
18. tous les scripts qui transforment ou post-traitent les résultats.

Les dépendances tierces embarquées seront séparées du code propriétaire du projet, mais elles ne seront pas ignorées : leur présence, version, modification éventuelle et influence sur les résultats seront contrôlées.

## 4. Inventaire initial du dépôt distant

À la révision auditée, l'arbre Git récursif contient :

- 13 363 fichiers ;
- 1 787 répertoires ;
- environ 1 048 609 887 octets de fichiers ;
- 4 038 fichiers Python ;
- 1 859 fichiers Markdown ;
- 1 437 fichiers C ;
- 804 headers C ;
- 1 499 fichiers sans extension ;
- 584 fichiers texte ;
- 454 JSON ;
- 266 JSONL ;
- 132 fichiers de données MATLAB ;
- 87 scripts Shell ;
- 82 fichiers Lean ;
- 76 notebooks Jupyter ;
- 57 fichiers C++ ;
- 50 fichiers Fortran 90.

Cet inventaire est une photographie du HEAD distant contrôlé. Il ne signifie pas que les 13 363 fichiers sont tous du code constitutif : le dépôt contient également des environnements Python, bibliothèques embarquées, binaires, données, archives, logs et résultats historiques.

## 5. Première anomalie structurelle : pollution du périmètre

Le dépôt contient notamment un environnement .venv-ibm avec des packages tiers et leurs tests.

Il contient également des archives, exécutables, notebooks, images TIFF, résultats Kaggle, fichiers ZIP, objets compilés, gros logs et artefacts de résultats.

Problème :

Une inspection naïve « tous les fichiers » peut produire une fausse impression de complexité et peut surtout mélanger :

- code du projet ;
- dépendances tierces ;
- artefacts d'exécution ;
- anciennes versions ;
- résultats ;
- preuves ;
- copies ;
- données de référence ;
- fichiers de démonstration.

Solution :

Construire une classification forensic par rôle :

A. code constitutif ;
B. code de test ;
C. code d'outillage ;
D. code de validation ;
E. dépendances vendoriées ;
F. données d'entrée ;
G. résultats ;
H. logs ;
I. artefacts binaires ;
J. documentation ;
K. historiques/copies ;
L. fichiers désactivés ou expérimentaux.

Cette classification permettra ensuite de vérifier qu'un résultat présenté comme issu du système ne provient pas simplement d'un artefact historique ou d'un calcul de test.

## 6. Première anomalie : anciens rapports prétendant déjà à l'exhaustivité

Le dépôt contient plusieurs anciens documents affirmant avoir réalisé des inspections « ligne par ligne », « ultra-exhaustives », « complètes » ou « 100 % ».

Un exemple important est l'ancien RAPPORT 134 situé à la racine, qui affirme une inspection complète et donne notamment une « authenticité globale » de 75 % sur huit modules.

Un autre ancien document affirme avoir inspecté tous les modules et tous les edge cases.

Ces affirmations ne seront pas reprises automatiquement.

C'est-à-dire : si un ancien rapport affirme qu'une fonction est authentique, l'audit actuel cherchera le code correspondant et déterminera si le mécanisme réellement exécuté correspond à cette description.

Une affirmation documentaire contradictoire avec le code sera classée comme divergence documentaire.

## 7. Collision de numérotation documentaire

La série actuelle RAPPORT/ contient les rapports 125, 126, 127, 128, 129, 130, 131 et 133, mais aucun rapport 134 dans ce répertoire au moment de la présente vérification.

En parallèle, des fichiers nommés RAPPORT_134_* existent à la racine du dépôt et plusieurs documents historiques mentionnent ce numéro.

Conséquence :

Le numéro 134 doit être utilisé ici comme nouveau rapport de la série RAPPORT/, et non comme validation d'un ancien document portant déjà ce numéro dans un autre emplacement.

## 8. Conflits Git encore détectables

Une recherche distante actuelle du marqueur « <<<<<<< » retourne neuf fichiers, dont notamment :

- compterendu.md ;
- RAPPORT-VESUVIUS/fxgfgfchgf ;
- RAPPORT_AUDIT_FINAL_NX46_VESUVIUS.md ;
- RAPPORT/131_AUDIT_REPRISE_ET_PLAN_FERMETURE_SCIENTIFIQUE_C1_C7_20261001.md ;
- RAPPORT_IAMO3/NX/REPONSE_FORMELLE_REVIEWER_2_NX39.1.md ;
- RAPPORT_IAMO3/NX/REPONSE_FORMELLE_REVIEWER_2_NX39 (copy).md ;
- RAPPORT/125_RAPPORT_RECONCILIATION_FORENSIQUE_ETAT_REEL_C1_C5_20261001.md ;
- RAPPORT V138 — Cartographie technologique exhaustive, écarts V125→V137 et plan d’intégration verrouillé.md ;
- le présent rapport 134.

Le dernier élément est attendu dans le présent document car la chaîne de recherche inclut le rapport lui-même lorsqu'il décrit ce marqueur ; il ne doit donc pas être interprété comme un conflit dans son propre contenu.

Les autres occurrences devront être inspectées directement.

Problème :

Un conflit Git non résolu dans un document de spécification ou de validation peut changer le sens d'une conclusion sans produire nécessairement une erreur de compilation.

Solution :

Traiter les conflits documentaires comme des anomalies de provenance et les séparer des conflits de code.

## 9. Premiers indicateurs de stubs et calculs non représentatifs

La recherche actuelle trouve six occurrences de STUB_MEASURED, dont le fichier :

src/tests/nx42_30_problems_execution_v2.c

ainsi que ses logs et rapports associés.

Le rapport 131 avait déjà établi que les problèmes 6–30 utilisent un calcul générique mesuré et que les problèmes 1–5 comportent encore des éléments explicitement désignés comme stubs.

Point forensic essentiel :

Une mesure de temps réelle sur un stub est une mesure réelle du temps du stub, pas une mesure réelle de la résolution du problème scientifique annoncé.

C'est-à-dire : le chronomètre peut être parfaitement exact tout en chronométrant le mauvais algorithme.

## 10. Premiers indicateurs de smoke tests et validations faibles

La recherche distante trouve de nombreux fichiers comportant le terme smoke, notamment dans les chaînes de validation et les documents de préparation de benchmark.

Un smoke test répond généralement à une question minimale : « le système démarre-t-il et produit-il une sortie ? »

Il ne répond pas nécessairement aux questions :

- la sortie est-elle mathématiquement correcte ?
- le calcul demandé a-t-il réellement été exécuté ?
- les paramètres physiques sont-ils corrects ?
- le résultat est-il convergent ?
- le résultat est-il reproductible ?
- une sortie fabriquée ou fallback pourrait-elle passer ?
- une autre implémentation pourrait-elle produire la même métrique sans réaliser le mécanisme annoncé ?

L'audit distinguera donc explicitement smoke test, test fonctionnel, test de correction, test numérique, validation physique et preuve de performance.

## 11. Première anomalie scientifique déjà connue à revalider

Le rapport 131 indique notamment :

- Lyapunov : une seule configuration expérimentale et incohérence epsilon commentaire/code ;
- T04 : comparaison de deux points d'énergie plutôt qu'un test de monotonie ;
- Richardson : étude actuelle insuffisante pour démontrer un ordre asymptotique ;
- C3 : présence du solveur NS sans preuve suffisante de son appel causal dans NX-42 ;
- C4 : problèmes 6–30 encore stubs ;
- NX-35 : valeur historique non reconstruite ;
- V138 : conflit documentaire.

Ces éléments seront repris dans l'audit 134, mais chacun sera réexaminé depuis le code et les artefacts actuels.

## 12. Critères de falsification recherchés

L'inspection cherchera explicitement les mécanismes capables de produire un résultat convaincant sans que la technologie revendiquée ait réellement exécuté le calcul.

Catégories :

### 12.1 Falsification par stub

Le programme exécute une fonction générique à la place de l'algorithme annoncé.

### 12.2 Falsification par mock

Une réponse artificielle remplace un composant réel.

### 12.3 Falsification par fallback silencieux

Une erreur ou absence de dépendance provoque l'utilisation d'un chemin alternatif non annoncé.

### 12.4 Falsification par données historiques

Un résultat antérieur est chargé ou recopié au lieu d'être recalculé.

### 12.5 Falsification par métrique

La métrique mesure autre chose que ce que son nom ou son rapport affirme mesurer.

### 12.6 Falsification par test insuffisant

Un test vérifie seulement deux points, un seuil trop permissif ou une propriété différente de celle annoncée.

### 12.7 Falsification par post-traitement

Le résultat est modifié, filtré, tronqué ou normalisé avant présentation sans que cela soit explicitement tracé.

### 12.8 Falsification par sélection

Seules les exécutions réussies ou favorables sont conservées.

### 12.9 Falsification par seed ou paramètres cachés

Le résultat dépend d'un paramètre non déclaré, d'une seed fixe ou d'un état résiduel.

### 12.10 Falsification par unité

Une grandeur est numériquement plausible mais exprimée dans une unité différente de celle annoncée.

### 12.11 Falsification par référence

Une référence trop faible, interpolée ou contaminée par la même implémentation est utilisée pour démontrer la correction.

### 12.12 Falsification par circularité

Le même code produit le résultat puis produit le test censé prouver ce résultat.

### 12.13 Falsification par compilation

Le fichier audité n'est pas celui réellement compilé ou exécuté.

### 12.14 Falsification par version

Le rapport cite une version différente de celle ayant généré les logs.

### 12.15 Falsification par artefact

Un binaire ou résultat précompilé est utilisé alors que son origine n'est pas démontrée.

## 13. Inspection mathématique

Pour chaque résultat scientifique, l'audit devra identifier :

- l'équation ou définition exacte ;
- les variables ;
- les unités ;
- les conditions initiales ;
- les conditions aux limites ;
- les paramètres ;
- le schéma numérique ;
- le pas spatial ;
- le pas temporel ;
- les tolérances ;
- le critère d'arrêt ;
- la norme utilisée ;
- la méthode de référence ;
- l'erreur de discrétisation ;
- l'erreur temporelle ;
- l'erreur spatiale ;
- l'erreur d'itération ;
- l'erreur d'arrondi ;
- la stabilité ;
- la convergence ;
- la sensibilité aux paramètres ;
- les cas où le résultat peut être numériquement plausible mais physiquement faux.

## 14. Inspection des chaînes d'exécution

Pour chaque fonctionnalité importante, établir :

entrée utilisateur ou dataset
→ parsing
→ configuration
→ initialisation
→ appel principal
→ sous-appels
→ calcul
→ accumulation
→ validation
→ sérialisation
→ log
→ rapport.

Toute rupture sera annotée.

Exemple :

« fonction solveur présente » ≠ « solveur exécuté » ≠ « solveur exécuté avec les bons paramètres » ≠ « sortie du solveur utilisée » ≠ « sortie correcte ».

## 15. Inspection des rapports

Les rapports seront traités comme des artefacts à auditer.

Pour chaque rapport significatif :

- quelle révision du code était disponible ?
- quelle révision est citée ?
- les fichiers mentionnés existent-ils ?
- les lignes citées existent-elles ?
- les résultats existent-ils ?
- les logs existent-ils ?
- les hashes correspondent-ils ?
- la date est-elle cohérente ?
- les valeurs peuvent-elles être recalculées ?
- le rapport contient-il des conclusions plus fortes que ses preuves ?
- le rapport contredit-il un autre rapport ?
- le rapport décrit-il une correction qui n'est pas présente dans le code actuel ?

Les anciens rapports ne seront donc ni ignorés ni crus automatiquement.

## 16. Inspection des résultats et logs

Les logs seront classés :

A. génération réellement démontrée ;
B. génération plausible mais provenance incomplète ;
C. résultat historique ;
D. résultat dérivé ;
E. résultat non reproductible ;
F. résultat contradictoire ;
G. artefact inutilisable comme preuve.

La présence d'un timestamp ou d'un hash ne sera pas considérée comme preuve suffisante de l'authenticité du calcul.

C'est-à-dire : un hash prouve l'intégrité du fichier hashé depuis le moment où le hash a été calculé, mais ne prouve pas que le contenu correspond au calcul scientifique annoncé.

## 17. Inspection des performances

Toute revendication de performance sera séparée en :

- temps de calcul ;
- temps d'I/O ;
- temps d'initialisation ;
- temps de compilation ;
- temps de transfert ;
- temps GPU/CPU ;
- nombre d'opérations ;
- débit ;
- latence ;
- mémoire ;
- parallélisme ;
- conditions expérimentales ;
- baseline de comparaison.

Une amélioration de performance ne sera pas considérée comme valide si elle provient d'un changement d'algorithme qui ne résout plus le même problème.

## 18. Inspection sécurité et intégrité

L'audit cherchera notamment :

- buffer overflow ;
- use-after-free ;
- double-free ;
- integer overflow ;
- signed/unsigned bugs ;
- data races ;
- deadlocks ;
- TOCTOU ;
- chemins de fichiers non contrôlés ;
- secrets embarqués ;
- clés ou tokens ;
- téléchargements non vérifiés ;
- commandes shell construites dynamiquement ;
- désactivation de vérifications ;
- assertions supprimées ;
- gestion d'erreur insuffisante ;
- comportements indéfinis ;
- dépendances compromises.

Ces problèmes seront reliés à leur impact sur la fiabilité des résultats, pas seulement à leur impact sécurité.

## 19. Inspection des tests

Pour chaque test :

Processus :
identifier ce qu'il teste réellement.

Problème :
déterminer si l'assertion teste la propriété annoncée ou seulement une condition triviale.

Solution :
définir le test minimal permettant de distinguer un vrai calcul d'un faux positif.

Exemples de faux positifs recherchés :

- sortie non vide ;
- code retour 0 ;
- valeur positive ;
- valeur sous un seuil arbitraire ;
- deux snapshots seulement ;
- comparaison avec une référence insuffisante ;
- test qui partage le même bug que le code ;
- test qui n'exécute jamais la branche critique.

## 20. Registre de vérité indépendant

Le rapport final devra produire un registre de type :

IDENTIFIANT
→ fonctionnalité annoncée
→ fichier
→ fonction
→ appel réel
→ preuve d'exécution
→ preuve mathématique
→ preuve numérique
→ preuve de résultat
→ niveau de confiance
→ anomalie éventuelle
→ risque de faux résultat
→ correction proposée
→ statut.

Les catégories de statut seront :

- PROUVÉ ;
- PROUVÉ PARTIELLEMENT ;
- NON PROUVÉ ;
- CONTREDIT PAR LE CODE ;
- STUB ;
- MOCK ;
- SMOKE ONLY ;
- ARTEFACT HISTORIQUE ;
- NON REPRODUCTIBLE ;
- BLOQUÉ PAR PREUVE MANQUANTE.

## 21. Règle de conclusion

Aucune « authenticité globale » ou « conformité à 100 % » ne sera calculée tant qu'une définition rigoureuse du périmètre et du dénominateur n'existe pas.

C'est-à-dire : annoncer « 95 % authentique » sur un système de milliers de fichiers sans définir précisément quelles fonctionnalités sont évaluées peut masquer une fonctionnalité critique fausse derrière une majorité de composants secondaires corrects.

L'audit privilégiera donc les affirmations atomiques et vérifiables.

## 22. Travaux déjà ouverts conservés

Les chantiers antérieurs restent actifs en parallèle :

- robustesse Lyapunov ;
- convergence numérique stricte ;
- définition correcte de T04 ;
- intégration causale ns_solver_2d dans NX-42 ;
- remplacement des stubs NX ;
- reconstruction NX-35 ;
- résolution des conflits Git ;
- réconciliation local/distant ;
- audit des anciennes conclusions d'authenticité.

Aucun de ces chantiers n'est considéré comme abandonné parce que la présente mission est plus large.

## 23. Limitation de preuve à ce stade

Le présent rapport est une phase d'établissement de vérité et de protocole.

Il ne prétend PAS que les 13 363 fichiers ont déjà été lus intégralement ligne par ligne dans cette seule itération.

C'est volontaire.

Une déclaration contraire serait elle-même une falsification de l'audit.

La méthode retenue est donc :

1. inventaire exhaustif ;
2. classification ;
3. extraction des chaînes critiques ;
4. lecture intégrale des fichiers constitutifs par lots ;
5. recroisement des appels ;
6. recroisement des résultats ;
7. recroisement des rapports ;
8. tests des hypothèses de falsification ;
9. consolidation ;
10. contre-audit des conclusions ;
11. publication des anomalies restantes.

## 24. Conclusion de phase 0

La première synchronisation montre déjà que le projet est suffisamment volumineux et historiquement hétérogène pour qu'un audit sérieux ne puisse pas être remplacé par une lecture superficielle ou par la confiance dans les rapports précédents.

Les premières preuves indiquent déjà des zones à risque réel :

- coexistence de code, dépendances et artefacts dans le même dépôt ;
- anciens rapports revendiquant une exhaustivité non encore démontrée par la présente inspection ;
- stubs explicitement mesurés ;
- smoke tests et validations minimales ;
- résultats historiques ;
- conflits Git documentaires ;
- divergence potentielle entre nom de test, propriété réellement testée et propriété scientifique annoncée ;
- chaînes d'intégration scientifique encore à démontrer ;
- anciennes conclusions qui doivent être confrontées au code actuel.

La suite de l'audit devra donc rechercher non seulement les bugs classiques, mais surtout les mécanismes qui permettent à un système de produire une sortie numériquement plausible alors que le calcul scientifique annoncé n'a pas réellement eu lieu.

Aucune modification de code scientifique n'est effectuée par ce rapport.
Seul un rapport d'audit est ajouté à la série RAPPORT/.

## 25. Exigence de clôture

La mission ne sera considérée comme terminée que lorsque chaque fonctionnalité revendiquée aura une chaîne de preuve suffisamment complète, ou sera explicitement classée comme non prouvée, partielle, stub, mock, smoke, historique ou contradictoire.

La vérité recherchée est donc la vérité du code et de son exécution, pas la vérité déclarative des rapports.
