/**
 * MDBAI — Format .lum (LumVorax Forensic File Format)
 * 
 * Format binaire pour stocker les analyses forensiques bit-level
 * Conforme STANDARD_NAMES_MDBAI.md Section 6
 * 
 * Structure du fichier .lum:
 * - Header (64 bytes)
 * - Metadata (variable)
 * - Analysis Data (variable)
 * - Signature (256 bytes)
 */

import crypto from 'crypto';
import zlib from 'zlib';
import { promisify } from 'util';

const gzip = promisify(zlib.gzip);
const gunzip = promisify(zlib.gunzip);

// Magic number pour identifier les fichiers .lum
const LUM_MAGIC = Buffer.from('LUM\x00', 'ascii');
const LUM_VERSION = 1;

/**
 * Classe pour gérer le format .lum
 */
export class LumFormat {
  constructor() {
    this.version = LUM_VERSION;
  }

  /**
   * Crée un fichier .lum à partir d'une analyse forensique
   * @param {object} analysis - Résultat de l'analyse forensique
   * @param {string} privateKey - Clé privée pour signature (optionnel)
   * @returns {Promise<Buffer>} - Contenu du fichier .lum
   */
  async create(analysis, privateKey = null) {
    // 1. Créer le header (64 bytes)
    const header = this._createHeader(analysis);

    // 2. Créer les metadata (JSON compressé)
    const metadata = this._createMetadata(analysis);
    const compressedMetadata = await gzip(JSON.stringify(metadata));

    // 3. Créer les données d'analyse (JSON compressé)
    const analysisData = this._createAnalysisData(analysis);
    const compressedData = await gzip(JSON.stringify(analysisData));

    // 4. Calculer les checksums
    const metadataChecksum = crypto.createHash('sha256').update(compressedMetadata).digest();
    const dataChecksum = crypto.createHash('sha256').update(compressedData).digest();

    // 5. Créer la signature (si clé privée fournie)
    const signature = privateKey 
      ? this._createSignature(header, compressedMetadata, compressedData, privateKey)
      : Buffer.alloc(256, 0);

    // 6. Assembler le fichier
    const file = Buffer.concat([
      header,                    // 64 bytes
      metadataChecksum,          // 32 bytes
      dataChecksum,              // 32 bytes
      this._createSizeHeader(compressedMetadata.length, compressedData.length), // 16 bytes
      compressedMetadata,        // variable
      compressedData,            // variable
      signature                  // 256 bytes
    ]);

    return file;
  }

  /**
   * Parse un fichier .lum
   * @param {Buffer} fileBuffer - Contenu du fichier .lum
   * @param {string} publicKey - Clé publique pour vérification signature (optionnel)
   * @returns {Promise<object>} - Analyse forensique parsée
   */
  async parse(fileBuffer, publicKey = null) {
    let offset = 0;

    // 1. Lire et valider le header
    const header = fileBuffer.slice(offset, offset + 64);
    offset += 64;
    this._validateHeader(header);

    // 2. Lire les checksums
    const metadataChecksum = fileBuffer.slice(offset, offset + 32);
    offset += 32;
    const dataChecksum = fileBuffer.slice(offset, offset + 32);
    offset += 32;

    // 3. Lire les tailles
    const sizeHeader = fileBuffer.slice(offset, offset + 16);
    offset += 16;
    const { metadataSize, dataSize } = this._parseSizeHeader(sizeHeader);

    // 4. Lire les données compressées
    const compressedMetadata = fileBuffer.slice(offset, offset + metadataSize);
    offset += metadataSize;
    const compressedData = fileBuffer.slice(offset, offset + dataSize);
    offset += dataSize;

    // 5. Vérifier les checksums
    const metadataHash = crypto.createHash('sha256').update(compressedMetadata).digest();
    const dataHash = crypto.createHash('sha256').update(compressedData).digest();

    if (!metadataHash.equals(metadataChecksum)) {
      throw new Error('Metadata checksum mismatch - fichier corrompu');
    }
    if (!dataHash.equals(dataChecksum)) {
      throw new Error('Data checksum mismatch - fichier corrompu');
    }

    // 6. Lire et vérifier la signature
    const signature = fileBuffer.slice(offset, offset + 256);
    if (publicKey) {
      const isValid = this._verifySignature(
        header, compressedMetadata, compressedData, signature, publicKey
      );
      if (!isValid) {
        throw new Error('Signature invalide - fichier non authentique');
      }
    }

    // 7. Décompresser et parser les données
    const metadataJson = await gunzip(compressedMetadata);
    const dataJson = await gunzip(compressedData);

    const metadata = JSON.parse(metadataJson.toString('utf8'));
    const analysisData = JSON.parse(dataJson.toString('utf8'));

    return {
      metadata,
      analysis: analysisData,
      verified: publicKey !== null
    };
  }

  /**
   * Crée le header du fichier .lum (64 bytes)
   * @private
   */
  _createHeader(analysis) {
    const header = Buffer.alloc(64);
    let offset = 0;

    // Magic number (4 bytes)
    LUM_MAGIC.copy(header, offset);
    offset += 4;

    // Version (2 bytes)
    header.writeUInt16LE(this.version, offset);
    offset += 2;

    // Timestamp (8 bytes)
    const timestamp = BigInt(Date.now());
    header.writeBigUInt64LE(timestamp, offset);
    offset += 8;

    // Analysis type (4 bytes)
    const typeCode = this._getAnalysisTypeCode(analysis.type || 'commit');
    header.writeUInt32LE(typeCode, offset);
    offset += 4;

    // Flags (4 bytes)
    const flags = this._createFlags(analysis);
    header.writeUInt32LE(flags, offset);
    offset += 4;

    // Reserved (42 bytes) - pour extensions futures
    // Déjà rempli de zéros par Buffer.alloc

    return header;
  }

