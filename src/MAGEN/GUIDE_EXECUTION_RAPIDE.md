# 🚀 Guide d'Exécution Rapide MAGEN

**Démarrage rapide pour tests locaux avec jeux ARC-AGI-3 réels**

---

## ⚡ Exécution en 3 Étapes

### Étape 1: Installation Dépendances (1 fois)

```bash
cd lumvorax2/src/MAGEN

# Installer wheels ARC-AGI-3
bash scripts/install_arc_dependencies.sh
```

**Durée**: ~2 minutes  
**Résultat attendu**: ✅ arc_agi et arcengine installés

---

### Étape 2: Exécuter Tests Locaux

```bash
# Test sur 3 jeux (rapide)
bash scripts/run_magen_local_tests.sh --games 3

# OU test sur 5 jeux avec forensique complet
bash scripts/run_magen_local_tests.sh --games 5 --forensic-full
```

**Durée**: ~5-15 minutes selon nombre de jeux  
**Résultat attendu**: Logs dans `logs/` + résultats JSON

---

### Étape 3: Analyser Résultats

```bash
# Voir résultats
cat logs/results/magen_arc_results_*.json | jq .

# Voir logs forensiques
cat logs/forensic/magen_arc_*.json | jq .

# Voir logs exécution
tail -100 logs/magen_arc_test.log
```

---

## 📊 Commandes Utiles

### Tests Spécifiques

```bash
# Jeux spécifiques
bash scripts/run_magen_local_tests.sh --game-ids ar25 bp35 cd82

# Mode debug
bash scripts/run_magen_local_tests.sh --games 3 --debug

# Tous les jeux disponibles (25)
bash scripts/run_magen_local_tests.sh --games 25 --forensic-full
```

### Vérifications

```bash
# Vérifier dataset
ls -lh data/arc_agi_3/environment_files/

# Vérifier wheels
ls -lh data/arc_agi_3/arc_agi_3_wheels/

# Tester adaptateur
python adapters/arc_agi_adapter.py

# Tester imports
python -c "from arc_agi import EnvironmentWrapper; print('✅ OK')"
```

### Configuration Doppler (Optionnel)

```bash
# Installer Doppler
curl -Ls https://cli.doppler.com/install.sh | sudo sh

# Authentifier
doppler login

# Configurer secrets
bash scripts/configure_doppler.sh

# Exécuter avec secrets
doppler run --project magen --config local -- \
  bash scripts/run_magen_local_tests.sh --games 3
```

---

## 🎯 Métriques à Surveiller

### Performance
- **Actions/niveau**: Comparer vs baseline humain
- **Efficiency ratio**: `baseline_total / magen_total`
- **Score**: `(baseline_total / magen_total)²`
- **Temps**: Durée par jeu et totale

### Mémoire
- **Usage MB**: Doit rester < 100 MB
- **Patterns extraits**: Nombre de patterns mémorisés
- **Liens causaux**: Connexions entre expériences
- **Consolidation**: Temps de consolidation

### Forensique
- **Événements**: Nombre d'événements tracés
- **Signatures**: 100% doivent être validées
- **Anomalies**: Doit être 0
- **Intégrité**: Vérifier avec HMAC-SHA256

---

## 🔍 Dépannage

### Erreur: arc_agi non trouvé

```bash
# Réinstaller wheels
cd lumvorax2/src/MAGEN
bash scripts/install_arc_dependencies.sh
```

### Erreur: Dataset introuvable

```bash
# Télécharger dataset
python scripts/download_arc_dataset.py
```

### Erreur: Venv introuvable

```bash
# Créer venv
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

### Logs vides ou incomplets

```bash
# Vérifier permissions
chmod +x scripts/*.sh

# Vérifier répertoires
mkdir -p logs/forensic logs/results logs/magen

# Relancer avec debug
bash scripts/run_magen_local_tests.sh --games 1 --debug
```

---

## 📈 Résultats Attendus

### Tests Simulés (Déjà Validés ✅)
```
✅ 6/6 niveaux réussis (100%)
⚡ 38 actions totales (6.3/niveau)
💾 2.71 MB mémoire
🔐 6 événements forensiques, 0 anomalie
⏱️  0.17s durée
```

### Tests Réels (À Valider)
```
🎮 3-5 jeux testés
✅ X/Y niveaux complétés
🎯 Z actions totales
📈 Efficiency: W%
🏆 Score moyen: V
⏱️  T secondes
```

---

## ⚠️ IMPORTANT

### Contraintes
- ✅ **TOUT RESTE EN LOCAL** jusqu'à validation
- ❌ **Aucune soumission Kaggle** sans autorisation
- ✅ **Validation manuelle** requise avant production

### Prochaines Étapes
1. ✅ Exécuter tests locaux
2. ✅ Analyser résultats et logs
3. ✅ Optimiser si nécessaire
4. ⏳ **ATTENDRE VALIDATION UTILISATEUR**
5. ⏳ Préparer soumission Kaggle (après validation)

---

## 📞 Support

**Logs à consulter en cas de problème**:
- `logs/magen_arc_test.log` - Logs exécution
- `logs/forensic/*.json` - Logs forensiques
- `logs/results/*.json` - Résultats détaillés

**Commandes de diagnostic**:
```bash
# État système
python -c "import sys; print(f'Python: {sys.version}')"
pip list | grep -E "(arc_agi|arcengine|numpy|scipy)"

# État dataset
find data/arc_agi_3 -type f | wc -l

# État logs
ls -lh logs/forensic/ logs/results/
```

---

**Dernière mise à jour**: 2026-06-12 01:57 UTC  
**Version**: 1.0.0-local  
**Statut**: ✅ Prêt pour exécution