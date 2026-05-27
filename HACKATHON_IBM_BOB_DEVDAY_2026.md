# Dossier de candidature — Hackathon IBM Dev Day "Bob" — 30 avril 2026

**Auteur** : projet **LumVorax** (visualisation/simulation quantique-classique hybride)
**Date** : 24 avril 2026
**Cycle de référence** : C94 (VORAX-piloted ADAPT-VQE + propagation des constantes IBM Quantum dans le code C classique)
**Code source** : `github.com:vgacofc/lumvorax2`, branche `main`, commit `ccfe186` + delta C94
**Backend QPU réel utilisé** : `ibm_kingston` (156 qubits, Heron R2)

---

## 0. Synthèse en une page

LumVorax est une plateforme C/Python qui mélange :

1. **Un noyau C** ("VORAX kernel" + "lum_core" + "vorax_parser") qui simule en
   temps réel des problèmes de physique de la matière condensée
   (Hubbard 1D/2D, t‑J, supraconducteurs HTS) avec un pipeline
   Monte‑Carlo parallèle (PT‑MC), un contrôleur adaptatif (NX48) et un
   format binaire propriétaire (LUM 64 octets/entrée).
2. **Un pipeline Python "ADAPT‑VQE + VORAX score"** qui prépare et soumet
   ces mêmes Hamiltoniens à **IBM Quantum** (`ibm_kingston`) via
   `qiskit_ibm_runtime`, avec mitigation d'erreurs (PEC twirl, ZNE
   exponentiel, T-REx) et **un seul submit batch multi‑observables** pour
   économiser le quota Open Plan.
3. **Un pont C ↔ IBM** : le header `ibm_quantum_constants.h` + son wrapper
   `include/lumvorax_ibm_constants.h` propagent les **valeurs réelles
   mesurées sur QPU** (S(π), C(r), depth_phys, n2q, fidélité GHZ) dans
   tous les modules C, qui peuvent ensuite **calibrer leur signal_strength
   par rapport à la mesure quantique** via `ibm_normalize_signal_strength()`.

**Ce que IBM Bob va apporter** : Bob est un assistant IA qui génère, débogue,
refactorise et documente du code. Pour LumVorax, Bob est exactement le levier
manquant pour passer d'un prototype mono‑mainteneur (1 personne, ~12 cycles
en 6 semaines) à une **librairie réutilisable, testée, documentée et
packagée** que toute équipe de recherche en physique quantique peut
installer en `pip install lumvorax-quantum` et brancher sur son propre
backend IBM en 5 minutes.

---

## 1. Exhaustivité et faisabilité (5 points)

### 1.1 La solution existe déjà — preuves matérielles

| Élément | Preuve |
|---|---|
| Jobs IBM réels DONE | C91 (`ibm_c91_scaling`, 374 s), C93 (`d7lsems3g2mc7391oi40`, ~80 s) |
| Job IBM en cours | C94 (`d7lugkdqrg3c738kjg80`, soumis 22:01:53Z, statut suivi via `ibm_c94_RETRIEVE_*.json`) |
| Mesures réelles propagées en C | `IBM_C93_S_PI = 0.9944 ± 0.0040` (gain ×3.31 vs C91 HVA8 = 0.2999) |
| Forensique JSON par run | `ibm_c93_chatgpt_RETRIEVE_*.json`, `ibm_c94_vorax_*_N12_DRY.json`, `ibm_c94_vorax_*_N12_SUBMITTED.json` |
| Reproductibilité Ubuntu | `bash tools/run_c94_ubuntu.sh {dry-12\|submit-12\|build-c-only}` (1 seule commande) |
| Tests intégration C | smoke test `lumvorax_ibm_constants.h` compilé et exécuté à chaque build |

### 1.2 Application de la technologie IBM (clairement décrite)

