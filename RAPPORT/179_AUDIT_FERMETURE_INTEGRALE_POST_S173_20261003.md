# Rapport 179 — Audit de fermeture intégrale post-S173 — aucune dette déclarée fermée sans preuve

Session : S174
Date : 2026-10-03
Dépôt : vgacofc/lumvorax2
Branche auditée : main
HEAD audité : 05623cf4601913b0d27de7fe9b4777880a30a517
Principe : aucun code source modifié dans ce rapport ; rapport documentaire uniquement.

## 1. Expertises activées
- Audit forensique Git/GitHub
- Analyse statique C99
- Concurrence POSIX / pthreads / condition variables
- Analyse des conditions de course et des réveils parasites
- Validation mémoire et durée de vie
- ThreadSanitizer / sanitizers
- Ingénierie CI reproductible
- Portabilité ISA et compilation sans extensions
- CFD / Navier–Stokes 2D
- Convergence spatiale et temporelle / Richardson
- Chorin splitting et stabilité temporelle
- Dynamique non linéaire / Lyapunov
- Forensic bit-level / IEEE-754 / provenance
- Blockchain / SHA-256 / sérialisation canonique
- Audit NX-42 et distinction calcul réel / STUB_MEASURED
- Traçabilité commit → source → exécution → log → rapport
- Gestion du registre de dette technique

## 2. Synchronisation Git
Le dernier commit observé sur main est 05623cf4601913b0d27de7fe9b4777880a30a517.
Message : Fix BUG-PARALLEL-001 : task_queue_dequeue shutdown flag — pthread_join deadlock fermé S173.
Le correctif annoncé est réellement présent dans le diff Git.
Le statut CI associé au commit présente actuellement un contexte Vercel = failure. Ce statut ne prouve pas l’échec du correctif C, mais il interdit de qualifier l’état global de CI comme entièrement vert.

## 3. BUG-PARALLEL-001 — correction principale confirmée
### Processus
Le correctif ajoute un état shutdown à task_queue_t, l’initialise à false, puis le positionne à true sous mutex avant le broadcast. task_queue_dequeue() attend désormais avec une condition qui inclut !shutdown.
La séquence de destruction devient : destroy → signal de sortie → shutdown sous mutex → broadcast → pthread_join.
Le run S173 produit un exit code 0 et une libération parallel_processor_t via parallel_processor_destroy().

### Problème
Le deadlock historique est effectivement supprimé dans le scénario testé.
Mais la nouvelle fonction contient encore une faiblesse de robustesse : après la boucle while (queue->head == NULL && !queue->shutdown), elle teste seulement si queue->head == NULL puis retourne NULL.
Or une condition variable POSIX peut produire un réveil parasite. Dans ce cas, le thread peut se réveiller alors que queue->head == NULL et shutdown == false, sortir de la boucle, retourner NULL et potentiellement quitter son chemin de worker alors qu’aucun shutdown n’a été demandé.
C’est-à-dire : le correctif ferme le deadlock démontré, mais la condition d’attente n’est pas encore formulée comme une preuve complète de protocole de condition variable.

### Solution
La condition de sortie normale doit être distinguée explicitement de la condition de shutdown. L’invariant doit rester : queue vide + shutdown=false → attendre à nouveau ; queue vide + shutdown=true → retourner NULL.
Il faut ensuite exécuter une campagne de non-régression sous ThreadSanitizer et avec répétitions élevées afin de vérifier absence de deadlock, réveil parasite mal interprété, data race et fuite de worker.
État : OPEN — sous-anomalie de robustesse à traiter avant certification globale.

## 4. BUILD-THREAD-001 — priorité immédiate
Processus : ThreadSanitizer détecte principalement les data races et certaines erreurs de synchronisation dans les programmes multi-threadés.
Problème : le rapport 178 ferme le deadlock observé par une exécution simple, mais aucune preuve TSan n’est fournie pour démontrer que le nouveau protocole de queue est exempt de course dans les différents interleavings.
Solution : construire une cible TSan indépendante et reproductible, puis tester création/destruction immédiate, queue vide, queue chargée, plusieurs workers, shutdown concurrent avec soumission, répétitions massives et destructions répétées.
Verdict attendu : zéro data race TSan, zéro deadlock et zéro fuite observable sur les scénarios testés.
État : OPEN.

## 5. BUILD-PROOF-001 — CI C reproductible
Processus : une CI reproductible transforme les preuves locales en vérifications exécutables automatiquement.
Problème : le statut global du commit 05623cf n’est pas entièrement vert et aucun pipeline C indépendant n’est établi comme preuve centrale.
Solution : compilation C99, warnings stricts, tests unitaires, intégration, ASan/UBSan, TSan, build portable, conservation des logs et verdict machine lisible.
État : OPEN.

## 6. BUILD-PORT-002 — portabilité ISA
Processus : une build portable doit compiler et s’exécuter sans dépendre d’instructions SIMD particulières.
Problème : un fallback scalaire ou un guard de compilation ne constitue pas une preuve d’exécution sur une cible réellement dépourvue de l’extension.
Solution : compilation sans AVX2/AVX-512, exécution réelle sur cible appropriée, mêmes résultats fonctionnels et vérification qu’aucune instruction interdite n’est utilisée.
État : OPEN.

