# Audit honnête — réponses aux questions du 23 avril 2026

> **Méthode** : grep/find sur le code source réel (`*.c`, `*.h`, `*.py`, `*.cpp`),
> **pas** lecture des rapports markdown `CHAT/*.md` (ces derniers sont des
> analyses produites par des LLM, pas une preuve d'implémentation).
>
> **Limite** : aucun accès au matériel IBM Quantum réel depuis cet environnement
> Replit. Toute affirmation sur des runs IBM est non vérifiable ici.

---

## 1. Couverture VORAX dans le code source

| Module | Fichiers contenant `VORAX` ou `vorax_` | Verdict |
|---|---:|---|
| `src/qdayprize/` | **0** | ❌ Non appliqué |
| `src/advanced_calculations/bitcoin_quantum_mining/src/` | **7** | ⚠️ Partiel (mentions seulement, à auditer ligne-à-ligne) |
| `src/advanced_calculations/quantum_problem_hubbard_hts/` | 730 | ✔️ Très étendu |
| `src/parallel/` | 3 | ⚠️ Minimal |
| `src/nx_versions/` | 4 | ⚠️ Minimal |

**Réponse directe à votre question** : *« le vorax est-il appliqué à tous les
16 problèmes ? »* → **NON**. Le code `qdayprize` n'a aucune référence VORAX.
Le code BTC mining n'a que 7 fichiers avec une mention. Seul Hubbard HTS est
réellement vorax-isé.

---

## 2. « 16 problèmes ADVANCED PARALLEL »

```
grep -rn "16.problem|n_problems.*16|NUM_PROBLEMS" src/ --include="*.c" --include="*.h" --include="*.py"
→ aucun résultat
```

**Verdict** : la liste « 16 problèmes » n'existe **que dans les rapports
markdown** (`CHAT/RAPPORT_IBM_QUANTUM_C64..C90`). Aucune structure de données
ni constante du code source ne définit ces 16 problèmes. Il faut soit :

- (a) extraire la liste depuis un des markdown et la **codifier** dans un
  fichier de config (`config/16_problems.csv` par exemple),
- (b) ou abandonner ce vocabulaire si ce n'était qu'une narration.

**À décider par vous.**

---

## 3. NX ATOM — état réel

```
grep -rln "NX_ATOM|nx_atom" src/ --include="*.c" --include="*.h" --include="*.py" --include="*.cpp"
→ aucun résultat
```

**Verdict** : **NX ATOM n'existe pas dans le code source**. Toutes les
mentions sont dans les rapports markdown CHAT. Donc :

- ❌ NX ATOM ne contrôle **rien** dynamiquement.
- ❌ « fidélité 100% » n'est pas un objectif implémenté.
- ❌ Les « paramètres ré-ajustés simultanément en temps réel » n'existent
  qu'en narration.

**Pour rendre NX ATOM réel**, il faut spécifier :
1. Quels paramètres exactement (noms, plages, types) ?
2. Quels objectifs mesurables (fidélité ⟨ψ|ψ_target⟩, énergie variationnelle,
   gap, etc.) ?
3. Quel solveur d'ajustement (Adam, CMA-ES, SPSA — adaptés au quantique) ?
4. Quelle boucle de feedback (latence acceptable, source des mesures) ?

**Sans ces 4 réponses, aucun code honnête ne peut être écrit.**

---

## 4. Format `.lum` appliqué à VORAX

```
grep -rln ".lum|LUM_FORMAT" src/qdayprize/ src/advanced_calculations/bitcoin_quantum_mining/src/
→ 3 fichiers BTC mining
→ 0 dans qdayprize
```

**Verdict** : format `.lum` partiellement utilisé côté BTC mining
(`nx48_btc_controller.[ch]`, `main_btc_mining.c`), absent de qdayprize.
**Pas de propagation systématique.**

---

## 5. Records IBM Quantum / QDayPrize / Willow

**Réponse directe** : **je ne peux ni confirmer ni infirmer** un quelconque
record. Raisons :

- Pas d'accès IBM Quantum depuis cet environnement (pas de `IBM_QUANTUM_TOKEN`
  configuré, pas de soumission possible).
- Pas de données brutes signées (logs forensiques cross-backend) accessibles
  ici pour une comparaison Willow / IBM / autres.
- L'analyse critique externe que vous avez collée précédemment **conclut
  explicitement** que :
  > « ⚠️ tu es dans un régime de corrélations de circuit, pas de phase
  > physique confirmée » et « ❌ affirmation "AFM mesuré" est prématurée ».

