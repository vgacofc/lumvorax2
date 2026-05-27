/**
 * MDBAI — Service de génération de rapport Markdown
 * Conforme STANDARD_NAMES_MDBAI.md Section 5 (generateReport, formatErrors...)
 * CF-010: RAPPORT_MDBAI_*.md créé
 */

import { writeFileSync, mkdirSync } from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';
import logger from '../utils/logger.js';
import { VULN_SEVERITY_CRITICAL, VULN_SEVERITY_HIGH } from '../models/result.model.js';
import { MdbaiError, ERR_REPORT_GEN } from '../utils/errors.js';

const __dirname = dirname(fileURLToPath(import.meta.url));
const REPORTS_DIR = join(__dirname, '../../reports');

export class ReportService {
  /**
   * Génère un rapport Markdown complet depuis un AnalysisResult
   * @param {AnalysisResult} result
   * @param {object} meta - {jobId, repoUrl, repoOwner, repoName, startedAt, duration_ms}
   * @returns {string} Contenu Markdown
   */
  generateReport(result, meta = {}) {
    try {
      const {
        jobId = 'N/A', repoUrl = '', repoOwner = '', repoName = '',
        startedAt = new Date().toISOString(), duration_ms = 0,
      } = meta;

      const score = result.analysis?.score ?? 0;
      const emoji = score >= 80 ? '🟢' : score >= 60 ? '🟡' : '🔴';
      const ts = new Date().toISOString().slice(0, 19).replace('T', ' ');
      const durationSec = Math.round(duration_ms / 1000);
      const lang = result.repo?.language || 'unknown';

      const sections = [
        this._header(emoji, score, repoOwner, repoName, ts, jobId),
        this._summary(result, durationSec, lang),
        this.formatErrors(result.analysis?.errors || []),
        this.formatMemoryLeaks(result.forensic?.memory_leaks || []),
        this.formatVulnerabilities(result.analysis?.vulnerabilities || []),
        this.formatPerformance(result.forensic?.performance || {}),
        this.formatForensic(result.forensic || {}),
        this._footer(score, repoUrl),
      ];

      const markdown = sections.join('\n\n');
      logger.info('[REPORT] Rapport généré', { jobId, score, size: markdown.length });
      return markdown;
    } catch (e) {
      throw new MdbaiError(ERR_REPORT_GEN, `Génération rapport échouée: ${e.message}`);
    }
  }

  _header(emoji, score, owner, repo, ts, jobId) {
    return [
      `# ${emoji} Rapport MDBAI — Analyse Forensique`,
      ``,
      `| Champ | Valeur |`,
      `|-------|--------|`,
      `| **Dépôt** | \`${owner}/${repo}\` |`,
      `| **Score qualité** | **${score}/100** |`,
      `| **Date** | ${ts} |`,
      `| **Job ID** | \`${jobId}\` |`,
      `| **Version** | MDBAI v0.1.0 |`,
      `| **Moteur forensic** | LumVorax C111 |`,
      ``,
      `> Rapport généré automatiquement par [MDBAI](https://t.me/masterdebugai_bot)`,
      `> Traçabilité bit-level LumVorax activée — prompt.txt Règle #1`,
    ].join('\n');
  }

  _summary(result, durationSec, lang) {
    const errors = result.analysis?.errors || [];
    const vulns  = result.analysis?.vulnerabilities || [];
    const leaks  = result.forensic?.memory_leaks || [];
    const crit   = errors.filter(e => e.severity === 'critical').length;
    const high   = errors.filter(e => e.severity === 'high').length;
    const med    = errors.filter(e => e.severity === 'medium').length;
    const low    = errors.filter(e => e.severity === 'low').length;

    return [
      `## 📊 Résumé Exécutif`,
      ``,
      `| Catégorie | Nombre | Impact |`,
      `|-----------|--------|--------|`,
      `| 🔴 Erreurs critiques | **${crit}** | Bloquant |`,
      `| 🟠 Erreurs hautes | **${high}** | Majeur |`,
      `| 🟡 Erreurs moyennes | **${med}** | Moyen |`,
      `| ⚪ Avertissements | **${low}** | Mineur |`,
      `| 🛡 Vulnérabilités CVE | **${vulns.length}** | Sécurité |`,
      `| 💾 Fuites mémoire | **${leaks.length}** | Stabilité |`,
      ``,
      `**Langage détecté:** \`${lang}\`  `,
      `**Durée d'analyse:** ${durationSec}s  `,
      `**Exit code:** \`${result.execution?.exit_code ?? -1}\``,
    ].join('\n');
  }

  /**
   * Formate la section erreurs
   */
  formatErrors(errors = []) {
    if (!errors.length) {
      return `## ✅ Erreurs\n\nAucune erreur détectée.`;
    }

    const lines = [`## 🔴 Erreurs Détectées (${errors.length})\n`];
    const bySeverity = ['critical', 'high', 'medium', 'low'];

    for (const sev of bySeverity) {
      const sevErrors = errors.filter(e => e.severity === sev);
      if (!sevErrors.length) continue;
      const icon = { critical: '🔴', high: '🟠', medium: '🟡', low: '⚪' }[sev];
      lines.push(`### ${icon} ${sev.charAt(0).toUpperCase() + sev.slice(1)} (${sevErrors.length})\n`);
      for (const err of sevErrors.slice(0, 20)) {
        lines.push(`- **[${err.type || 'error'}]** \`${err.message}\``);
      }
      lines.push('');
    }
    return lines.join('\n');
  }

