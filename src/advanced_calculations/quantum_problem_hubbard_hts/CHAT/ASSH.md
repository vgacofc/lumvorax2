Voici la commande correcte pour créer une clé SSH avec ton email **`vgac4224@outlook.com`** dans Replit pour l’utiliser avec Git et GitHub.

---

# 1. Vérifier si une clé SSH existe déjà

```bash
ls -al ~/.ssh
```

Si tu vois :

```
id_ed25519
id_ed25519.pub
```

alors une clé existe déjà.

---

# 2. Créer une clé SSH (si elle n’existe pas)

Commande :

```bash
ssh-keygen -t ed25519 -C "vgac4224@outlook.com"
```

Ensuite :.

```
Enter file in which to save the key
```

appuie sur **Entrée** pour accepter :

```
/home/runner/.ssh/id_ed25519
```

Pour **passphrase** :

* soit tu mets un mot de passe
* soit **Entrée deux fois**.

---

# 3. Démarrer l’agent SSH

```bash
eval "$(ssh-agent -s)"
```

---

# 4. Ajouter la clé

```bash
ssh-add ~/.ssh/id_ed25519
```

---

# 5. Afficher la clé publique

```bash
cat ~/.ssh/id_ed25519.pub
```

Copie **tout le contenu affiché**.

---

# 6. Ajouter la clé dans GitHub

Aller dans :

```
GitHub → Settings → SSH and GPG keys → New SSH Key
```

Coller la clé.

---

# 7. Tester la connexion

```bash
ssh -T git@github.com
```

Message attendu :

```
Hi USERNAME! You've successfully authenticated.
```

---

# 8. Pousser vers la branche main

Dans ton projet :

```bash
git add .
git commit -m "initial commit"
git push -u origin main
```

---

# 9. Vérifier la configuration Git

```bash
git remote -v
```

Tu dois voir un lien SSH :

```
git@github.com:USERNAME/REPO.git
```

---

Si tu veux, je peux aussi te donner :

* **la commande unique qui configure SSH + GitHub + push automatiquement dans Replit**,
* ou **la procédure pour connecter Replit à GitHub en 30 secondes sans erreur SSH**.