Je m'aligne sur cette conclusion. Aucune affirmation de record battu ne
serait honnête sans :
1. Soumission officielle QDayPrize → numéro de ticket / hash de soumission
2. Logs IBM signés (`job_id`, backend, calibration, timestamps)
3. Scaling N (8 → 16 → 24 → 32 qubits) avec collapse universel
4. Cross-backend (`ibm_fez` vs `ibm_brisbane` vs `ibm_kingston`)

---

## 6. Optimisations GPU encore possibles sur Ubuntu pour BTC

Actions concrètes possibles (sans matériel ici, juste préparation code) :

| Piste | Gain estimé | Effort |
|---|---|---|
| Kernel CUDA SHA-256 batched (occupancy 100%) | x2-3 vs naïf | moyen |
| Kernel OpenCL pour AMD/Intel iGPU | x1-2 | élevé |
| AVX-512 SHA-NI sur CPU récent | x4-8 vs scalaire | faible |
| Pipelining nonce+midstate (ASIC-style) | x1.3 | moyen |

**Mais** : impossible de mesurer sans GPU réel. Il faudrait que vous
exécutiez les benchmarks sur votre machine Ubuntu et reveniez avec les
chiffres.

---

## 7. Logs forensiques « neurone atom par atom »

```
find src -iname "*neuron*atom*" -o -iname "*atom*neuron*"
→ aucun fichier
```

**Verdict** : ce module n'existe pas dans le code. Comme NX ATOM,
narratif uniquement.

---

## 8. Tests de tresses VORAX vs Willow Google

Aucun fichier `tresse*` ou `braid*` trouvé dans le code. Aucune mesure
« calculs/sec vs Willow » réalisée. **Affirmation impossible.**

---

## 9. Ce qui A été fait dans cette session (Lot A — vérifiable)

| Élément | Statut | Fichier |
|---|---|---|
| Couleurs console (vert records, rouge anomalies) | ✔️ Fait | `src/visualization/console_banner.py` |
| Bannière version git auto-mise à jour à chaque run | ✔️ Fait | `src/visualization/console_banner.py` + `main.py` |
| Affichage commit court + branche + sujet + auteur + date au boot | ✔️ Fait | visible dans logs `Start application` |
| Helpers `print_record()` / `print_anomaly()` exportés | ✔️ Fait | utilisables depuis n'importe quel module Python |

**Test live** (extrait des logs après restart) :
```
==============================================================================
LUM/VORAX  -  Visualization Server
==============================================================================
Version  : f1c93b6 [DIRTY]  (main)
Last push: Add a detailed critical analysis of a scientific result
Pushed   : 2026-04-23 19:50:29 +0000  by supermicro20217
Boot     : 2026-04-23 19:52:04
==============================================================================
```

---

## 10. Questions d'experts non posées qu'il faudrait clarifier

1. **Reproductibilité** : seed fixée, version qiskit/numpy gelée, hash du circuit
   transpilé ?
2. **Calibration drift** : quel intervalle entre calibration backend et job ?
3. **Mitigation** : ZNE order, extrapolation (linéaire/exp), nb de noise factors ?
4. **Estimateur** : EstimatorV2 options (`default_precision`, `dynamical_decoupling`) ?
5. **Soumission QDayPrize** : règles officielles 2026 lues ? Format de submission ?
6. **Propriété intellectuelle** : code publié sous quelle licence ? (rien dans le repo)
7. **Sécurité crypto** : SHA-256 « LumVorax » audité par tiers ? (sinon usage BTC dangereux)

---

## 11. Décision requise de votre part

Pour avancer **honnêtement**, choisissez :

- **(A)** Codifier la liste des 16 problèmes en `config/16_problems.csv` + écrire
  un harness qui itère VORAX dessus (qdayprize + BTC + Hubbard).
- **(B)** Spécifier NX ATOM (4 questions section 3) pour qu'il devienne du code.
- **(C)** Configurer `IBM_QUANTUM_TOKEN` ici pour tenter des runs réels (très
  limités en temps gratuit, et **vous validez chaque soumission**).
- **(D)** Auditer ligne-à-ligne les 7 fichiers BTC mining qui mentionnent VORAX
  pour produire un rapport « VORAX réellement utilisé vs juste mentionné ».

**Je ne ferai rien d'autre sans votre choix** — pour ne pas inventer.