  /**
   * Formate la section fuites mémoire
   */
  formatMemoryLeaks(leaks = []) {
    if (!leaks.length) {
      return `## ✅ Fuites Mémoire\n\nAucune fuite mémoire détectée.`;
    }

    const totalBytes = leaks.reduce((sum, l) => sum + (l.size_bytes || 0), 0);
    const lines = [
      `## 💾 Fuites Mémoire (${leaks.length} — total: ${(totalBytes / 1024).toFixed(1)} KB)\n`,
    ];

    for (const leak of leaks.slice(0, 15)) {
      lines.push(
        `- **Tool:** \`${leak.tool}\` | **Size:** ${leak.size_bytes} bytes\n  ` +
        `\`${leak.raw?.slice(0, 150) || 'N/A'}\``
      );
    }

    if (leaks.length > 15) lines.push(`\n> ... et ${leaks.length - 15} autres fuites`);
    return lines.join('\n');
  }

  /**
   * Formate la section vulnérabilités
   */
  formatVulnerabilities(vulns = []) {
    if (!vulns.length) {
      return `## ✅ Vulnérabilités\n\nAucune vulnérabilité CVE détectée.`;
    }

    const critCount = vulns.filter(v => v.severity === VULN_SEVERITY_CRITICAL).length;
    const highCount = vulns.filter(v => v.severity === VULN_SEVERITY_HIGH).length;
    const lines = [
      `## 🛡 Vulnérabilités CVE (${vulns.length} — 🔴 critical: ${critCount}, 🟠 high: ${highCount})\n`,
      `| Package | Version | Sévérité | CVE |`,
      `|---------|---------|----------|-----|`,
    ];

    for (const v of vulns.slice(0, 20)) {
      const icon = {
        critical: '🔴', high: '🟠', medium: '🟡', low: '⚪'
      }[v.severity] || '⚪';
      const cve = v.cve ? `[${v.cve}](${v.cve.startsWith('http') ? v.cve : '#'})` : 'N/A';
      lines.push(`| \`${v.package}\` | \`${v.version}\` | ${icon} ${v.severity} | ${cve} |`);
    }

    return lines.join('\n');
  }

  /**
   * Formate la section performance
   */
  formatPerformance(metrics = {}) {
    return [
      `## ⚡ Performance`,
      ``,
      `| Métrique | Valeur |`,
      `|----------|--------|`,
      `| CPU | ${metrics.cpu_percent || 0}% |`,
      `| Mémoire | ${metrics.memory_mb || 0} MB |`,
      `| I/O Lecture | ${metrics.io_read_mb || 0} MB |`,
      `| I/O Écriture | ${metrics.io_write_mb || 0} MB |`,
      `| Durée | ${metrics.duration_ms || 0} ms |`,
      `| Snapshots LUM | ${metrics.lum_snapshots_count || 0} |`,
    ].join('\n');
  }

  /**
   * Formate la section forensic LumVorax
   */
  formatForensic(forensic = {}) {
    const snaps = forensic.lum_snapshots || [];
    const lines = [
      `## 🔬 Traçabilité Forensique LumVorax`,
      ``,
      `**Moteur:** LumVorax C111 | **Format:** .lum binaire | **Magic:** \`0x4D444241\``,
      `**Granularité:** PAGE (4 KiB par LUM) | **Rotation:** 20 MB`,
      ``,
    ];

    if (snaps.length) {
      lines.push(`### Snapshots Mémoire .lum\n`);
      lines.push(`| Fichier | Taille | Type | Magic |`);
      lines.push(`|---------|--------|------|-------|`);
      for (const s of snaps) {
        lines.push(`| \`${s.file?.split('/').pop()}\` | ${(s.size_bytes / 1024).toFixed(1)} KB | ${s.type} | \`${s.magic}\` |`);
      }
    } else {
      lines.push(`> Snapshots LUM non disponibles (libmdbai_forensic.so non compilée)`);
      lines.push(`> Compiler avec: \`cd src/MDBAI/forensic && make\``);
    }

    lines.push(`\n*Overhead forensic: ~2-8% (acceptable per prompt.txt Règle #1)*`);
    return lines.join('\n');
  }

  _footer(score, repoUrl) {
    const recommendation = score >= 80
      ? '✅ Code de qualité — prêt pour production avec corrections mineures'
      : score >= 60
      ? '⚠️ Corrections requises avant déploiement'
      : '🚫 Bloquant — corrections critiques obligatoires';

    return [
      `## 📝 Recommandation`,
      ``,
      `**${recommendation}**`,
      ``,
      `---`,
      ``,
      `*Rapport généré par [MDBAI](https://t.me/masterdebugai_bot) — Master Debug AI*  `,
      `*LumVorax Forensic Engine — 0€ budget — Tier gratuit uniquement*  `,
      `*[Dépôt analysé](${repoUrl})*`,
    ].join('\n');
  }

  /**
   * Sauvegarde le rapport sur disque local
   * @param {string} content
   * @param {string} jobId
   * @returns {string} Chemin du fichier
   */
  saveReport(content, jobId) {
    mkdirSync(REPORTS_DIR, { recursive: true });
    const ts = new Date().toISOString().replace(/[:.]/g, '').slice(0, 15);
    const filename = `RAPPORT_MDBAI_${ts}_${jobId.slice(0, 8)}.md`;
    const filepath = join(REPORTS_DIR, filename);
    writeFileSync(filepath, content, 'utf8');
    logger.info(`[REPORT] Rapport sauvegardé: ${filepath}`);
    return filepath;
  }
}

export default ReportService;
