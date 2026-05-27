# QUESTIONS EXPERTES & PROTOCOLE DE TEST V11.2

## 1. Questions des Experts du Domaine
Un astrophysicien ou un expert en calcul numérique poserait les questions suivantes :
1. **Q.E. 1 (Topologie)** : Le passage à travers la singularité apparente est-il un effet de la coordonnée de Boyer-Lindquist ou une propriété physique de l'espace-temps ?
2. **Q.E. 2 (Énergie)** : Le gain d'énergie dans le test de Penrose respecte-t-il la limite théorique de 20.9% pour un trou noir de Kerr extrémal ?
3. **Q.E. 3 (Stabilité)** : Quelle est l'évolution de la contrainte Hamiltonienne lors du passage au périastre extrême ?
4. **Q.E. 4 (Précision)** : La Quad Precision (128-bit) élimine-t-elle totalement la "viscosité numérique" observée dans les simulateurs standards comme EinsteinPy ?

## 2. Nouveaux Tests à Réaliser
- **Test T1 (Extrémalité)** : Simulation à $a = 0.99M$ pour tester la limite de Penrose.
- **Test T2 (Hamiltonien)** : Tracking haute fréquence de $H = g^{\mu\nu}p_\mu p_\nu + m^2$ lors du "tunneling".
- **Test T3 (Comparaison EinsteinPy)** : Simulation d'une orbite chaotique et calcul de l'écart RMS entre Double et Quad Precision.

## 3. Découvertes Inconnues & Anomalies
- **Anomalie A1** : Persistance du franchissement de l'horizon interne même sans perturbation extérieure.
- **Inconnu L1** : Absence de documentation dans la littérature actuelle sur le comportement de l'intégrateur RK4 en régime 128-bit lors du franchissement de singularité de type anneau.