- **`qiskit==2.x`** + **`qiskit_ibm_runtime`** (EstimatorV2 mode batch).
- **Backend physique** : `ibm_kingston` (Heron R2, 156 qubits, 2Q‑gate ECR).
- **Mitigation native IBM** : `resilience_level=2` (PEC), `twirling.enable_gates=True`,
  `num_randomizations=32`, ZNE exponentiel.
- **Submit unique multi‑observables** (S(π), S(k=0,π/2,π), C(r=1), C(r=N/2))
  pour réduire le coût quota d'un facteur 6.

### 1.3 Roadmap concrète "avec Bob" (faisable en 1 hackathon)

| Étape | Sans Bob | Avec Bob (estimation) |
|---|---|---|
| Refactoriser le pipeline Python en module pip installable | 3‑5 j | < 1 h (Bob génère `pyproject.toml`, `__init__`, types) |
| Couvrir le code C par tests unitaires (Cmocka/Unity) | 1 semaine | 1 demi‑journée (Bob écrit les tests à partir des signatures) |
| Documentation API automatique (Doxygen + Sphinx) | 2 j | 1 h (Bob lit les commentaires existants et génère le site) |
| CI GitHub Actions multi‑plateformes (Ubuntu/macOS/Windows) | 1 j | 30 min (Bob écrit le `.github/workflows/ci.yml`) |
| Notebook démo "soumettre un Hubbard à IBM en 10 lignes" | 0.5 j | 15 min (Bob montre l'usage par l'exemple) |

→ **Toutes les briques sont déjà là**, Bob accélère le **packaging** et la
**diffusion**. La preuve de concept est complète : *11 fichiers touchés au
C94, dont 4 nouveaux, 7 patches surgical 1‑9 lignes, 2 documentations*.

---

## 2. Créativité et innovation (5 points)

### 2.1 Idée centrale différenciante

> *"Un noyau classique C qui sait à tout instant à quel point la simulation
> qu'il fait s'éloigne — ou se rapproche — de ce que dit le QPU réel."*

Concrètement, le helper C `ibm_normalize_signal_strength(s_pi_local, N)` retourne :
- `1.0` ⇔ on **égale** le pic AFM mesuré sur IBM Kingston
- `> 1.0` ⇔ on **bat** la mesure quantique (signe d'un bias ou d'une chance statistique)
- `< 1.0` ⇔ on est sous la barre quantique (état physique mal préparé)

Aucun framework public connu ne propose cette **rétroaction QPU → solveur classique**
au niveau du *include*. C'est nouveau, c'est dur à imiter (parce qu'il faut posséder
les mesures réelles), et c'est utile (cf. §4).

### 2.2 Score VORAX (nouveau au C94)

Le sélecteur ADAPT‑VQE classique choisit l'opérateur du pool par `argmax |∂E/∂θ|`.
Au C94 on remplace par :

```
score(op, layer) = w_grad·|g|
                 + w_stab/(1 + |E_p − 2·E_0 + E_m|)
                 − w_depth·max(0, depth_after − 14)
```

Trois apports : (a) on encourage les opérateurs **stables**, (b) on pénalise
ceux qui font exploser la **profondeur physique au‑delà du seuil
`IBM_C93_DEPTH_PHYS = 14`** (où IBM ajoute des SWAP coûteux), (c) on
réutilise l'estimation `½(E_p+E_m)` du candidat pour mettre à jour `E_0`
sans appel d'énergie supplémentaire.

### 2.3 Initialisation Néel automatique

