
#!/bin/bash -ex
echo "=== BENCHMARKS COMPLETS BASELINES vs LUM/VORAX ==="

# Benchmarks baselines
./benchmark_baseline/pg_setup.sh
./benchmark_baseline/redis_benchmark.sh

# Benchmark LUM/VORAX
echo "=== BENCHMARK LUM/VORAX ==="
make clean && make all

# Test performance LUM/VORAX
echo "LUM/VORAX 1M operations:"
/usr/bin/time -v ./bin/lum_vorax --stress-test-million 2>&1 | tee benchmark_lumvorax.log

# Extraction métriques
LUMVORAX_TIME=$(grep "Elapsed" benchmark_lumvorax.log | awk '{print $8}' | cut -d: -f2)
LUMVORAX_THROUGHPUT=$(grep "Creation rate:" benchmark_lumvorax.log | awk '{print $3}')

# Génération rapport comparatif
cat > benchmark_comparison.csv << EOF
system,dataset_size,operation,wall_seconds,throughput_ops_per_s
lumvorax,1000000,create,$LUMVORAX_TIME,$LUMVORAX_THROUGHPUT
EOF

echo "✅ Benchmarks complets terminés"
