
#!/bin/bash
set -euo pipefail

# Script de validation forensique automatisée LUM/VORAX
# Basé sur le rapport d'audit scientifique final

echo "=== VALIDATION FORENSIQUE LUM/VORAX ==="
echo "Date: $(date -u +"%Y-%m-%dT%H:%M:%SZ")"
echo "Environnement: $(uname -a)"
echo "Compilateur: $(clang --version | head -1)"
echo

# 1. Nettoyage et build forensique
echo "1. Build forensique avec sanitizers..."
make clean
CC=clang CFLAGS="-std=c99 -Wall -Wextra -Werror -O2 -g -fno-omit-frame-pointer -fsanitize=address,undefined" make all

# 2. Génération des checksums
echo "2. Génération des empreintes cryptographiques..."
find src -name "*.c" -o -name "*.h" | sort | xargs sha256sum > checksums_forensic.txt

# 3. Validation de la taille des structures
echo "3. Validation ABI des structures..."
./bin/lum_vorax --sizeof-checks 2>&1 | tee sizeof_validation.log

# 4. Tests SHA-256 RFC 6234 complets
echo "4. Validation cryptographique RFC 6234..."
./bin/lum_vorax --crypto-validation 2>&1 | tee crypto_validation.log

# 5. Tests threading POSIX
echo "5. Validation threading POSIX..."
./bin/lum_vorax --threading-tests 2>&1 | tee threading_validation.log

# 6. Tests de conversion binaire
echo "6. Validation conversion binaire..."
./bin/lum_vorax --binary-conversion-tests 2>&1 | tee binary_validation.log

# 7. Tests parser VORAX
echo "7. Validation parser VORAX..."
./bin/lum_vorax --parser-tests 2>&1 | tee parser_validation.log

# 8. Tests de stress mémoire
echo "8. Tests de stress mémoire..."
./bin/lum_vorax --memory-stress-tests 2>&1 | tee memory_validation.log

# 9. Génération du rapport de preuves
echo "9. Génération du rapport de preuves..."
tar czf lumvorax_forensic_proofs.tar.gz \
    checksums_forensic.txt \
    sizeof_validation.log \
    crypto_validation.log \
    threading_validation.log \
    binary_validation.log \
    parser_validation.log \
    memory_validation.log \
    logs/*.log

# 10. Signature cryptographique du paquet de preuves
sha256sum lumvorax_forensic_proofs.tar.gz > lumvorax_forensic_proofs.tar.gz.sha256

echo
echo "=== VALIDATION FORENSIQUE TERMINÉE ==="
echo "Preuves disponibles dans: lumvorax_forensic_proofs.tar.gz"
echo "Empreinte SHA-256: $(cat lumvorax_forensic_proofs.tar.gz.sha256)"
echo
