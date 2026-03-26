# FEUILLE DE ROUTE OPÉRATIONNELLE : NX-46 V1

## 1. PHASE DE PRÉPARATION (L0)
- **Action** : Configuration de l'environnement de capture matériel.
- **Détails** : Activation des sondes `/proc/stat` (CPU), `/proc/meminfo` (RAM) et lecture du TSC (Time Stamp Counter) pour les nanosecondes.

## 2. PHASE DE DÉVELOPPEMENT CORE (L1-L2)
- **Action** : Codage du moteur `nx46_v1_engine.cpp`.
- **Détails** :
    - Implémentation du **Slab Allocator** pour les neurones dynamiques.
    - Intégration du **Calculateur de Q.I.** (Ratio complexité/ressource).
    - Boucle de calcul Collatz optimisée AVX2.

## 3. PHASE DE SÉCURITÉ ET LOGGING (L3)
- **Action** : Mise en place du protocole HFBL-360.
- **Détails** :
    - Écriture asynchrone des logs .csv et .json.
    - Calcul de la racine Merkle 360 toutes les 100ms.

## 4. PHASE DE VALIDATION ET RAPPORT
- **Action** : Exécution des tests unitaires et rédaction du rapport.
- **Détails** :
    - Vérification du % d'activation de chaque fonction.
    - Analyse ligne par ligne des fichiers de logs générés.

---
# CAHIER DES CHARGES TECHNIQUE

## Spécifications du Neurone NX-46 V1
- **Type** : Neurone Atome à Entropie Physique.
- **Capacité** : Auto-génération de 1 à 10^9 neurones.
- **Précision** : Nanoseconde (HFBL-360).
- **Certification** : Compatible Aristotle / Lean 4.

## Définition des Termes Pédagogiques
- **Slab Allocator** : Un système qui pré-réserve des blocs de mémoire pour créer des neurones instantanément sans ralentir le processeur.
- **Jitter d'Horloge** : Les minuscules variations de temps entre deux pulsations du processeur, utilisées ici comme source de hasard pur (entropie).
- **Indice de Q.I. Computationnel** : Score calculé en divisant le nombre d'étapes de Collatz résolues par l'énergie CPU consommée. Plus le score est haut, plus le système est "intelligent".
- **Merkle 360** : Une structure de données où chaque résultat est lié au précédent par une clé secrète. Si une seule donnée change, toute la chaîne est brisée.
