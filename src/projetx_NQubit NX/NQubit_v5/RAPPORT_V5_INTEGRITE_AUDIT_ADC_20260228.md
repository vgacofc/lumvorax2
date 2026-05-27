# RAPPORT V5 — Intégrité d’audit, signature et statut ADC réel

## 1) Réponse claire à la question 7
**Question 7:** Quel artefact fait foi pour l’audit final (manifest ou sha256 list) ?

### Réponse V5 (décision nette)
L’artefact de référence doit être le **manifest signé** (`manifest_forensic_v5.json` + `manifest_forensic_v5.json.sig`).

Pourquoi :
- Le manifest porte l’inventaire complet des artefacts + hash + tailles.
- Un fichier `sha256 list` seul n’a pas de statut d’autorité s’il peut être régénéré sans gouvernance.
- La signature cryptographique du manifest permet d’authentifier l’émetteur (non-répudiation).

Conclusion Q7 :
- **Autorité finale = manifest signé**.
- `sha256 list` = artefact de support opérationnel (debug/recalcul rapide), pas la source de vérité finale.

---

## 2) Réponse claire à la question 8
**Question 8:** Existe-t-il un mécanisme de signature (clé privée, attestations) au-delà du SHA256 ?

### Ce que signifie la question
- SHA256 prouve l’intégrité de contenu (même contenu -> même hash).
- SHA256 seul ne prouve pas *qui* a publié l’artefact.
- Il faut une signature asymétrique (clé privée / clé publique) pour prouver l’origine.

### Ce que V5 apporte
- Script de signature : `tools/sign_manifest_v5.sh`.
- Vérification possible de signature avec `openssl dgst -verify`.

### Limite actuelle (état réel)
- Le mécanisme est intégré côté outillage.
- La validation complète dépend de la présence d’une vraie clé privée d’audit gérée hors dépôt.

Conclusion Q8 :
- **Oui, mécanisme au-delà du SHA256 désormais prévu/intégré en V5**.
- **Non, la chaîne n’est pas “institutionnellement validée” tant que la gestion de clés/attestations n’est pas officiellement opérée.**

---

## 3) ADC réel + reproductibilité stricte du protocole : validé ou pas ?

Question posée :
> Avons-nous validé ou pas acquisition ADC réelle et reproduire exactement ce protocole avec les mêmes sorties forensic ?

### Réponse factuelle
- Dans les artefacts V4 disponibles, la capture de bruit est un **proxy logiciel/hardware jitter** (pas une acquisition ADC matérielle certifiée avec carte ADC identifiée + calibration métrologique formelle).
- Donc la claim "ADC réel validé" ne peut pas être attestée scientifiquement à 100% avec les seules preuves actuelles.

### Ce qui est validé aujourd’hui
- Pipeline de capture et de logs fonctionne.
- Reproductibilité logicielle du protocole forensic est forte sur l’environnement exécuté.

### Ce qui n’est pas validé aujourd’hui
- Validation métrologique d’une **chaîne ADC physique réelle** (matériel, calibration, bruit instrument, répétitions inter-lab).
- Reproduction *exacte* cross-environment au bit près (possible variations d’horloge/virtualisation).

## Verdict ADC
- **Validation ADC réelle: NON (pas démontrée formellement).**
- **Validation protocole forensic logiciel: OUI (dans le périmètre de test fourni).**

---

## 4) Correctifs V5 intégrés (sans casser l’existant)
1. Nouveau manifest canonique V5 générable automatiquement.
2. Vérification hash exhaustive des artefacts déclarés.
3. Signature du manifest possible via clé privée.
4. Rapport décisionnel clarifiant autorité d’audit et statut ADC.

Aucun ancien fichier V4 n’a été écrasé/modifié pour ces correctifs V5.

---

## 5) Conclusion finale simple
- Si on parle **audit final** : c’est le **manifest signé** qui doit faire foi.
- Si on parle **preuve d’origine** : SHA256 seul ne suffit pas, il faut signature/attestation.
- Si on parle **ADC réel validé** : pas encore validé formellement avec les preuves actuelles.
- Si on parle **protocole forensic logiciel** : validé sur le périmètre fourni.

Donc : V5 corrige le cadre d’intégrité et de gouvernance de preuve, mais l’étape scientifique ADC physique reste à valider par campagne instrumentée dédiée.
