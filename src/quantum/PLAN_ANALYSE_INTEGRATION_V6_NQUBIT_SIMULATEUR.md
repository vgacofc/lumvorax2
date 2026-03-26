# Plan d'analyse et d'implémentation — Intégration NQubit V6 dans `src/quantum`

## 0) But de ce plan
Tu as demandé un plan **avant implémentation complète** pour :
1. copier/adapter V6 dans `src/quantum`,
2. comparer **avant copie** vs **après copie**,
3. ajouter des tests unitaires + stress + intégration,
4. enrichir les logs en profondeur nanoseconde,
5. mesurer performances (ops/s, qubits simulés/s, nqubits simulés/s),
6. détecter anomalies et opportunités "nouvelles".

Ce document est la feuille de route exécutable pour lancer ce chantier proprement.

---

## 1) État initial identifié (temps réel)

## 1.1 Modules `src/quantum` actuels
- `src/quantum/v_kernel_quantum.c`
- `src/quantum/v_kernel_quantum.py`

Constat rapide :
- le module est encore orienté prototype (simulation pseudo-aléatoire, logs basiques),
- pas de pipeline forensic complet type V6,
- pas de matrice de tests unifiée dans `src/quantum`.

## 1.2 Bloc V6 source à intégrer
Source candidate :
- `src/projetx_NQubit NX/NQubit_v6/`

Atouts V6 à réutiliser :
- pipeline en 6 étapes (collect, acquire, stress, manifest, verify, report),
- résultats structurés JSON/CSV/log,
- manifeste + signature,
- tests Python déjà présents.

---

## 2) Stratégie d'intégration (sans casser l'existant)

## 2.1 Principe "copie contrôlée"
On ne modifie pas directement les fichiers V6 source.

On crée une copie de travail dédiée :
- `src/quantum/nqubit_v6_integration/`

Cette copie servira de bac à sable pour :
- adapter les chemins,
- brancher l'observabilité nano-seconde,
- relier aux APIs du simulateur quantique local.

## 2.2 Compatibilité progressive
Ordre recommandé :
1. **Phase A** : miroir minimal (copie + exécution identique),
2. **Phase B** : instrumentation avancée (logs/metrics),
3. **Phase C** : extension "quantum simulator bridge",
4. **Phase D** : validation comparative complète.

---

## 3) Plan d'analyse avant codage (baseline "avant copie")

## 3.1 Mesures baseline à capturer
Avant toute copie, figer les métriques de l'état actuel `src/quantum` :
- temps moyen/run,
- événements loggés/s,
- taille de logs,
- stabilité (écart-type run-to-run),
- couverture tests existants (si présents),
- consommation CPU/mémoire.

## 3.2 Artefacts baseline à produire
- `src/quantum/results_baseline/baseline_summary.json`
- `src/quantum/results_baseline/baseline_trace_ns.log`
- `src/quantum/results_baseline/baseline_manifest.json`

Pourquoi :
- pour prouver noir sur blanc la différence avant/après.

---

## 4) Plan d'implémentation technique (après copie)

## 4.1 Architecture cible
Créer ces composants dans `src/quantum/nqubit_v6_integration/` :
1. `kernel_bridge.py` : pont entre simulateur quantique local et pipeline V6.
2. `metrics_ns.py` : compteur haute résolution (`time.time_ns` / `perf_counter_ns`).
3. `forensic_manifest.py` : hash + manifest + validation.
4. `stress_quantum.py` : stress multi-workers orienté charge simulation.
5. `report_compare.py` : rapport automatique avant/après.

## 4.2 Logging nanoseconde (profondeur maximale)
Format log recommandé (JSON Lines) :
- `ts_ns` : timestamp nanoseconde,
- `phase` : étape pipeline,
- `module` : source (`quantum`, `nqubit_v6_integration`, etc.),
- `event` : début/fin/opération,
- `duration_ns` : durée précise,
- `ops_count` : opérations traitées,
- `ops_per_sec` : débit,
- `qubits_simulated` : nombre de qubits simulés,
- `nqubits_simulated` : nombre de "NQubit events" simulés,
- `cpu_pct`, `mem_pct` : pression système,
- `checksum` : intégrité ligne (sha256 du payload).

## 4.3 Métriques clés obligatoires
- `ops_per_sec`
- `qubits_per_sec`
- `nqubits_per_sec`
- `latency_p50_ns`, `latency_p95_ns`, `latency_p99_ns`
- `error_rate`
- `reproducibility_score` (écart run-to-run)
- `integrity_ok` (manifest/verif)

---



## 4.4 Statut exact demandé : contrôle du bruit NX (réponse claire)
**Réponse courte : non, pas encore implémenté dans le code `src/quantum` au moment de ce plan.**

Ce fichier est un plan d'implémentation, pas l'implémentation finale.  
La logique de contrôle du bruit sur laquelle vous avez travaillé (analyse dissipation/jitter/Lyapunov) est donc :
- **prise en compte dans la stratégie**,
- mais **pas encore codée** dans le simulateur quantique final.

