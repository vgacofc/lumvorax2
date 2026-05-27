L'utilisateur veut :

Corriger immédiatement les bugs identifiés (BUG-CACHÉ-01, 02, 03, 04)
Comprendre et corriger la disparition de la granularité step-by-step dans les logs
Implémenter toutes les corrections du rapport 27
Je dois d'abord analyser en profondeur le bug de granularité des logs, puis corriger tous les bugs. Laissez-moi lire le code source pour comprendre où se trouve le logging step-by-step.
Je lis en profondeur le code source pour identifier la cause exacte de la disparition du logging step-by-step avant toute correction.
J'ai trouvé le problème du logging step-by-step. Voici ce que je vois :

Dans hubbard_hts_research_cycle.c (code classique) :

Ligne 341 : if (trace_csv && step % 100 == 0) → logging tous les 100 steps
Ligne 348 : (unsigned long long)step, → numéro du step
Dans hubbard_hts_research_cycle_advanced_parallel.c (code advanced) :

Ligne 401 : if (trace_csv && step % 100 == 0) → logging tous les 100 steps
Ligne 408 : (unsigned long long)step,
Dans la section PT-MC :

Ligne 704 : seulement 7 points enregistrés sur 4000 sweeps au total
Le vrai problème, c'est que le logging fullscale se déclenche tous les 100 steps, ce qui signifie qu'on ne capture que 7 points pour 700 steps au lieu d'avoir une couverture plus dense.

Pour PT-MC, c'est encore plus limité — seulement 7 sweeps enregistrés sur 4000 disponibles. L'utilisateur veut vraiment du logging à chaque étape et à chaque sweep, pas juste ces points épars. Je dois supprimer la condition step % 100 == 0 pour activer le logging continu, et remplacer le tableau de sweeps limité par une approche qui capture chaque sweep.

Côté volume de données, fullscale génère environ 27 300 lignes (gérable), tandis que PT-MC avec tous les sweeps atteindrait 364 000 lignes — un fichier d'environ 40 Mo. L'utilisateur a clairement dit que la consommation de ressources n'est pas une préoccupation, donc je peux procéder avec ces modifications sans retenue.

Maintenant je dois corriger plusieurs bugs cachés dans le code : d'abord remplacer la valeur finale d'appairage par la moyenne correcte, puis ajouter un recalcul d'énergie après chaque clipping, changer un message d'erreur fatal en avertissement dans le logger, et enfin modifier la condition de logging pour enregistrer chaque étape au lieu de sauter les étapes intermédiaires.
Trouvé ! Deux violations de granularité distinctes. Je lis les sections exactes pour corriger tout en parallèle.
J'ai maintenant tous les éléments. Voici le diagnostic complet :

BUG GRANULARITÉ STEP-BY-STEP — Causes exactes :

step % 100 == 0 dans hubbard_hts_research_cycle_advanced_parallel.c (ligne ~401) et hubbard_hts_research_cycle.c (ligne ~341) :
Dans le code classique : logging tous les 100 steps → pour 2100 steps, on voit 21 points
Dans le code advanced : logging tous les 100 steps → pour 2100 steps, on voit 21 points
CORRECTION : supprimer la condition step % 100 == 0 → log tous les steps
record_sweeps[7] = {0, 200, 500, 1000, 1500, 1800, N_SW - 1} dans le PT-MC :
Seulement 7 sweeps enregistrés sur 4000 → résolution très faible
CORRECTION : enregistrer tous les sweeps de production
Je dois maintenant examiner les autres corrections nécessaires, notamment le bug caché à la ligne ~829. Je vois le problème : le clipping modifie d_new après l'acceptance mais avant de l'assigner à d_rep[r][idx], ce qui crée une désynchronisation entre l'énergie accumulée dans E_rep[r] et l'état réel du système. Je dois recalculer l'énergie locale exacte après l'assignment du clipping pour corriger cette incohérence.

Cependant, puisqu'une recalculation complète de E_rep[r] se fait à la fin de chaque sweep, le biais n'existe que pendant le sweep et est corrigé après. Le vrai problème est que la décision Metropolis utilise le dE calculé avant le clipping, mais l'énergie réelle après clipping peut être très différente — si d_new est clippé à 1.0, l'énergie locale avec cette valeur clippée peut diverger significativement de celle de la proposition originale, ce qui invalide l'acceptance Metropolis. E < 0), mais on applique en fait d=1.0 (qui peut avoir une énergie encore plus basse ou plus haute). C'est un biais dans l'algorithme Metropolis.

La correction correcte est de clipper d_new avant de calculer dE, plutôt qu'après. Ça garantit que la décision Metropolis se fait avec l'énergie exacte du nouvel état, éliminant le biais introduit par le clipping.

Maintenant je dois identifier les trois corrections principales à faire dans le code : d'abord activer les logs de diagnostic, puis corriger l'enregistrement des sweeps pour capturer tous les points de données, ensuite corriger le calcul de l'appairage en divisant par le nombre de points plutôt que de laisser la somme brute, et enfin appliquer le clipping avant le calcul d'énergie dans les boucles de simulation.
Diagnostic complet identifié. Je lis maintenant les sections exactes des 4 bugs avant correction simultanée.
Parfait. J'ai maintenant toutes les informations pour faire les corrections :

BUG GRANULARITÉ : step % 100 == 0 dans les deux fichiers → supprimer la condition
BUG GRANULARITÉ PT-MC : record_sweeps[7] → enregistrer tous les sweeps
BUG-CACHÉ-02 : final_pairing_cold → chi_pair_sum / (double)chi_n
BUG-CACHÉ-03 : Clipper d_new AVANT dE (dans thermalisation et production)
BUG-CACHÉ-04 : sign_ratio_extreme devrait être un avertissement plutôt qu'une erreur fatale — je dois vérifier si c'est dans le script shell ou la macro de logging plutôt que dans le code C lui-même.
Pour la correction 3, le problème clé est que le clipping des valeurs se fait après l'acceptation du pas Monte Carlo, ce qui crée une incohérence. Il faut appliquer le clipping avant de calculer l'énergie avec la nouvelle valeur, pas après.

Maintenant je regarde la correction 1 sur la granularité du logging...

Removing the modulo condition so every sweep gets logged instead of just every 100th step, then looking at correction 2 which simplifies the sweep recording logic by eliminating the hardcoded array of specific sweeps and just logging all of them directly...

Now I need to locate these exact lines in the source files and apply the fixes, starting with the main binary in hubbard_hts_research_cycle_advanced_parallel.c since that's the critical one being used. Je dois aussi corriger le clipping dans la phase de production, puis m'attaquer aux bugs cachés dans le script shell et le logger. Après les corrections critiques, j'ajouterai les accumulateurs pour la variance et la capacité thermique.

Modifying the code edits... I need to move the clipping operation before the energy calculation in the thermalisation section, then apply the same fix to the production loop. I'm also removing the now-unused record_sweeps variable and adding accumulators for bootstrap jackknife calculations and heat capacity. Let me work through these edits systematically and locate the corresponding loop in the other simulator.

Now I'm tracking down where sign_ratio_extreme triggers the abort condition—the forensic logging module flags the anomaly, and the shell script checks the binary's exit code, so I need to find where that exit(1) is being called and understand the flow.
Diagnostic complet. Je fais maintenant toutes les corrections simultanément en lisant les sections exactes.