
#!/bin/bash -ex
echo "=== VALIDATION CRYPTOGRAPHIQUE RFC 6234 ==="

# Compilation avec crypto
make clean && make all

# Tests vecteurs RFC 6234
echo "Test vecteurs SHA-256 RFC 6234..."
./bin/lum_vorax --crypto-validation &> crypto_validation.log

# Test hash vide selon RFC
echo "Test hash vide (RFC 6234 vecteur 1)..."
echo -n "" | ./bin/lum_vorax --sha256 | xxd -p -l 32 >> crypto_vectors.txt

# Validation complète
echo "✅ Validation crypto terminée - résultats dans crypto_validation.log"
cat crypto_validation.log
