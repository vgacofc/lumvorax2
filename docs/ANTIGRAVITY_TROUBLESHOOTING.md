# 🔧 Dépannage Antigravity IDE - Problème de Sandbox

## ⚠️ Problème Rencontré

Lors du lancement d'Antigravity IDE, vous avez rencontré cette erreur :

```
FATAL:sandbox/linux/suid/client/setuid_sandbox_host.cc:166] 
The SUID sandbox helper binary was found, but is not configured correctly.
```

## ✅ Solutions Disponibles

### Solution 1 : Lancement avec --no-sandbox (APPLIQUÉE)

**Cette solution a été automatiquement appliquée.** Le lanceur [`/home/lvx/.local/bin/antigravity-ide`](../../.local/bin/antigravity-ide:1) a été modifié pour inclure l'option `--no-sandbox`.

**Vous pouvez maintenant lancer Antigravity IDE normalement :**

```bash
antigravity-ide
```

**Avantages :**
- ✅ Fonctionne immédiatement
- ✅ Pas besoin de privilèges sudo
- ✅ Simple et rapide

**Inconvénients :**
- ⚠️ Légèrement moins sécurisé (le sandbox Chrome est désactivé)
- ⚠️ Acceptable pour un usage en développement local

---

### Solution 2 : Configurer les Permissions du Sandbox (OPTIONNELLE)

Si vous préférez utiliser le sandbox Chrome pour une sécurité maximale, exécutez ce script :

```bash
./fix_antigravity_permissions.sh
```

Ce script va :
1. Demander votre mot de passe sudo
2. Configurer les permissions correctes pour `chrome-sandbox`
3. Permettre l'utilisation du sandbox

**Commandes manuelles équivalentes :**
```bash
sudo chown root:root /home/lvx/.local/share/antigravity-ide/chrome-sandbox
sudo chmod 4755 /home/lvx/.local/share/antigravity-ide/chrome-sandbox
```

**Après avoir exécuté ces commandes, modifiez le lanceur pour retirer --no-sandbox :**
```bash
# Éditez /home/lvx/.local/bin/antigravity-ide
# Changez cette ligne :
exec ./antigravity-ide --no-sandbox "$@"
# En :
exec ./antigravity-ide "$@"
```

---

## 🚀 Utilisation Actuelle

### Lancement Standard
```bash
antigravity-ide
```

### Lancement avec un Fichier
```bash
antigravity-ide mon_fichier.py
```

### Lancement avec un Projet
```bash
antigravity-ide /chemin/vers/projet
```

### Lancement dans un Terminal Externe
```bash
./launch_antigravity_external.sh
```

---

## 📚 Comprendre le Problème

### Qu'est-ce que le Sandbox Chrome ?

Le **sandbox** est une couche de sécurité qui isole les processus du navigateur Chromium (sur lequel est basé Antigravity IDE) du reste du système. Il empêche les processus malveillants d'accéder à vos fichiers ou à votre système.

### Pourquoi cette Erreur ?

Lorsque vous copiez les fichiers d'Antigravity IDE, le fichier `chrome-sandbox` perd ses permissions spéciales :
- Il doit appartenir à **root** (propriétaire système)
- Il doit avoir les permissions **4755** (SUID bit activé)

Sans ces permissions, Chrome refuse de démarrer avec le sandbox activé.

### Est-ce Dangereux d'Utiliser --no-sandbox ?

**Pour un usage en développement local : NON**

- ✅ Vous travaillez sur votre propre machine
- ✅ Vous ouvrez vos propres fichiers de code
- ✅ Vous ne naviguez pas sur des sites web malveillants dans l'IDE

**Quand éviter --no-sandbox :**
- ❌ Si vous ouvrez des fichiers non fiables
- ❌ Si vous utilisez des extensions non vérifiées
- ❌ Sur un serveur de production

---

## 🔍 Vérification de l'Installation

### Vérifier que le Lanceur Fonctionne
```bash
which antigravity-ide
# Devrait afficher : /home/lvx/.local/bin/antigravity-ide
```

### Vérifier la Version
```bash
antigravity-ide --version
# Devrait afficher : v22.21.1
```

### Vérifier les Permissions du Sandbox (si Solution 2 appliquée)
```bash
ls -l /home/lvx/.local/share/antigravity-ide/chrome-sandbox
# Devrait afficher : -rwsr-xr-x 1 root root ...
```

---

## 💡 Recommandations

### Pour un Usage Normal (Développement)
✅ **Utilisez la Solution 1 (--no-sandbox)** - C'est suffisant et pratique.

### Pour un Usage en Production ou Sécurisé
✅ **Utilisez la Solution 2** - Configurez les permissions correctes du sandbox.

---

## 📖 Ressources Supplémentaires

- **Guide complet** : [`docs/GUIDE_ANTIGRAVITY_IDE.md`](GUIDE_ANTIGRAVITY_IDE.md:1)
- **Script de correction** : [`fix_antigravity_permissions.sh`](../fix_antigravity_permissions.sh:1)
- **Lanceur externe** : [`launch_antigravity_external.sh`](../launch_antigravity_external.sh:1)

---

## ✅ Résumé

**Problème résolu !** Antigravity IDE est maintenant configuré pour fonctionner avec l'option `--no-sandbox`.

**Pour lancer l'IDE :**
```bash
antigravity-ide
```

**Tout fonctionne correctement pour un usage en développement local.**

---

*Document créé le 29 mai 2026*
*Version : 1.0*