## 7. FORENSIC-UNIF-002 — provenance bit-level
Processus : une preuve bit-level complète doit relier chaque bit d’entrée à un identifiant stable, à sa transformation et à son résultat, sans collision ni duplication.
Problème : les audits précédents ont établi que la couverture d’emplacements de bits ne suffit pas. Les limites portent notamment sur BIT_ID, LUM_ID, la portée des coordonnées et les timestamps synthétiques.
Solution : entrée → run_id → LUM_ID global → BIT_ID global → transformation → sortie → événement forensic, avec unicité globale, détection perte/duplication, valeur réelle du bit et timestamp réellement mesuré ou explicitement qualifié comme logique.
État : OPEN.

## 8. Richardson / Navier–Stokes
Processus : les travaux récents ont séparé plus proprement les erreurs spatiales et temporelles et obtenu des ordres spatiaux proches de 2 dans des protocoles dédiés.
Problème : cela ne ferme pas automatiquement l’ordre temporel du solveur Chorin complet. L’expérience Euler ODE et l’expérience spatiale sont des preuves de sous-composants ou de protocoles particuliers.
Solution : fermer séparément l’ordre temporel du Chorin complet, l’effet du splitting, la dépendance à dt, la stationnarité, le résidu Poisson, la divergence, L2/L∞ et la répétition sur plusieurs résolutions.
Une solution manufacturée reste la référence privilégiée lorsque la référence externe peut saturer l’erreur.
État : OPEN.

## 9. T04 — stationnarité
Processus : le critère actuel a été amélioré et ne repose plus seulement sur la positivité de deux énergies.
Problème : deux points finaux ne démontrent pas l’absence d’oscillation dans toute la fenêtre.
Solution : analyser toute la fenêtre finale avec min/max, moyenne, écart-type, pente, variation relative maximale et norme de variation d’état.
État : OPEN pour preuve stationnaire forte.

## 10. Lyapunov robuste
Processus : le signe négatif de λ a été observé avec plusieurs amplitudes, mais la robustesse quantitative n’est pas encore démontrée sur toute la plage annoncée.
Problème : un test sur epsilon et epsilon×10 ne suffit pas à certifier une plage complète ni la stabilité quantitative de λ.
Solution : balayer réellement plusieurs epsilon et répéter avec plusieurs warmups, intervalles de renormalisation, nombres de renormalisations et résolutions.
Il faut séparer robustesse du signe et robustesse de la valeur numérique.
État : OPEN.

## 11. Blockchain BL-003 → BL-012
Processus : la sérialisation canonique récente du header et les corrections BL-013/BL-015 ont renforcé le déterminisme et la non-régression.
Problème : les chantiers historiques BL-003 → BL-012 restent explicitement ouverts dans le registre S173.
Solution : auditer chaque numéro indépendamment avec définition du protocole, vecteur de référence, test positif, test négatif, non-régression, log brut et reproductibilité inter-machine.
État : OPEN.

## 12. C3 — NS réel → NX-42
Processus : un solveur Navier–Stokes réel existe dans le dépôt.
Problème : la présence du solveur n’implique pas son intégration scientifique correcte dans NX-42.
Solution : démontrer raccordement réel, état dynamique défini, conditions aux limites, conservation, convergence, stabilité, logs et non-régression, puis seulement benchmark et Lyapunov.
État : OPEN.

## 13. C4 — NX-42 problèmes 6–30
Processus : une mesure de temps peut être techniquement correcte même si le calcul mesuré est un stub.
Problème : les problèmes 6–30 restent qualifiés STUB_MEASURED.
C’est-à-dire : une latence mesurée correctement ne transforme pas une opération simplifiée en problème scientifique réellement résolu.
Solution : pour chaque problème, définir le problème scientifique, exécuter une implémentation réelle, produire le résultat, vérifier stabilité et convergence, conserver les logs, puis seulement mesurer la performance.
État : OPEN / STUB_MEASURED.

## 14. Dette technique globale
Aucune dette ci-dessus ne doit être masquée par un PASS local.
Principe de fermeture : code corrigé + test reproductible + preuve d’exécution + artefact conservé + critère explicite + absence de contradiction documentaire.
Un rapport seul ne constitue pas l’exécution. Un PASS isolé ne constitue pas une certification globale.

## 15. Verdict S174
BUG-PARALLEL-001 : correction principale validée, mais robustesse du protocole de condition variable encore à vérifier.
BUILD-THREAD-001 : OPEN.
BUILD-PROOF-001 : OPEN.
BUILD-PORT-002 : OPEN.
FORENSIC-UNIF-002 : OPEN.
Richardson temporel Chorin complet : OPEN.
T04 stationnarité forte : OPEN.
Lyapunov robuste : OPEN.
BL-003 → BL-012 : OPEN.
C3 NS → NX-42 : OPEN.
C4 NX-42 6–30 : OPEN / STUB_MEASURED.
CERTIFIED_100=false.
unique_human_proven=false.
Aucune certification 100 % ne doit être déclarée tant que ces preuves ne sont pas réellement produites.

## 16. Règle de continuation
Priorité immédiate : BUILD-THREAD-001 → correction/validation robuste de la condition variable → BUILD-PROOF-001 → BUILD-PORT-002 → FORENSIC-UNIF-002 → Richardson temporel Chorin → T04 → Lyapunov → BL-003→BL-012 → C3 → C4.
Les chantiers historiques restent persistants et ne sont pas supprimés par l’ouverture d’un nouveau chantier.