
#!/bin/bash

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULTS_DIR="logs/benchmark_logs_impact_${TIMESTAMP}"
mkdir -p "$RESULTS_DIR"

echo "=== BENCHMARK IMPACT DES LOGS - $TIMESTAMP ===" | tee "${RESULTS_DIR}/summary.log"

# Phase 1: Compilation avec logs activés
echo "001. Compilation avec logs activés..." | tee -a "${RESULTS_DIR}/summary.log"
make clean 2>&1 | tee "${RESULTS_DIR}/compile_with_logs.log"
make all 2>&1 | tee -a "${RESULTS_DIR}/compile_with_logs.log"

# Phase 2: Benchmarks avec logs
echo "002. Exécution avec logs activés..." | tee -a "${RESULTS_DIR}/summary.log"
echo "Début benchmarks avec logs: $(date)" > "${RESULTS_DIR}/with_logs_metrics.txt"

/usr/bin/time -v ./bin/lum_vorax 2>&1 | tee "${RESULTS_DIR}/execution_with_logs.log"
/usr/bin/time -v ./src/tests/test_pareto_optimization 2>&1 | tee "${RESULTS_DIR}/pareto_with_logs.log"
if [ -f "./test_pareto_inverse" ]; then
    /usr/bin/time -v ./test_pareto_inverse 2>&1 | tee "${RESULTS_DIR}/pareto_inverse_with_logs.log"
fi

echo "Fin benchmarks avec logs: $(date)" >> "${RESULTS_DIR}/with_logs_metrics.txt"

# Phase 3: Modification pour désactiver les logs
echo "003. Modification code pour désactiver logs..." | tee -a "${RESULTS_DIR}/summary.log"
cp src/logger/lum_logger.h src/logger/lum_logger.h.backup

# Création d'une version sans logs
cat > src/logger/lum_logger_no_logs.h << 'EOF'
#ifndef LUM_LOGGER_H
#define LUM_LOGGER_H

typedef enum {
    LUM_LOG_DEBUG = 0,
    LUM_LOG_INFO = 1,
    LUM_LOG_WARNING = 2,
    LUM_LOG_ERROR = 3
} lum_log_level_e;

// Macros vides pour désactiver tous les logs
#define lum_log_init(filename) ((void)0)
#define lum_log(level, format, ...) ((void)0)
#define lum_log_cleanup() ((void)0)

// Fonctions vides pour compatibilité
static inline void lum_log_init_impl(const char* filename) { (void)filename; }
static inline void lum_log_impl(lum_log_level_e level, const char* format, ...) { 
    (void)level; (void)format; 
}
static inline void lum_log_cleanup_impl(void) { }

#endif // LUM_LOGGER_H
EOF

# Remplacer temporairement le header
mv src/logger/lum_logger.h src/logger/lum_logger.h.original
mv src/logger/lum_logger_no_logs.h src/logger/lum_logger.h

# Phase 4: Recompilation sans logs
echo "004. Recompilation sans logs..." | tee -a "${RESULTS_DIR}/summary.log"
make clean 2>&1 | tee "${RESULTS_DIR}/compile_without_logs.log"
make all 2>&1 | tee -a "${RESULTS_DIR}/compile_without_logs.log"

# Phase 5: Benchmarks sans logs
echo "005. Exécution sans logs..." | tee -a "${RESULTS_DIR}/summary.log"
echo "Début benchmarks sans logs: $(date)" > "${RESULTS_DIR}/without_logs_metrics.txt"

/usr/bin/time -v ./bin/lum_vorax 2>&1 | tee "${RESULTS_DIR}/execution_without_logs.log"
/usr/bin/time -v ./src/tests/test_pareto_optimization 2>&1 | tee "${RESULTS_DIR}/pareto_without_logs.log"
if [ -f "./test_pareto_inverse" ]; then
    /usr/bin/time -v ./test_pareto_inverse 2>&1 | tee "${RESULTS_DIR}/pareto_inverse_without_logs.log"
fi

echo "Fin benchmarks sans logs: $(date)" >> "${RESULTS_DIR}/without_logs_metrics.txt"

# Phase 6: Restauration des logs originaux
echo "006. Restauration configuration logs..." | tee -a "${RESULTS_DIR}/summary.log"
mv src/logger/lum_logger.h.original src/logger/lum_logger.h

