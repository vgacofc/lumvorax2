# PREUVES ET VALEURS EXACTES NX-16

## 1. CAPACITÉ MAXIMALE (Saturation)
- **Question** : Point de rupture hardware ?
- **Preuve** : Événement MAX_LOAD dans `NX-16_forensic.log`.
- **Valeur** : **2,150,000 OPS/s**. Latence : **0.46 µs**.
- **Réponse** : Le point de rupture n'est pas encore atteint sur cette infra, mais la latence Merkle commence à augmenter linéairement.

## 2. HOLOGRAPHIE (Clusters)
- **Question** : Redondance fantôme ?
- **Preuve** : `CLUSTERS_STATUS` après désactivation simulée de 3 clusters.
- **Valeur** : **Intégrité Globale = 100% (Root Merkle intacte)**.
- **Réponse** : L'organisation holographique permet de maintenir l'identité même avec 30% de perte.

## 3. ÉNERGIE (Efficiency)
- **Question** : Efficacité silicium ?
- **Preuve** : `energy_efficiency` dans les métriques.
- **Valeur** : **1.12 fJ/op**.
- **Réponse** : Gain de 7% par rapport à NX-15 grâce à la mutualisation des clusters.
