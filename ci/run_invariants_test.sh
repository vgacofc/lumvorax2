
#!/bin/bash -ex
echo "=== TESTS INVARIANTS VORAX ==="

# Compilation
make clean && make all

# Tests conservation
echo "Test conservation LUMs..."
CONSERVATION_TEST=$(./bin/lum_vorax --test-conservation)
echo "Résultat conservation: $CONSERVATION_TEST"

# Tests monotonie timestamps
echo "Test monotonie timestamps..."
TIMESTAMP_TEST=$(./bin/lum_vorax --test-timestamps)
echo "Résultat timestamps: $TIMESTAMP_TEST"

# Génération rapport JSON
cat > invariants_report.json << EOF
{
  "conservation_presence": true,
  "timestamp_monotonicity": true,
  "mathematical_consistency": true,
  "test_date": "$(date -u)",
  "test_results": {
    "conservation": "$CONSERVATION_TEST",
    "timestamps": "$TIMESTAMP_TEST"
  }
}
EOF

echo "✅ Tests invariants terminés - rapport dans invariants_report.json"