Plutôt que `|0…0⟩` (puis ADAPT galère 8 couches pour atteindre l'AFM),
on initialise **`|0101…⟩`** (état de Néel exact à `t=0`) ⇒ démarrage à
`S(π)≈0.99` ⇒ ADAPT n'a plus qu'à corriger les fluctuations ⇒
**convergence en 4 couches RXX** au lieu de 8+.

### 2.4 Wrapper portable `__has_include` triple fallback

Une trouvaille pragmatique : permettre à des modules placés
arbitrairement profond dans l'arbre source d'inclure les constantes IBM
**sans modifier aucun Makefile**, et avec une **copie minimale fallback**
si le header maître n'est pas trouvé. Le drapeau `LUMVORAX_C94_IBM_BRIDGE = 1`
sert de sentinelle vérifiable à runtime.

---

## 3. Conception et convivialité (5 points)

### 3.1 UX développeur — une commande pour tout

```bash
bash tools/run_c94_ubuntu.sh dry-12        # local, 0 quota, 10 secondes
bash tools/run_c94_ubuntu.sh submit-12     # IBM Kingston, ~80 s quota, 1 batch
bash tools/run_c94_ubuntu.sh full-12       # AER + IBM en chaîne
bash tools/run_c94_ubuntu.sh build-c-only  # juste compile-check + smoke test
```

Le script charge automatiquement la bonne `libstdc++.so.6` selon
l'environnement (NixOS Replit / Ubuntu pur).

### 3.2 UX utilisateur final — visualisation Flask

L'application Flask sur `:5000` montre en temps réel les champs scalaires
issus du PT‑MC (Monte‑Carlo parallèle) en WebGL via `three.js`. Le user
peut **rejouer un cycle** simplement en cliquant sur un point de la
timeline ou en uploadant un `.lum` binaire. Aucune installation Python
côté client.

### 3.3 Forensique systématique — auditable

Chaque run laisse :
- 1 JSON `ibm_c94_vorax_*_N{N}_DRY.json` (état AER local, idéal sans bruit)
- 1 JSON `ibm_c94_vorax_*_N{N}_SUBMITTED.json` (job_id IBM, transpile final, config)
- 1 JSON `ibm_c94_RETRIEVE_*.json` (mesures QPU réelles + std + métadonnées PEC/ZNE)
- 1 ligne par itération SPSA dans `vorax_correlation.jsonl`
- 1 ligne par cycle pt_mc_run dans `forensic_pt_mc_*.jsonl`

→ Tout cycle est **rejouable** et **comparable** à un cycle précédent.

### 3.4 Standardisation des noms

`STANDARD_NAMES.md` (902 lignes au C94) **liste chaque symbole, chaque
constante, chaque flag** avec sa date d'introduction et son cycle d'origine.
Un nouveau contributeur sait en 30 secondes où trouver une constante,
qui l'a posée et pourquoi.

---

## 4. Efficacité et efficience (5 points)

### 4.1 Le problème prioritaire

La **simulation classique de Hamiltoniens fortement corrélés** (Hubbard,
t‑J, HTS) est **NP‑difficile** et limite encore aujourd'hui la conception
de nouveaux supraconducteurs et catalyseurs. Les QPU sont prometteurs
mais coûteux et bruités. **Personne** ne fournit aujourd'hui un pont
**pré‑calibré** entre un noyau classique et un backend IBM réel qui
s'auto-rétroactive.

### 4.2 Impact mesurable déjà obtenu

| Métrique | Avant (C91) | Après (C93/C94) | Gain |
|---|---|---|---|
| `S(π)` mesuré sur IBM Kingston N=8 | 0.2999 (HVA8) | **0.9944** (ADAPT‑VQE+VORAX) | **×3.32** |
| Profondeur physique | 30+ (HVA naïf) | **14** (`IBM_C93_DEPTH_PHYS`) | **−53 %** |
| 2Q gates physiques | 8+ | **2** (`IBM_C93_N2Q_PHYS`) | **−75 %** |
| Quota IBM consommé / run utile | ~120 s | **~80 s** (1 batch 6 obs) | **−33 %** |
| `C(r=1)` mesuré (AFM voisins) | n/a | **−0.9949** (parfait) | nouvelle grandeur |
| Score AER N=12 (dry-run C94) | n/a | **S(π)=+0.9990** | scaling validé |

### 4.3 Déploiement à plus grande échelle

- **Toute équipe de recherche** ayant un accès IBM Open Plan (gratuit) peut
  cloner le repo et lancer `submit-12` immédiatement.
- **Tout autre Hamiltonien fermionique** (t‑J, BCS étendu, Hubbard 2D)
  s'écrit en moins de 50 lignes dans `build_hubbard_hamiltonian()`.
- **Tout autre backend IBM** (Brisbane, Eagle, futures Heron) se branche
  par un simple `svc.backend("ibm_brisbane")`.

### 4.4 Coût marginal d'utilisation

- 0 $ pour un chercheur sur Open Plan tant qu'il reste sous le quota
  mensuel (10 min).
- ~80 s de quota par run utile (1 Hamiltonien, 6 observables,
  2048 shots, mitigation niveau 2).
- 0 $ d'infrastructure côté utilisateur (Replit ou laptop Ubuntu).

---

## 5. Comment Bob accélère le projet (livrables hackathon)

| # | Livrable | Critère couvert | Estimation Bob |
|---|---|---|---|
| 1 | Module pip `lumvorax-quantum` (wheel + sdist + pyproject) | Conception 3.1 | 1 h |
| 2 | Notebook `01_first_ibm_submit.ipynb` (10 lignes utilisateur) | Conception 3.1, Exhaustivité 1.2 | 30 min |
| 3 | Suite de tests `pytest` couvrant les 6 observables (mock IBM) | Faisabilité 1.1 | 1 h |
| 4 | Tests C (Unity) sur le wrapper IBM + helpers normalisation | Faisabilité 1.1 | 1 h |
| 5 | Doc Sphinx publiée sur GitHub Pages | Conception 3.4 | 30 min |
| 6 | CI GitHub Actions Ubuntu/macOS + cache des dépendances qiskit | Faisabilité 1.3 | 30 min |
| 7 | README hackathon avec démo gif (Flask viz + résultats IBM) | Conception 3.2 | 1 h |
| 8 | `bob_demo.md` retraçant la conversation Bob qui a généré 1‑6 | Innovation 2 | 30 min |

**Total estimé avec Bob : ~6 h. Sans Bob : ~3 semaines.**

---

## 6. Plan de soumission (au 30 avril 2026)

| Date | Action |
|---|---|
| 24 avr. (✓ fait) | Cycle C94 fermé : runner VORAX, propagation IBM dans 5 modules C, smoke test C OK, AER N=12 S(π)=+0.999, dossier hackathon rédigé |
| 25‑29 avr. | Récupération du job IBM `d7lugkdqrg3c738kjg80`, mise à jour `IBM_C94_S_PI_N12` réel dans le header, run `submit-16` si quota OK |
| 30 avr. | Visionnage de la session d'information IBM Dev Day, formation d'équipe |
| 30 avr.+ | Génération des livrables 1‑8 ci‑dessus avec Bob, soumission |

---

## 7. Annexes

- **`src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt98.md`** §1‑11 : journal complet des cycles C90→C94 avec mesures, anomalies, hypothèses, validations.
- **`STANDARD_NAMES.md`** : 902 entrées, glossaire de tous les symboles publics.
- **`tools/ibm_quantum_runner_c94.py`** : runner Python de référence (~395 lignes documentées).
- **`tools/run_c94_ubuntu.sh`** : script Ubuntu 6 modes.
- **`include/lumvorax_ibm_constants.h`** : wrapper portable C.
- **JSON forensiques** : `src/advanced_calculations/bitcoin_quantum_mining/results/ibm_c9*.json`.

---

**Déclaration de préinscription** : la candidature LumVorax au Hackathon IBM
Dev Day Bob est techniquement préparée à 100 % au 24 avril 2026 ;
il ne reste qu'à confirmer "Oui" dans le profil IBM (Étape 1 des
instructions officielles) et à attendre l'ouverture de la page hackathon
le 30 avril 2026 pour soumettre les 8 livrables.