  /**
   * Crée les metadata
   * @private
   */
  _createMetadata(analysis) {
    return {
      version: this.version,
      timestamp: new Date().toISOString(),
      repoUrl: analysis.repoUrl,
      commitHash: analysis.commitHash,
      author: analysis.author || 'unknown',
      message: analysis.message || '',
      filesChanged: analysis.filesChanged || 0,
      insertions: analysis.insertions || 0,
      deletions: analysis.deletions || 0,
      tool: 'MDBAI',
      toolVersion: '0.1.0'
    };
  }

  /**
   * Crée les données d'analyse
   * @private
   */
  _createAnalysisData(analysis) {
    return {
      bitLevelAnalysis: analysis.bitLevelAnalysis || {},
      anomalies: analysis.anomalies || [],
      entropy: analysis.entropy || 0,
      byteDistribution: analysis.byteDistribution || {},
      suspiciousPatterns: analysis.suspiciousPatterns || [],
      checksums: analysis.checksums || {},
      riskScore: analysis.riskScore || 0,
      recommendations: analysis.recommendations || []
    };
  }

  /**
   * Crée le header de tailles (16 bytes)
   * @private
   */
  _createSizeHeader(metadataSize, dataSize) {
    const sizeHeader = Buffer.alloc(16);
    sizeHeader.writeUInt32LE(metadataSize, 0);
    sizeHeader.writeUInt32LE(dataSize, 4);
    // 8 bytes réservés pour extensions futures
    return sizeHeader;
  }

  /**
   * Parse le header de tailles
   * @private
   */
  _parseSizeHeader(sizeHeader) {
    return {
      metadataSize: sizeHeader.readUInt32LE(0),
      dataSize: sizeHeader.readUInt32LE(4)
    };
  }

  /**
   * Valide le header
   * @private
   */
  _validateHeader(header) {
    // Vérifier magic number
    const magic = header.slice(0, 4);
    if (!magic.equals(LUM_MAGIC)) {
      throw new Error('Format de fichier invalide - magic number incorrect');
    }

    // Vérifier version
    const version = header.readUInt16LE(4);
    if (version > this.version) {
      throw new Error(`Version ${version} non supportée (max: ${this.version})`);
    }
  }

  /**
   * Obtient le code du type d'analyse
   * @private
   */
  _getAnalysisTypeCode(type) {
    const types = {
      'commit': 0x01,
      'file': 0x02,
      'diff': 0x03,
      'full': 0x04
    };
    return types[type] || 0x00;
  }

  /**
   * Crée les flags
   * @private
   */
  _createFlags(analysis) {
    let flags = 0;
    if (analysis.compressed) flags |= 0x01;
    if (analysis.encrypted) flags |= 0x02;
    if (analysis.signed) flags |= 0x04;
    if (analysis.anomaliesDetected) flags |= 0x08;
    return flags;
  }

  /**
   * Crée une signature RSA
   * @private
   */
  _createSignature(header, metadata, data, privateKey) {
    const sign = crypto.createSign('RSA-SHA256');
    sign.update(header);
    sign.update(metadata);
    sign.update(data);
    
    try {
      const signature = sign.sign(privateKey);
      // Padding à 256 bytes si nécessaire
      if (signature.length < 256) {
        return Buffer.concat([signature, Buffer.alloc(256 - signature.length, 0)]);
      }
      return signature.slice(0, 256);
    } catch (error) {
      console.warn('[LUM] Signature échouée:', error.message);
      return Buffer.alloc(256, 0);
    }
  }

  /**
   * Vérifie une signature RSA
   * @private
   */
  _verifySignature(header, metadata, data, signature, publicKey) {
    try {
      const verify = crypto.createVerify('RSA-SHA256');
      verify.update(header);
      verify.update(metadata);
      verify.update(data);
      
      // Retirer le padding
      const actualSignature = signature.slice(0, signature.indexOf(0) || signature.length);
      return verify.verify(publicKey, actualSignature);
    } catch (error) {
      console.warn('[LUM] Vérification signature échouée:', error.message);
      return false;
    }
  }

  /**
   * Obtient les informations du fichier sans le parser complètement
   * @param {Buffer} fileBuffer - Contenu du fichier .lum
   * @returns {object} - Informations du header
   */
  getFileInfo(fileBuffer) {
    if (fileBuffer.length < 64) {
      throw new Error('Fichier trop petit pour être un fichier .lum valide');
    }

    const header = fileBuffer.slice(0, 64);
    this._validateHeader(header);

    const version = header.readUInt16LE(4);
    const timestamp = Number(header.readBigUInt64LE(6));
    const typeCode = header.readUInt32LE(14);
    const flags = header.readUInt32LE(18);

    const typeNames = {
      0x01: 'commit',
      0x02: 'file',
      0x03: 'diff',
      0x04: 'full'
    };

    return {
      version,
      timestamp: new Date(timestamp).toISOString(),
      type: typeNames[typeCode] || 'unknown',
      compressed: (flags & 0x01) !== 0,
      encrypted: (flags & 0x02) !== 0,
      signed: (flags & 0x04) !== 0,
      anomaliesDetected: (flags & 0x08) !== 0,
      fileSize: fileBuffer.length
    };
  }
}

export default LumFormat;

// Made with Bob
