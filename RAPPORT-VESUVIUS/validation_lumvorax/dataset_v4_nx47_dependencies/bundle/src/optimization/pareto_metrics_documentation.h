
/**
 * Documentation des métriques d'optimisation Pareto
 * 
 * Scores utilisés:
 * - Score FUSE: Ratio d'amélioration temps/mémoire (-0.089 = 8.9% amélioration)
 * - Score SPLIT: Facteur d'équilibrage des partitions (3.505 = distribution optimale)
 * - Score CYCLE: Efficacité du modulo adaptatif (43.153 = réduction LUMs)
 * 
 * Objectifs contradictoires:
 * - Minimiser temps d'exécution
 * - Minimiser utilisation mémoire  
 * - Maximiser conservation des LUMs
 * - Minimiser complexité algorithmique
 */

typedef enum {
    PARETO_METRIC_TIME = 0,
    PARETO_METRIC_MEMORY = 1,
    PARETO_METRIC_CONSERVATION = 2,
    PARETO_METRIC_COMPLEXITY = 3
} pareto_metric_type_t;

typedef struct {
    double execution_time_us;    // Microsecondes
    size_t memory_usage_bytes;   // Bytes
    double conservation_ratio;   // 0.0-1.0
    int algorithmic_complexity;  // O(n), O(log n), etc.
} pareto_objectives_t;
