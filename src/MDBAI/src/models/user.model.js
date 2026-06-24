/**
 * User Model
 * Modèle de données pour les utilisateurs MDBAI
 */

// Constantes pour les méthodes d'authentification
export const AUTH_METHOD_EMAIL = 'email';
export const AUTH_METHOD_PHONE = 'phone';
export const AUTH_METHOD_GITHUB = 'github';

// Constantes pour les statuts utilisateur
export const USER_STATUS_ACTIVE = 'active';
export const USER_STATUS_INACTIVE = 'inactive';
export const USER_STATUS_SUSPENDED = 'suspended';
export const USER_STATUS_PENDING = 'pending';

export class User {
  constructor(data = {}) {
    this.id = data.id || null;
    this.githubId = data.githubId || null;
    this.telegramId = data.telegramId || null;
    this.username = data.username || null;
    this.email = data.email || null;
    this.avatarUrl = data.avatarUrl || null;
    this.accessToken = data.accessToken || null;
    this.refreshToken = data.refreshToken || null;
    this.role = data.role || 'user'; // user, admin, beta_tester
    this.credits = data.credits || 0;
    this.preferences = data.preferences || {
      notifications: true,
      language: 'fr',
      theme: 'dark'
    };
    this.metadata = data.metadata || {};
    this.createdAt = data.createdAt || new Date();
    this.updatedAt = data.updatedAt || new Date();
    this.lastLoginAt = data.lastLoginAt || null;
    this.isActive = data.isActive !== undefined ? data.isActive : true;
    this.isBetaTester = data.isBetaTester || false;
  }

  /**
   * Valide les données de l'utilisateur
   */
  validate() {
    const errors = [];

    if (!this.githubId && !this.telegramId) {
      errors.push('Au moins un identifiant (GitHub ou Telegram) est requis');
    }

    if (this.email && !this.isValidEmail(this.email)) {
      errors.push('Email invalide');
    }

    if (!['user', 'admin', 'beta_tester'].includes(this.role)) {
      errors.push('Rôle invalide');
    }

    return {
      isValid: errors.length === 0,
      errors
    };
  }

  /**
   * Vérifie si l'email est valide
   */
  isValidEmail(email) {
    const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
    return emailRegex.test(email);
  }

  /**
   * Convertit l'utilisateur en objet simple (pour JSON)
   */
  toJSON() {
    return {
      id: this.id,
      githubId: this.githubId,
      telegramId: this.telegramId,
      username: this.username,
      email: this.email,
      avatarUrl: this.avatarUrl,
      role: this.role,
      credits: this.credits,
      preferences: this.preferences,
      metadata: this.metadata,
      createdAt: this.createdAt,
      updatedAt: this.updatedAt,
      lastLoginAt: this.lastLoginAt,
      isActive: this.isActive,
      isBetaTester: this.isBetaTester
    };
  }

  /**
   * Convertit l'utilisateur en objet public (sans tokens)
   */
  toPublic() {
    return {
      id: this.id,
      username: this.username,
      avatarUrl: this.avatarUrl,
      role: this.role,
      credits: this.credits,
      isBetaTester: this.isBetaTester
    };
  }

  /**
   * Met à jour le dernier login
   */
  updateLastLogin() {
    this.lastLoginAt = new Date();
    this.updatedAt = new Date();
  }

  /**
   * Ajoute des crédits
   */
  addCredits(amount) {
    if (amount > 0) {
      this.credits += amount;
      this.updatedAt = new Date();
    }
  }

  /**
   * Retire des crédits
   */
  removeCredits(amount) {
    if (amount > 0 && this.credits >= amount) {
      this.credits -= amount;
      this.updatedAt = new Date();
      return true;
    }
    return false;
  }

  /**
   * Vérifie si l'utilisateur a assez de crédits
   */
  hasCredits(amount) {
    return this.credits >= amount;
  }

  /**
   * Met à jour les préférences
   */
  updatePreferences(newPreferences) {
    this.preferences = {
      ...this.preferences,
      ...newPreferences
    };
    this.updatedAt = new Date();
  }

  /**
   * Active/désactive l'utilisateur
   */
  setActive(isActive) {
    this.isActive = isActive;
    this.updatedAt = new Date();
  }

  /**
   * Définit le statut beta tester
   */
  setBetaTester(isBetaTester) {
    this.isBetaTester = isBetaTester;
    this.updatedAt = new Date();
  }

  /**
   * Crée un utilisateur depuis les données GitHub
   */
  static fromGitHub(githubData) {
    return new User({
      githubId: githubData.id,
      username: githubData.login,
      email: githubData.email,
      avatarUrl: githubData.avatar_url,
      metadata: {
        githubProfile: githubData.html_url,
        githubName: githubData.name,
        githubBio: githubData.bio
      }
    });
  }

  /**
   * Crée un utilisateur depuis les données Telegram
   */
  static fromTelegram(telegramData) {
    return new User({
      telegramId: telegramData.id,
      username: telegramData.username,
      metadata: {
        telegramFirstName: telegramData.first_name,
        telegramLastName: telegramData.last_name
      }
    });
  }
  /**
   * Vérifie l'email d'un utilisateur
   */
  static async verifyUserEmail(userId, email) {
    // Cette fonction devrait être implémentée avec la logique de vérification
    // Pour l'instant, on retourne true pour permettre le démarrage
    return true;
  }
}

// Export de la fonction verifyUserEmail pour telegram.service.js
export const verifyUserEmail = User.verifyUserEmail;

export default User;

// Made with Bob
