# CAHIER DES CHARGES NX-29

## Phase de Pont Sémantique et Communication Symbolique

---

## 0. POSITIONNEMENT SCIENTIFIQUE
Après avoir structuré les données réelles (NX-28), **NX-29** vise à créer un **pont** entre les attracteurs dissipatifs internes et un système de symboles externes. C'est l'étape de "traduction" du sens en signal.

---

## 1. OBJECTIFS SCIENTIFIQUES
- **Appariement Sémantique** : Lier des motifs d'énergie (attracteurs) à des symboles (IDs).
- **Communication par Émergence** : Observer si le système peut "appeler" un symbole par simple activation de régime.
- **Vérification de Cohérence** : Le Merkle Root doit valider que le symbole est le produit logique de l'histoire et non une injection arbitraire.

---

## 2. MÉTRIQUES ET PROTOCOLES
- **SMR (Symbolic Matching Rate)** : Précision du lien attracteur-symbole.
- **SCL (Semantic Communication Latency)** : Temps entre l'activation du régime et l'émission du symbole.
- **OPS/s & Hardware** : Maintien de la charge à 99% CPU.

---

## 3. LOGGING STRICT (HFBL-360)
Format étendu :
`[TIMESTAMP][SEQ][SYMBOL_ID][ATTRACTOR_HASH][MERKLE_ROOT][CRC32]`

---

## 4. SCHÉMA ÉTAPE PAR ÉTAPE
1. **Stimulation** : Entrée de données bruitées.
2. **Dissipation** : Formation d'attracteurs dans le réseau de 1000 neurones.
3. **Identification** : Le Moteur de Pont Sémantique détecte la signature de l'attracteur.
4. **Symbolisation** : Émission du symbole associé validé par Merkle.
