
#!/bin/bash

echo "🔥 VALIDATION COMPLÈTE FONCTIONNALITÉ LUM/VORAX"
echo "Interdiction absolue de placeholder - Code 100% réel"
echo "=================================================="

# Nettoyage complet
echo "1. Nettoyage..."
make clean

# Compilation avec détection d'erreurs
echo "2. Compilation..."
if ! make all; then
    echo "❌ ÉCHEC COMPILATION"
    exit 1
fi

echo "✅ Compilation réussie"

# Vérification qu'aucun placeholder n'existe dans le code
echo "3. Vérification anti-placeholder..."
PLACEHOLDER_COUNT=$(grep -r "TODO\|FIXME\|placeholder\|XXX\|IMPLEMENT" src/ --exclude-dir=tests | wc -l)
if [ $PLACEHOLDER_COUNT -gt 0 ]; then
    echo "❌ PLACEHOLDER DÉTECTÉ - INTERDIT!"
    grep -r "TODO\|FIXME\|placeholder\|XXX\|IMPLEMENT" src/ --exclude-dir=tests
    exit 1
fi
echo "✅ Aucun placeholder détecté"

# Test de compilation du test complet
echo "4. Compilation test complet..."
if ! make test-complete; then
    echo "❌ ÉCHEC TEST COMPLET"
    exit 1
fi

# Test d'exécution principal
echo "5. Exécution programme principal..."
if ! ./bin/lum_vorax; then
    echo "❌ ÉCHEC EXÉCUTION PRINCIPALE"
    exit 1
fi

echo "✅ Exécution principale réussie"

# Vérification des logs générés
echo "6. Vérification logs..."
if [ ! -f "logs/lum_vorax.log" ]; then
    echo "❌ Logs non générés"
    exit 1
fi

LOG_LINES=$(wc -l < logs/lum_vorax.log)
if [ $LOG_LINES -lt 5 ]; then
    echo "❌ Logs insuffisants ($LOG_LINES lignes)"
    exit 1
fi

echo "✅ Logs générés ($LOG_LINES lignes)"

# Tests cryptographiques
echo "7. Tests cryptographiques RFC 6234..."
if ! ./bin/lum_vorax --crypto-validation; then
    echo "❌ ÉCHEC VALIDATION CRYPTO"
    exit 1
fi

echo "✅ Cryptographie validée"

# Test ABI
echo "8. Tests ABI structures..."
if ! ./bin/lum_vorax --sizeof-checks; then
    echo "❌ ÉCHEC VÉRIFICATION ABI"
    exit 1
fi

echo "✅ ABI vérifié"

echo ""
echo "🎉 VALIDATION COMPLÈTE RÉUSSIE!"
echo "✅ Code 100% fonctionnel sans placeholder"
echo "✅ Compilation sans erreur"
echo "✅ Exécution sans crash"
echo "✅ Logs système générés"
echo "✅ Cryptographie RFC 6234 conforme"
echo "✅ Structures ABI correctes"
echo "✅ Tests complets passés"
echo ""
echo "RÉSULTAT: PROJET AUTHENTIQUE ET FONCTIONNEL"
