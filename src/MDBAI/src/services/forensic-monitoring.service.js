/**
 * Forensic Monitoring Service
 * Service de monitoring et analyse forensique pour MDBAI
 */

import logger from '../utils/logger.js';

class ForensicMonitoringService {
  constructor() {
    this.metrics = {
      analysisCount: 0,
      errorCount: 0,
      averageAnalysisTime: 0,
      totalAnalysisTime: 0
    };
    this.alerts = [];
    this.initialized = false;
  }

  /**
   * Initialise le service de monitoring
   */
  async initialize() {
    try {
      logger.info('Initialisation du service de monitoring forensique');
      this.initialized = true;
      this.startMetricsCollection();
      return { success: true };
    } catch (error) {
      logger.error('Erreur lors de l\'initialisation du monitoring forensique:', error);
      return { success: false, error: error.message };
    }
  }

  /**
   * Démarre la collecte de métriques
   */
  startMetricsCollection() {
    // Collecte des métriques toutes les 5 minutes
    this.metricsInterval = setInterval(() => {
      this.collectMetrics();
    }, 5 * 60 * 1000);
  }

  /**
   * Collecte les métriques système
   */
  collectMetrics() {
    try {
      const metrics = {
        timestamp: new Date().toISOString(),
        memory: process.memoryUsage(),
        uptime: process.uptime(),
        analysisCount: this.metrics.analysisCount,
        errorCount: this.metrics.errorCount,
        averageAnalysisTime: this.metrics.averageAnalysisTime
      };

      logger.debug('Métriques collectées:', metrics);
      return metrics;
    } catch (error) {
      logger.error('Erreur lors de la collecte des métriques:', error);
      return null;
    }
  }

  /**
   * Enregistre le début d'une analyse
   */
  startAnalysis(analysisId, metadata = {}) {
    const analysis = {
      id: analysisId,
      startTime: Date.now(),
      metadata,
      status: 'running'
    };

    logger.info(`Analyse démarrée: ${analysisId}`, metadata);
    return analysis;
  }

  /**
   * Enregistre la fin d'une analyse
   */
  endAnalysis(analysisId, result = {}) {
    const endTime = Date.now();
    const duration = result.startTime ? endTime - result.startTime : 0;

    this.metrics.analysisCount++;
    this.metrics.totalAnalysisTime += duration;
    this.metrics.averageAnalysisTime = 
      this.metrics.totalAnalysisTime / this.metrics.analysisCount;

    logger.info(`Analyse terminée: ${analysisId}`, {
      duration: `${duration}ms`,
      success: result.success !== false
    });

    return {
      id: analysisId,
      duration,
      endTime,
      success: result.success !== false
    };
  }

  /**
   * Enregistre une erreur
   */
  recordError(error, context = {}) {
    this.metrics.errorCount++;

    const errorRecord = {
      timestamp: new Date().toISOString(),
      message: error.message,
      stack: error.stack,
      context
    };

    logger.error('Erreur enregistrée:', errorRecord);

    // Créer une alerte si trop d'erreurs
    if (this.metrics.errorCount > 10) {
      this.createAlert('high_error_rate', {
        errorCount: this.metrics.errorCount,
        message: 'Taux d\'erreur élevé détecté'
      });
    }

    return errorRecord;
  }

  /**
   * Crée une alerte
   */
  createAlert(type, data) {
    const alert = {
      id: `alert_${Date.now()}`,
      type,
      timestamp: new Date().toISOString(),
      data,
      severity: this.getAlertSeverity(type)
    };

    this.alerts.push(alert);
    logger.warn('Alerte créée:', alert);

    // Garder seulement les 100 dernières alertes
    if (this.alerts.length > 100) {
      this.alerts = this.alerts.slice(-100);
    }

    return alert;
  }

  /**
   * Détermine la sévérité d'une alerte
   */
  getAlertSeverity(type) {
    const severityMap = {
      high_error_rate: 'critical',
      slow_analysis: 'warning',
      memory_high: 'warning',
      disk_full: 'critical'
    };

    return severityMap[type] || 'info';
  }

  /**
   * Récupère les métriques actuelles
   */
  getMetrics() {
    return {
      ...this.metrics,
      uptime: process.uptime(),
      memory: process.memoryUsage(),
      alerts: this.alerts.length
    };
  }

  /**
   * Récupère les alertes
   */
  getAlerts(limit = 50) {
    return this.alerts.slice(-limit);
  }

  /**
   * Réinitialise les métriques
   */
  resetMetrics() {
    this.metrics = {
      analysisCount: 0,
      errorCount: 0,
      averageAnalysisTime: 0,
      totalAnalysisTime: 0
    };
    this.alerts = [];
    logger.info('Métriques réinitialisées');
  }

  /**
   * Analyse les patterns d'erreurs
   */
  analyzeErrorPatterns() {
    // Analyse simplifiée des patterns d'erreurs
    const patterns = {
      totalErrors: this.metrics.errorCount,
      recentAlerts: this.alerts.slice(-10),
      errorRate: this.metrics.errorCount / Math.max(this.metrics.analysisCount, 1)
    };

    logger.debug('Patterns d\'erreurs analysés:', patterns);
    return patterns;
  }

  /**
   * Vérifie la santé du système
   */
  healthCheck() {
    const memory = process.memoryUsage();
    const memoryUsagePercent = (memory.heapUsed / memory.heapTotal) * 100;

    const health = {
      status: 'healthy',
      checks: {
        initialized: this.initialized,
        memoryUsage: memoryUsagePercent < 90,
        errorRate: this.metrics.errorCount / Math.max(this.metrics.analysisCount, 1) < 0.1,
        uptime: process.uptime() > 0
      },
      metrics: this.getMetrics()
    };

    // Déterminer le statut global
    const allChecksPass = Object.values(health.checks).every(check => check === true);
    health.status = allChecksPass ? 'healthy' : 'degraded';

    return health;
  }

  /**
   * Arrête le service
   */
  shutdown() {
    if (this.metricsInterval) {
      clearInterval(this.metricsInterval);
    }
    logger.info('Service de monitoring forensique arrêté');
  }
}

// Instance singleton
const forensicMonitoringService = new ForensicMonitoringService();

export default forensicMonitoringService;
export { ForensicMonitoringService };

// Made with Bob
