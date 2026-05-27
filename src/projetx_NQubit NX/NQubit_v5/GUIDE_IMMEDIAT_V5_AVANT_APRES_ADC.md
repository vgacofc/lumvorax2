# GUIDE IMMÉDIAT V5 — Quoi faire maintenant + explication ultra claire du AVANT/APRÈS ADC

## 0) Réponse directe à "Donc il faut faire quoi exactement ?"
Pour intégrer V5 **immédiatement** sans rien casser :
1. Geler les artefacts de run (ne plus modifier les fichiers `results/` ciblés).
2. Générer un manifest canonique V5 depuis ces artefacts.
3. Vérifier que le manifest matche 100% des fichiers.
4. Signer le manifest avec une clé privée d’audit.
5. Vérifier la signature avec la clé publique.
6. Bloquer la CI si vérification hash/signature échoue.

En une phrase : **manifest signé = vérité d’audit**, CI = gardien automatique.

---

## 1) AVANT V5 vs APRÈS V5 (explication pédagogique)

| Sujet | AVANT V5 | APRÈS V5 |
|---|---|---|
| Autorité d’audit | Ambiguë (manifest V4 vs sha256 list) | Claire : `manifest_forensic_v5.json` **signé** |
| Intégrité | Hash présents mais divergence possible entre artefacts | Vérification reproductible via `verify_manifest_v5.py` |
| Preuve d’origine | SHA256 seul (intégrité uniquement) | Signature asymétrique possible (origine + intégrité) |
| Gouvernance | Pas de point d’ancrage unique explicite | `source_of_truth` explicite dans le manifest V5 |
| Risque principal | Confusion en audit final si fichiers régénérés | Réduit si pipeline "générer -> vérifier -> signer -> vérifier" est respecté |

### Ce que cela veut dire concrètement
- **Avant** : on pouvait dire "les hash existent", mais pas toujours "cet ensemble est le bon ensemble final validé par l’autorité".
- **Après** : on peut dire "cet ensemble précis est figé, vérifié, et validé par signature".

---

## 2) Pourquoi "ADC réel validé : NON" (ultra clair)

## 2.1 Ce qu’on appelle "ADC réel validé"
Pour dire "ADC réel validé", il faut normalement prouver :
1. un matériel ADC physique identifié (référence, série, configuration),
2. une calibration/documentation métrologique,
3. un protocole de mesure répété,
4. des résultats reproductibles inter-environnements/labs,
5. des artefacts forensic cohérents signés.

## 2.2 Ce qu’on a dans les preuves actuelles
- Une capture de bruit via un **proxy software/hardware jitter**.
- Des logs forensic robustes.
- Des vérifications de pipeline logiciel.

## 2.3 Pourquoi la conclusion est NON (et pas peut-être)
- Parce que la preuve actuelle décrit surtout une chaîne logicielle instrumentée,
- mais **pas** une chaîne ADC matérielle certifiée de bout en bout.

👉 Donc :
- **Validé côté protocole forensic logiciel : OUI**.
- **Validé côté acquisition ADC physique réelle : NON à ce stade**.

---

## 3) Plan d’intégration immédiate V5 (copier-coller)

## 3.1 Générer le manifest canonique
```bash
python 'src/projetx_NQubit NX/NQubit_v5/tools/build_manifest_v5.py' \
  --input-dir 'src/projetx_NQubit NX/NQubit_v4/results' \
  --output 'src/projetx_NQubit NX/NQubit_v5/results/manifest_forensic_v5.json'
```

## 3.2 Vérifier les hash du manifest
```bash
python 'src/projetx_NQubit NX/NQubit_v5/tools/verify_manifest_v5.py' \
  --manifest 'src/projetx_NQubit NX/NQubit_v5/results/manifest_forensic_v5.json'
```
Attendu : `missing=0` et `mismatches=0`.

## 3.3 Signer le manifest
```bash
bash 'src/projetx_NQubit NX/NQubit_v5/tools/sign_manifest_v5.sh' \
  'src/projetx_NQubit NX/NQubit_v5/results/manifest_forensic_v5.json' \
  '<private_key.pem>' \
  '<public_key.pem>'
```

## 3.4 Mettre en CI (obligatoire)
- Étape 1: exécuter `verify_manifest_v5.py`.
- Étape 2: vérifier la signature.
- Si une étape échoue => pipeline rouge, release bloquée.

---

## 4) Ce que tu peux annoncer maintenant (sans sur-promesse)
Tu peux dire immédiatement :
1. "V5 fixe l’autorité d’audit avec un manifest signé".
2. "La chaîne d’intégrité est vérifiable automatiquement".
3. "Le protocole forensic logiciel est reproductible dans le périmètre testé".

Tu ne dois pas encore dire :
- "ADC physique réel validé scientifiquement" tant que la campagne instrumentée ADC n’est pas exécutée et attestée.

---

## 5) Checklist de validation finale (courte)
- [ ] Manifest V5 régénéré après gel des artefacts.
- [ ] `verify_manifest_v5.py` => 0 mismatch.
- [ ] Signature `.sig` générée.
- [ ] Vérification signature OK avec clé publique.
- [ ] CI bloque les merges si hash/signature invalides.
- [ ] Claims publiques alignées avec le niveau de preuve réel ADC.

## Conclusion opérationnelle
V5 est prêt à être intégré **immédiatement** pour fiabiliser l’audit et l’intégrité.
Le prochain gap scientifique à fermer reste la validation expérimentale ADC physique réelle.