Référence d'analyse de bruit utilisée pour la suite :
- `RAPPORT_IAMO3/ANALYSE_POTENTIEL_HARDWARE_BRUIT_NX.md`

## 4.5 Intégration explicite prévue du contrôle du bruit NX
Pour lever toute ambiguïté, la future implémentation doit inclure ces briques :
1. **Noise Profile Engine**
   - mesure en temps réel : jitter ns, variance thermique proxy, drift de latence.
2. **Lyapunov Guardrail**
   - calcul d'un score de stabilité `V(x)` par fenêtre temporelle.
   - si dépassement de seuil : réduction dynamique de charge ou rollback.
3. **Stochastic Resonance Mode**
   - activation contrôlée du bruit utile (plage cible),
   - désactivation si bruit destructeur (hors enveloppe).
4. **Dissipative Memory Metrics**
   - suivi de persistance/hystérésis des états simulés.
5. **Hard Reset Merkle/Manifest**
   - si anomalie critique, snapshot signé + reprise état stable précédent.

## 4.6 Critères de validation spécifiques "contrôle du bruit"
L'intégration sera considérée effective seulement si :
- `noise_control_enabled=true` dans le rapport final,
- latence p95/p99 améliorée ou stabilisée sous stress,
- baisse du taux d'instabilité comparée au baseline,
- preuve d'activation des garde-fous Lyapunov dans les logs ns,
- test d'injection de bruit (faible/moyen/fort) passé sans corruption d'intégrité.


## 5) Plan de tests (unitaire, stress, intégration)

## 5.1 Tests unitaires
Ajouter :
- test des compteurs ns,
- test format logs JSONL,
- test calcul `qubits_per_sec`/`nqubits_per_sec`,
- test génération/validation manifest,
- test robustesse en cas de source hardware manquante.

## 5.2 Tests de stress
Scénarios :
- 1, 4, 16, 64 workers,
- durée 10s, 60s, 300s,
- charge CPU cible 70/90/99%,
- capture des percentiles de latence et des pics.

## 5.3 Tests d'intégration
- exécution pipeline complet,
- comparaison auto baseline vs post-intégration,
- validation que chaque artefact attendu existe,
- vérification signature/hash.

---

## 6) Plan de comparaison "avant copie" vs "après copie"

## 6.1 Rapport comparatif unique
Générer :
- `src/quantum/reports/COMPARAISON_AVANT_APRES_NQUBIT_V6_QUANTUM.md`

Avec tableaux :
1. performance,
2. stabilité,
3. intégrité,
4. volumétrie logs,
5. couverture tests,
6. anomalies.

## 6.2 Indicateurs de gain minimum (Go/No-Go)
Intégration validée seulement si :
- `integrity_ok = true`,
- `tests pass >= 95%`,
- `qubits_per_sec` et `nqubits_per_sec` non régressifs,
- latence p95 stable ou meilleure,
- aucune anomalie critique non expliquée.

---

## 7) Détection d'anomalies et "découvertes"

## 7.1 Anomalies à surveiller
- spikes latence ns incohérents,
- baisse brutale débit sous stress,
- divergence forte run-to-run,
- mismatch manifest/signature,
- métriques impossibles (ex: qubits/s négatif).

## 7.2 Pistes de découvertes potentielles
Sans prétendre à une découverte scientifique immédiate, on peut chercher :
- zones de performance non-linéaires selon nombre de workers,
- seuils de fragmentation des événements "nqubit" sous charge,
- corrélation jitter système ↔ stabilité simulation,
- signatures d'instabilité reproductibles multi-runs.

Sortie attendue :
- un chapitre "observations inédites" avec niveau de confiance (`faible/moyen/élevé`) et protocole de reproduction.

---

## 8) Risques et garde-fous
1. **Sécurité clés** : ne jamais commiter de clé privée active dans la copie d'intégration.
2. **Bruit de mesure** : distinguer anomalies système cloud vs anomalies algorithmiques.
3. **Dette logs** : trop de logs peut dégrader les perfs (prévoir mode sampling).
4. **Comparaison injuste** : normaliser hardware/contexte avant conclure.

---

## 9) Roadmap opérationnelle (ordre exact)
1. Capturer baseline `src/quantum` (mesures + manifest).
2. Créer `src/quantum/nqubit_v6_integration/` (copie contrôlée).
3. Faire tourner pipeline V6 "as-is" dans la copie.
4. Ajouter instrumentation ns avancée.
5. Ajouter suite de tests unitaire/stress/intégration.
6. Exécuter campagne comparative avant/après.
7. Générer rapport comparatif final + recommandations produit.

---

## 10) Livrables attendus (phase suivante)
- code d'intégration dans `src/quantum/nqubit_v6_integration/`,
- suite de tests automatique,
- logs ns détaillés,
- rapport comparatif avant/après,
- tableau anomalies + suggestions d'amélioration simulateur.

Ce plan est prêt pour passer en exécution technique au prochain cycle.
