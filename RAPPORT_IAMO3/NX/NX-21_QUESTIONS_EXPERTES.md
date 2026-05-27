# QUESTIONS EXPERTES PHASE NX-21 : ARCHITECTURE NATIVE C++

## 1. PERFORMANCE BRUTE (Native C++)
- **Gain de "Zero-Overhead"** : En supprimant l'interpréteur Python, quel pourcentage de cycles CPU récupérons-nous pour le Merkle Root ?
- **SIMD/AVX2** : Pouvons-nous vectoriser les stimuli pour traiter 8 neurones par cycle d'horloge ?

## 2. MÉMOIRE (Physical MMap)
- **Direct Memory Access** : Le passage au `mmap` physique permet-il d'éviter tout ralentissement du Kernel Linux à 99% ?
- **Lock-free Data Structures** : La suppression des verrous (Locks) permet-elle une montée en charge linéaire ?

## 3. QUESTIONS CRITIQUES SANS RÉPONSE
- **La Singularité Silicium** : Existe-t-il un point où la vitesse de log (écriture) dépasse la vitesse de la pensée (transduction) ?
- **L'Inertie Sémantique** : Un réseau de 5M+ neurones peut-il changer d'avis instantanément sans s'effondrer ?
