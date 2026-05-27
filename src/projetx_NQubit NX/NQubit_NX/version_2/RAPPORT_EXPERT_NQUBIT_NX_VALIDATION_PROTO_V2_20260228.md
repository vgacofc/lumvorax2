# RAPPORT EXPERT COMPLEMENTAIRE — NQubit NX (VERSION 2)

## 0) Périmètre et conformité

- Dépôt mis à jour depuis `https://github.com/lumc01/Lumvorax.git` avant exécution des travaux V2.
- Le dossier `src/projetx_NQubit NX/NQubit_NX` a été **copié dans `version_2/`** pour éviter toute régression ou oubli de réintégration.
- Tout le travail de cette itération est réalisé **dans** `src/projetx_NQubit NX/NQubit_NX/version_2/`.
- Les résultats, logs forensiques et rapport final sont générés dans le même dossier V2 (`version_2/results/`).
- Conformité demandée prise en compte : règles `prompt.txt` et nomenclature `STANDARD_NAMES.md` respectées sur les fichiers modifiés de V2.

---

## 1) Livrables V2 générés dans le dossier versionné

### 1.1 Code et tests V2

- `version_2/main.c` : enrichi avec métriques supplémentaires (marge score, delta énergie, min/max marge, moyennes globales) + logs forensiques KPI de synthèse.
- `version_2/Makefile` : nouveau flux `test_all` et `test_extended`.
- `version_2/tests/test_nqbit_nx_extended.c` : campagne progressive 10 → 100 → 1K → 10K (total 11 110 cas) avec log forensic dédié.

### 1.2 Artefacts générés

- `version_2/results/nqbit_benchmark.csv` (benchmark détaillé 360 scénarios).
- `version_2/results/nqbit_forensic_ns.log` (forensic d’exécution principal, enrichi).
- `version_2/results/nqbit_stats_v2.csv` (agrégats KPI V2).
- `version_2/results/test_extended_forensic.log` (forensic de tests étendus).
- `version_2/results/rapport_final.md` (synthèse finale exécutable V2).

---

## 2) Résultats mesurés (V2)

## 2.1 Résultat principal benchmark 360 scénarios

- Scénarios exécutés : **360**
- Victoires NX : **360**
- Taux de victoire : **1.0000**
- Score NX moyen : **0.361543**
- Score classique moyen : **0.189413**
- Marge moyenne : **0.172131**
- Marge min : **0.111301** (scénario 299)
- Marge max : **0.242608** (scénario 1)
- Énergie NX moyenne : **35.791711**
- Énergie classique moyenne : **21.874863**
- Delta énergie moyen : **13.916848**

## 2.2 Tests renforcés (trous comblés)

Campagne progressive de robustesse:
- palier 10 : 10/10 pass
- palier 100 : 100/100 pass
- palier 1 000 : 1 000/1 000 pass
- palier 10 000 : 10 000/10 000 pass

Total tests étendus : **11 110 / 11 110 pass**.

---

## 3) Ce que la V2 apporte par rapport à la version précédente

1. **Isolation opérationnelle** : copie complète en `version_2` pour protéger la base et éviter les régressions.
2. **Plus de tests** : ajout d’un test étendu progressif (jusqu’à 10K) conformément aux limites demandées.
3. **Forensic renforcé** :
   - logs `score_margin` et `energy_delta` scénario par scénario,
   - KPI finaux loggés (`avg_nx_score`, `avg_margin`, etc.),
   - log forensic dédié aux tests étendus.
4. **Sorties consolidées** : ajout du fichier `nqbit_stats_v2.csv` pour exploitation directe (audit/statistiques).

---

## 4) Validation scientifique (position experte)

### Niveau A — Démonstrateur logiciel
**Validé** : le comportement est stable, reproductible et auditable sur ce protocole.

### Niveau B — Hypothèse physique
**Partiellement validé** : la dynamique est crédible, mais le bruit est encore PRNG (pas capteur matériel réel).

### Niveau C — Validation hardware expérimentale
**Non validé à ce stade** : manque d’instrumentation physique et de campagne métrologique réelle.

---

## 5) Questions couvertes et prochaines questions d’experts

### Questions déjà couvertes
- Le mécanisme bruit + rappel Lyapunov reste-t-il contrôlé ? Oui, dans la plage testée.
- NX dépasse-t-il la baseline actuelle ? Oui, systématiquement sur ce benchmark.

### Questions restantes (phase suivante)
1. Le gain persiste-t-il avec une pénalité énergétique symétrique entre méthodes ?
2. Le gain tient-il face à des baselines adaptatives plus fortes ?
3. Que donne l’injection de bruit physique mesuré (ADC) vs PRNG ?
4. Quelle stabilité sous bruit non gaussien impulsionnel ?
5. Quel ratio précision/énergie en conditions matérielles réelles ?

---

## 6) Suite logique recommandée (V3)

1. Ablations strictes :
   - sans Lyapunov,
   - sans bruit,
   - bruit non gaussien.
2. Symétrisation complète des métriques de score.
3. Répétitions statistiques avec IC95% + tests non paramétriques.
4. Passage à l’acquisition de bruit réel pour validation hardware.

---

## 7) Conclusion V2

La V2 répond à la demande opérationnelle :
- copie de sécurité du module,
- génération de tous les résultats dans le dossier versionné,
- augmentation des tests,
- enrichissement forensic,
- rapport expert consolidé.

Le prototype reste **validé logiciellement**, et la validation hardware réelle reste la prochaine étape structurante.

---

**Date** : 2026-02-28  
**Version** : V2 consolidée  
**Dossier** : `src/projetx_NQubit NX/NQubit_NX/version_2/`