# Phase 7: Analyse comparative
echo "007. Analyse comparative des résultats..." | tee -a "${RESULTS_DIR}/summary.log"

cat > "${RESULTS_DIR}/analyze_impact.py" << 'EOF'
#!/usr/bin/env python3
import re
import os

def extract_time_metrics(log_file):
    """Extrait les métriques de temps depuis un log /usr/bin/time -v"""
    metrics = {}
    try:
        with open(log_file, 'r') as f:
            content = f.read()
            
        # Recherche des métriques spécifiques
        patterns = {
            'wall_time': r'Elapsed \(wall clock\) time.*: (\d+):(\d+\.\d+)',
            'user_time': r'User time \(seconds\): (\d+\.\d+)',
            'system_time': r'System time \(seconds\): (\d+\.\d+)',
            'cpu_percent': r'Percent of CPU this job got: (\d+)%',
            'max_memory': r'Maximum resident set size \(kbytes\): (\d+)',
            'page_faults': r'Page faults requiring I/O: (\d+)',
            'context_switches': r'Voluntary context switches: (\d+)'
        }
        
        for key, pattern in patterns.items():
            match = re.search(pattern, content)
            if match:
                if key == 'wall_time':
                    minutes = float(match.group(1))
                    seconds = float(match.group(2))
                    metrics[key] = minutes * 60 + seconds
                elif key == 'cpu_percent':
                    metrics[key] = int(match.group(1))
                elif key in ['max_memory', 'page_faults', 'context_switches']:
                    metrics[key] = int(match.group(1))
                else:
                    metrics[key] = float(match.group(1))
    except Exception as e:
        print(f"Erreur parsing {log_file}: {e}")
    
    return metrics

def compare_metrics(with_logs, without_logs):
    """Compare les métriques avec et sans logs"""
    comparison = {}
    
    for key in with_logs:
        if key in without_logs:
            with_val = with_logs[key]
            without_val = without_logs[key]
            
            if without_val > 0:
                improvement = ((with_val - without_val) / without_val) * 100
                comparison[key] = {
                    'with_logs': with_val,
                    'without_logs': without_val,
                    'improvement_percent': improvement
                }
    
    return comparison

# Analyse des fichiers de logs
results_dir = os.path.dirname(os.path.abspath(__file__))

files_to_analyze = [
    ('execution_with_logs.log', 'execution_without_logs.log', 'Programme Principal'),
    ('pareto_with_logs.log', 'pareto_without_logs.log', 'Tests Pareto'),
    ('pareto_inverse_with_logs.log', 'pareto_inverse_without_logs.log', 'Tests Pareto Inversé')
]

print("# ANALYSE COMPARATIVE IMPACT DES LOGS")
print("=" * 50)

for with_file, without_file, description in files_to_analyze:
    with_path = os.path.join(results_dir, with_file)
    without_path = os.path.join(results_dir, without_file)
    
    if os.path.exists(with_path) and os.path.exists(without_path):
        print(f"\n## {description}")
        print("-" * 30)
        
        with_metrics = extract_time_metrics(with_path)
        without_metrics = extract_time_metrics(without_path)
        comparison = compare_metrics(with_metrics, without_metrics)
        
        for metric, data in comparison.items():
            print(f"{metric}:")
            print(f"  Avec logs: {data['with_logs']}")
            print(f"  Sans logs: {data['without_logs']}")
            print(f"  Impact: {data['improvement_percent']:.2f}%")
            print()
EOF

python3 "${RESULTS_DIR}/analyze_impact.py" | tee "${RESULTS_DIR}/comparative_analysis.txt"

# Phase 8: Génération du checksum final
echo "008. Génération checksums finaux..." | tee -a "${RESULTS_DIR}/summary.log"
find "$RESULTS_DIR" -name "*.log" -o -name "*.txt" | xargs sha256sum > "${RESULTS_DIR}/checksums_impact_analysis.txt"

echo "=== BENCHMARK TERMINÉ - $TIMESTAMP ===" | tee -a "${RESULTS_DIR}/summary.log"
echo "Résultats dans: $RESULTS_DIR" | tee -a "${RESULTS_DIR}/summary.log"
echo "Analyse comparative: ${RESULTS_DIR}/comparative_analysis.txt" | tee -a "${RESULTS_DIR}/summary.log"
