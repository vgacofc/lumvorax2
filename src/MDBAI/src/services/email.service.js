/**
 * Email Service
 * Service d'envoi d'emails pour MDBAI
 */

import nodemailer from 'nodemailer';
import logger from '../utils/logger.js';

class EmailService {
  constructor() {
    this.transporter = null;
    this.initialized = false;
    this.from = process.env.EMAIL_FROM || 'noreply@mdbai.com';
  }

  /**
   * Initialise le service email
   */
  async initialize() {
    try {
      // Configuration du transporteur email
      const config = {
        host: process.env.SMTP_HOST || 'smtp.gmail.com',
        port: parseInt(process.env.SMTP_PORT || '587'),
        secure: process.env.SMTP_SECURE === 'true',
        auth: {
          user: process.env.SMTP_USER,
          pass: process.env.SMTP_PASS
        }
      };

      // Si pas de configuration SMTP, utiliser un transporteur de test
      if (!process.env.SMTP_USER || !process.env.SMTP_PASS) {
        logger.warn('Configuration SMTP manquante, utilisation du mode test');
        const testAccount = await nodemailer.createTestAccount();
        config.host = 'smtp.ethereal.email';
        config.port = 587;
        config.secure = false;
        config.auth = {
          user: testAccount.user,
          pass: testAccount.pass
        };
      }

      this.transporter = nodemailer.createTransport(config);
      
      // Vérifier la connexion
      await this.transporter.verify();
      this.initialized = true;
      logger.info('Service email initialisé avec succès');
    } catch (error) {
      logger.error('Erreur lors de l\'initialisation du service email:', error);
      // Ne pas bloquer l'application si l'email n'est pas configuré
      this.initialized = false;
    }
  }

  /**
   * Envoie un email
   */
  async sendEmail({ to, subject, text, html }) {
    if (!this.initialized) {
      logger.warn('Service email non initialisé, email non envoyé');
      return { success: false, error: 'Service email non initialisé' };
    }

    try {
      const mailOptions = {
        from: this.from,
        to,
        subject,
        text,
        html: html || text
      };

      const info = await this.transporter.sendMail(mailOptions);
      
      logger.info(`Email envoyé: ${info.messageId}`);
      
      // Si mode test, afficher l'URL de prévisualisation
      if (process.env.NODE_ENV !== 'production') {
        const previewUrl = nodemailer.getTestMessageUrl(info);
        if (previewUrl) {
          logger.info(`Prévisualisation: ${previewUrl}`);
        }
      }

      return { success: true, messageId: info.messageId };
    } catch (error) {
      logger.error('Erreur lors de l\'envoi de l\'email:', error);
      return { success: false, error: error.message };
    }
  }

  /**
   * Envoie un email de bienvenue
   */
  async sendWelcomeEmail(user) {
    const subject = 'Bienvenue sur MDBAI';
    const text = `
Bonjour ${user.username},

Bienvenue sur MDBAI (Master Debug AI) !

Votre compte a été créé avec succès. Vous pouvez maintenant :
- Analyser vos repositories GitHub
- Recevoir des rapports détaillés
- Suivre vos analyses via Telegram

Crédits disponibles : ${user.credits}

Merci de nous faire confiance !

L'équipe MDBAI
    `.trim();

    const html = `
<!DOCTYPE html>
<html>
<head>
  <style>
    body { font-family: Arial, sans-serif; line-height: 1.6; color: #333; }
    .container { max-width: 600px; margin: 0 auto; padding: 20px; }
    .header { background: #4CAF50; color: white; padding: 20px; text-align: center; }
    .content { padding: 20px; background: #f9f9f9; }
    .footer { padding: 20px; text-align: center; font-size: 12px; color: #666; }
    .button { display: inline-block; padding: 10px 20px; background: #4CAF50; color: white; text-decoration: none; border-radius: 5px; }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>Bienvenue sur MDBAI</h1>
    </div>
    <div class="content">
      <p>Bonjour <strong>${user.username}</strong>,</p>
      <p>Votre compte a été créé avec succès !</p>
      <p>Vous pouvez maintenant :</p>
      <ul>
        <li>Analyser vos repositories GitHub</li>
        <li>Recevoir des rapports détaillés</li>
        <li>Suivre vos analyses via Telegram</li>
      </ul>
      <p><strong>Crédits disponibles :</strong> ${user.credits}</p>
      <p>Merci de nous faire confiance !</p>
    </div>
    <div class="footer">
      <p>L'équipe MDBAI</p>
    </div>
  </div>
</body>
</html>
    `.trim();

    return this.sendEmail({
      to: user.email,
      subject,
      text,
      html
    });
  }

  /**
   * Envoie un email de notification d'analyse terminée
   */
  async sendAnalysisCompleteEmail(user, analysis) {
    const subject = `Analyse terminée : ${analysis.repository}`;
    const text = `
Bonjour ${user.username},

Votre analyse du repository ${analysis.repository} est terminée !

Résumé :
- Fichiers analysés : ${analysis.filesAnalyzed || 0}
- Problèmes détectés : ${analysis.issuesFound || 0}
- Durée : ${analysis.duration || 'N/A'}

Consultez le rapport complet sur votre tableau de bord.

L'équipe MDBAI
    `.trim();

    return this.sendEmail({
      to: user.email,
      subject,
      text
    });
  }

