/**
 * Service de déduplication pour éviter les analyses en double
 * Utilise Redis pour stocker les hash des repositories déjà analysés
 */

import crypto from 'crypto';
import { getRedisClient } from './redis.service.js';
import logger from '../utils/logger.js';

export class DeduplicationService {
  constructor() {
    this.prefix = 'mdbai:dedup:';
    this.ttl = 86400; // 24 heures
  }

  /**
   * Génère un hash unique pour un repository
   */
  generateHash(owner, repo, ref = 'main') {
    const data = `${owner}/${repo}/${ref}`;
    return crypto.createHash('sha256').update(data).digest('hex');
  }

  /**
   * Vérifie si un repository a déjà été analysé récemment
   */
  async isDuplicate(owner, repo, ref = 'main') {
    try {
      const hash = this.generateHash(owner, repo, ref);
      const key = `${this.prefix}${hash}`;
      
      const redis = await getRedisClient();
      const exists = await redis.exists(key);
      
      return exists === 1;
    } catch (error) {
      logger.error('Erreur vérification déduplication:', error);
      return false; // En cas d'erreur, on autorise l'analyse
    }
  }

  /**
   * Marque un repository comme analysé
   */
  async markAsAnalyzed(owner, repo, ref = 'main', metadata = {}) {
    try {
      const hash = this.generateHash(owner, repo, ref);
      const key = `${this.prefix}${hash}`;
      
      const data = {
        owner,
        repo,
        ref,
        analyzedAt: new Date().toISOString(),
        ...metadata
      };
      
      const redis = await getRedisClient();
      await redis.setex(key, this.ttl, JSON.stringify(data));
      
      logger.info(`Repository marqué comme analysé: ${owner}/${repo}/${ref}`);
      return true;
    } catch (error) {
      logger.error('Erreur marquage déduplication:', error);
      return false;
    }
  }

  /**
   * Récupère les métadonnées d'une analyse précédente
   */
  async getAnalysisMetadata(owner, repo, ref = 'main') {
    try {
      const hash = this.generateHash(owner, repo, ref);
      const key = `${this.prefix}${hash}`;
      
      const redis = await getRedisClient();
      const data = await redis.get(key);
      
      if (!data) return null;
      
      return JSON.parse(data);
    } catch (error) {
      logger.error('Erreur récupération métadonnées:', error);
      return null;
    }
  }

  /**
   * Supprime l'entrée de déduplication (force une nouvelle analyse)
   */
  async clearAnalysis(owner, repo, ref = 'main') {
    try {
      const hash = this.generateHash(owner, repo, ref);
      const key = `${this.prefix}${hash}`;
      
      const redis = await getRedisClient();
      await redis.del(key);
      
      logger.info(`Déduplication effacée: ${owner}/${repo}/${ref}`);
      return true;
    } catch (error) {
      logger.error('Erreur suppression déduplication:', error);
      return false;
    }
  }

  /**
   * Nettoie toutes les entrées de déduplication
   */
  async clearAll() {
    try {
      const redis = await getRedisClient();
      const keys = await redis.keys(`${this.prefix}*`);
      
      if (keys.length > 0) {
        await redis.del(...keys);
        logger.info(`${keys.length} entrées de déduplication supprimées`);
      }
      
      return keys.length;
    } catch (error) {
      logger.error('Erreur nettoyage déduplication:', error);
      return 0;
    }
  }

  /**
   * Obtient les statistiques de déduplication
   */
  async getStats() {
    try {
      const redis = await getRedisClient();
      const keys = await redis.keys(`${this.prefix}*`);
      
      return {
        totalAnalyzed: keys.length,
        prefix: this.prefix,
        ttl: this.ttl
      };
    } catch (error) {
      logger.error('Erreur récupération stats:', error);
      return {
        totalAnalyzed: 0,
        prefix: this.prefix,
        ttl: this.ttl,
        error: error.message
      };
    }
  }
}

// Export singleton
export const deduplicationService = new DeduplicationService();

// Export des fonctions utilitaires
export const isDuplicate = (owner, repo, ref) => 
  deduplicationService.isDuplicate(owner, repo, ref);

export const markAsAnalyzed = (owner, repo, ref, metadata) => 
  deduplicationService.markAsAnalyzed(owner, repo, ref, metadata);

export const clearAnalysis = (owner, repo, ref) => 
  deduplicationService.clearAnalysis(owner, repo, ref);

export const getAnalysisMetadata = (owner, repo, ref) => 
  deduplicationService.getAnalysisMetadata(owner, repo, ref);

// Made with Bob
