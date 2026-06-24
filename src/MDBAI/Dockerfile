# Dockerfile pour MDBAI - Master Debug AI
FROM node:20-alpine

# Métadonnées
LABEL maintainer="Gabriel Chaves <vgacofficiel@gmail.com>"
LABEL description="Master Debug AI - Plateforme d'analyse forensique automatisée"
LABEL version="0.1.0"

# Variables d'environnement par défaut
ENV NODE_ENV=production \
    PORT=3000 \
    TZ=Europe/Paris

# Installation des dépendances système
RUN apk add --no-cache \
    bash \
    git \
    curl \
    ca-certificates \
    tzdata \
    && rm -rf /var/cache/apk/*

# Création du répertoire de travail
WORKDIR /app

# Copie des fichiers de dépendances
COPY package*.json ./

# Installation des dépendances Node.js
RUN npm ci --only=production && \
    npm cache clean --force

# Copie du code source
COPY . .

# Création des répertoires nécessaires
RUN mkdir -p logs backups reports && \
    chmod -R 755 logs backups reports

# Utilisateur non-root pour la sécurité
RUN addgroup -g 1001 -S nodejs && \
    adduser -S nodejs -u 1001 && \
    chown -R nodejs:nodejs /app

USER nodejs

# Exposition du port
EXPOSE 3000

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=40s --retries=3 \
    CMD node -e "require('http').get('http://localhost:3000/health', (r) => {process.exit(r.statusCode === 200 ? 0 : 1)})"

# Commande de démarrage
CMD ["node", "src/server.js"]