  /**
   * Envoie un email de notification de crédits faibles
   */
  async sendLowCreditsEmail(user) {
    const subject = 'Crédits faibles - MDBAI';
    const text = `
Bonjour ${user.username},

Vos crédits MDBAI sont faibles : ${user.credits} crédits restants.

Pour continuer à utiliser nos services, pensez à recharger votre compte.

L'équipe MDBAI
    `.trim();

    return this.sendEmail({
      to: user.email,
      subject,
      text
    });
  }

  /**
   * Envoie un email de réinitialisation de mot de passe
   */
  async sendPasswordResetEmail(user, resetToken) {
    const resetUrl = `${process.env.APP_URL}/reset-password?token=${resetToken}`;
    const subject = 'Réinitialisation de mot de passe - MDBAI';
    const text = `
Bonjour ${user.username},

Vous avez demandé la réinitialisation de votre mot de passe.

Cliquez sur le lien suivant pour réinitialiser votre mot de passe :
${resetUrl}

Ce lien expire dans 1 heure.

Si vous n'avez pas demandé cette réinitialisation, ignorez cet email.

L'équipe MDBAI
    `.trim();

    return this.sendEmail({
      to: user.email,
      subject,
      text
    });
  }

  /**
   * Envoie un email de notification d'erreur
   */
  async sendErrorNotificationEmail(adminEmail, error) {
    const subject = `[MDBAI] Erreur système`;
    const text = `
Une erreur système s'est produite :

Message : ${error.message}
Stack : ${error.stack}

Timestamp : ${new Date().toISOString()}
    `.trim();

    return this.sendEmail({
      to: adminEmail,
      subject,
      text
    });
  }
}

// Export de l'instance singleton
const emailService = new EmailService();
export default emailService;

// Export des fonctions pour compatibilité
export async function sendVerificationCodeEmail(email, code) {
  await emailService.initialize();
  const subject = '[MDBAI] Code de vérification';
  const text = `
Votre code de vérification MDBAI est : ${code}

Ce code expire dans 10 minutes.

Si vous n'avez pas demandé ce code, ignorez cet email.

L'équipe MDBAI
  `.trim();
  
  return emailService.sendEmail({
    to: email,
    subject,
    text
  });
}

export async function sendWelcomeEmail(user) {
  await emailService.initialize();
  return emailService.sendWelcomeEmail(user);
}

export async function sendAnalysisCompleteEmail(user, analysis) {
  await emailService.initialize();
  return emailService.sendAnalysisCompleteEmail(user, analysis);
}

export async function sendErrorNotificationEmail(adminEmail, error) {
  await emailService.initialize();
  return emailService.sendErrorNotificationEmail(adminEmail, error);
}

export async function sendVerificationEmail(email, token) {
  await emailService.initialize();
  const verifyUrl = `${process.env.APP_URL || process.env.BASE_URL || 'http://localhost:3001'}/verify-email?token=${token}`;
  const subject = '[MDBAI] Vérification de votre email';
  const text = `
Bonjour,

Merci de vous être inscrit sur MDBAI !

Cliquez sur le lien suivant pour vérifier votre email :
${verifyUrl}

Ce lien expire dans 24 heures.

Si vous n'avez pas créé de compte, ignorez cet email.

L'équipe MDBAI
  `.trim();
  
  const html = `
<!DOCTYPE html>
<html>
<head>
  <style>
    body { font-family: Arial, sans-serif; line-height: 1.6; color: #333; }
    .container { max-width: 600px; margin: 0 auto; padding: 20px; }
    .header { background: #4CAF50; color: white; padding: 20px; text-align: center; }
    .content { padding: 20px; background: #f9f9f9; }
    .button { display: inline-block; padding: 10px 20px; background: #4CAF50; color: white; text-decoration: none; border-radius: 5px; margin: 20px 0; }
    .footer { padding: 20px; text-align: center; font-size: 12px; color: #666; }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>Vérification de votre email</h1>
    </div>
    <div class="content">
      <p>Bonjour,</p>
      <p>Merci de vous être inscrit sur MDBAI !</p>
      <p>Cliquez sur le bouton ci-dessous pour vérifier votre email :</p>
      <p style="text-align: center;">
        <a href="${verifyUrl}" class="button">Vérifier mon email</a>
      </p>
      <p>Ou copiez ce lien dans votre navigateur :</p>
      <p style="word-break: break-all; color: #666;">${verifyUrl}</p>
      <p><strong>Ce lien expire dans 24 heures.</strong></p>
      <p>Si vous n'avez pas créé de compte, ignorez cet email.</p>
    </div>
    <div class="footer">
      <p>L'équipe MDBAI</p>
    </div>
  </div>
</body>
</html>
  `.trim();
  
  return emailService.sendEmail({
    to: email,
    subject,
    text,
    html
  });
}

export async function sendPasswordResetEmail(user, resetToken) {
  await emailService.initialize();
  return emailService.sendPasswordResetEmail(user, resetToken);
}
