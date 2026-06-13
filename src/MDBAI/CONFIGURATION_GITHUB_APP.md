# Configuration GitHub App MDBAI

## URLs à configurer sur GitHub App Settings

**GitHub App ID**: 3888479  
**Client ID**: Iv23liM06X4pQnng7oFm

### 1. Homepage URL
```
https://headboard-romp-crust.ngrok-free.dev
```

### 2. Callback URL (OAuth)
```
https://headboard-romp-crust.ngrok-free.dev/auth/github/callback
```

### 3. Webhook URL
```
https://headboard-romp-crust.ngrok-free.dev/webhook/github
```

### 4. Webhook Secret
Générer un secret aléatoire et l'ajouter dans `.env`:
```bash
GITHUB_WEBHOOK_SECRET=votre_secret_ici
```

## Permissions requises

### Repository permissions
- **Contents**: Read & write (pour créer branches et commits)
- **Pull requests**: Read & write (pour créer PRs)
- **Issues**: Read & write (pour ajouter labels)
- **Metadata**: Read-only (obligatoire)

### Account permissions
- **Email addresses**: Read-only (pour récupérer email utilisateur)

## OAuth Scopes
- `repo` (accès complet aux dépôts privés/publics)
- `read:user` (lire profil utilisateur)

## Événements webhook à activer
- `push` (détection nouveaux commits)
- `pull_request` (création/modification PR)
- `installation` (installation/désinstallation app)

## Variables d'environnement (.env)

```bash
# GitHub App OAuth
GITHUB_APP_CLIENT_ID=Iv23liM06X4pQnng7oFm
GITHUB_APP_CLIENT_SECRET=25a467c860016f63afedbbbaec6246fc2a85d51b
GITHUB_APP_ID=3888479

# GitHub OAuth Callback URL (ngrok pour webhooks)
GITHUB_CALLBACK_URL=https://headboard-romp-crust.ngrok-free.dev/auth/github/callback

# Base URL (localhost pour développement)
MDBAI_BASE_URL=http://localhost:3001
```

## Vérification configuration

```bash
# 1. Vérifier variables chargées
node -e "import('./src/config.js').then(m => console.log('CLIENT_ID:', m.config.github.clientId, '\nCALLBACK_URL:', m.config.github.callbackUrl))"

# 2. Tester URL OAuth générée
node -e "import('./src/services/github.service.js').then(m => { const g = new m.GitHubService(); console.log(g.getAuthUrl('TEST')); })"

# 3. Vérifier serveur actif
curl http://localhost:3001/health
```

## Flux OAuth complet

1. Utilisateur tape `/github` sur Telegram
2. Bot envoie URL OAuth avec `state=telegram_id`
3. Utilisateur clique → redirigé vers GitHub
4. GitHub demande autorisation
5. Utilisateur accepte → GitHub redirige vers callback
6. Callback échange `code` contre `access_token`
7. Callback récupère `github_login` via API
8. Callback met à jour Redis user avec `github_login` + `github_token`
9. Callback envoie notification Telegram de succès

## Troubleshooting

### Erreur "client_id vide"
- Vérifier que `.env` contient `GITHUB_APP_CLIENT_ID` (pas `GITHUB_CLIENT_ID`)
- Redémarrer serveur après modification `.env`

### Erreur 404 sur callback
- Vérifier que ngrok est actif: `curl https://headboard-romp-crust.ngrok-free.dev/health`
- Vérifier route callback existe: `grep -n "github/callback" src/routes/auth.js`

### Erreur "User not found" après OAuth
- Vérifier que utilisateur existe dans Redis: `redis-cli GET mdbai:user:telegram:7324485506`
- Vérifier que `state` contient bien le `telegram_id`

## Logs à surveiller

```bash
tail -f server.log | grep -E "AUTH|GITHUB|TELEGRAM"
```

Rechercher:
- `[AUTH] OAuth réussi` (échange code réussi)
- `[AUTH] Flux Telegram détecté` (state = telegram_id)
- `[AUTH] User Redis mis à jour` (github_login sauvegardé)
- `[AUTH] Notification Telegram envoyée` (message succès)