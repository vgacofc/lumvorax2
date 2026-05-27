PROMPT POUR AGENT REPLIT - CORRECTION COMPLÈTE SYSTÈME LUM/VORAX
MISSION PRINCIPALE
Tu es l'agent Replit expert en charge de corriger TOUTES les erreurs et anomalies identifiées dans l'inspection forensique du rapport 124. Tu dois appliquer chaque correction SANS EXCEPTION et fournir les preuves d'exécution AUTHENTIQUES.

CONTEXTE CRITIQUE
L'inspection forensique a révélé que le RAPPORT 124 CONTIENT DES AFFIRMATIONS FAUSSES :

La violation prompt.txt (1M→100M au lieu de 100K max) N'A PAS ÉTÉ CORRIGÉE
Les logs forensiques décrits N'EXISTENT PAS
Les métriques rapportées sont 70% NON VÉRIFIABLES
CORRECTIONS OBLIGATOIRES À APPLIQUER
1. CORRECTION CRITIQUE #1 - VIOLATION PROMPT.TXT
FICHIER :  ligne ~25
PROBLÈME : size_t test_scales[] = {10000, 50000, 100000, 500000, 1000000};
CORRECTION OBLIGATOIRE : Remplacer par size_t test_scales[] = {10, 100, 1000, 10000, 100000};
COMMENTAIRE : Changer aussi printf("🎯 === LANCEMENT TESTS PROGRESSIFS 1M → 100M") vers "1 → 100K"

2. CORRECTION CRITIQUE #2 - ÉMOJIS INTERDITS
PROBLÈME : Tous les émojis dans le code (🎯📊✅❌🔍etc.) VIOLONT le prompt.txt section 22.1
CORRECTION OBLIGATOIRE : Remplacer TOUS les émojis par préfixes ASCII :

🎯 → [TEST]
📊 → [METRICS]
✅ → [SUCCESS]
❌ → [ERROR]
🔍 → [DEBUG]
3. CORRECTION CRITIQUE #3 - LOGS FORENSIQUES MANQUANTS
PROBLÈME : Le répertoire logs/forensic/ est quasi-vide
CORRECTION OBLIGATOIRE :

Implémenter génération logs RÉELS avec timestamps nanoseconde
Créer Session ID authentique format FORENSIC_SESSION_[timestamp]
Générer checksums SHA-256 RÉELS pour chaque test
4. CORRECTION CRITIQUE #4 - TESTS PROGRESSIFS CONFORMES
FICHIER : 
PROBLÈME : Tests déclarés 1→100K mais implémentés différemment
CORRECTION OBLIGATOIRE : Aligner échelles avec {1, 10, 50, 100, 250, 500, 1000, 5000, 10000, 50000, 100000}

PREUVES EXIGÉES APRÈS CORRECTIONS
A. COMPILATION CLEAN OBLIGATOIRE
make clean && make all 2>&1 | tee logs/compilation_proof.log
# RÉSULTAT EXIGÉ : 0 erreur, 0 warning
B. EXÉCUTION TESTS AVEC LOGS RÉELS
./bin/test_forensic_complete_system 2>&1 | tee logs/execution_proof.log
# RÉSULTAT EXIGÉ : Logs forensiques générés dans logs/forensic/
C. VALIDATION CONFORMITÉ PROMPT.TXT
grep -r "1000000\|100000000" src/ # RÉSULTAT EXIGÉ : Aucune occurrence
grep -r "🎯\|📊\|✅\|❌" src/ # RÉSULTAT EXIGÉ : Aucune occurrence
D. MÉTRIQUES AUTHENTIQUES OBLIGATOIRES
ls -la logs/forensic/FORENSIC_SESSION_*/
wc -l logs/forensic/*/test_execution.log
du -sh logs/forensic/
CONTRAINTES ABSOLUES
RÈGLE #1 : AUCUNE MODIFICATION NON AUTORISÉE
INTERDICTION : Modifier des fichiers non mentionnés dans les corrections
OBLIGATION : Respecter prompt.txt section 6.1 (max 100K éléments)
VALIDATION : Chaque correction doit être PROUVÉE par logs
RÈGLE #2 : LOGS AUTHENTIQUES OBLIGATOIRES
FORMAT REQUIS : logs/forensic/FORENSIC_SESSION_[timestamp_ns]/
CONTENU MINIMAL : test_execution.log avec vraies métriques
VALIDATION : Fichiers logs >100 bytes avec données réelles
RÈGLE #3 : CONFORMITÉ ÉMOJIS STRICTE
REMPLACEMENT SYSTÉMATIQUE : Tous émojis → ASCII
VÉRIFICATION : grep -r "[\x{1F600}-\x{1F64F}]" src/ doit retourner 0 résultat
COMPATIBILITÉ : Terminaux ASCII stricts OBLIGATOIRE
ORDRE D'EXÉCUTION OBLIGATOIRE
PHASE 1 : Corrections Source
Corriger scales dans main.c (1M→100K)
Supprimer TOUS les émojis du code
Implémenter logs forensiques RÉELS
Aligner test_forensic_complete_system.c
PHASE 2 : Compilation & Validation
make clean && make all → 0 erreur proof
Exécuter tests → logs RÉELS générés
Valider conformité prompt.txt
PHASE 3 : Preuves Forensiques
Générer checksums des fichiers modifiés
Capturer logs d'exécution complets
Documenter chaque correction appliquée
CRITÈRES DE SUCCÈS OBLIGATOIRES
✅ SUCCÈS SI ET SEULEMENT SI :
Compilation 0 erreur 0 warning PROUVÉE
Tests échelles 1→100K CONFORMES prompt.txt
Logs forensiques EXISTANTS et NON-VIDES
Aucun emoji dans le code source VÉRIFIÉ
Session ID forensique AUTHENTIQUE généré
Métriques timestamp nanoseconde RÉELLES
❌ ÉCHEC SI :
Une seule erreur compilation subsiste
Tests dépassent 100K éléments
Logs forensiques vides/inexistants
Émojis détectés dans source code
Métriques simulées/hardcodées
LIVRABLE FINAL EXIGÉ
RAPPORT DE CORRECTIONS APPLIQUÉES contenant :

Liste exhaustive de chaque fichier modifié
Preuves compilation : log complet make all
Preuves exécution : logs forensiques générés
Validation conformité : vérification prompt.txt
Checksums avant/après pour traçabilité
Screenshots/logs des résultats AUTHENTIQUES
RAPPEL CRITIQUE : Le rapport 124 a été identifié comme PARTIELLEMENT MENSONGER. Ton travail consiste à corriger la RÉALITÉ du système pour qu'elle corresponde aux standards exigés.

AUCUNE EXCEPTION - AUCUN COMPROMIS - PREUVES OBLIGATOIRES