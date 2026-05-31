# 🚀 Guide Complet d'Utilisation d'Antigravity IDE

## 📋 Table des Matières

1. [Introduction](#introduction)
2. [Installation](#installation)
3. [Lancement de l'IDE](#lancement-de-lide)
4. [Interface Utilisateur](#interface-utilisateur)
5. [Fonctionnalités Principales](#fonctionnalités-principales)
6. [Raccourcis Clavier Essentiels](#raccourcis-clavier-essentiels)
7. [Configuration et Personnalisation](#configuration-et-personnalisation)
8. [Extensions et Plugins](#extensions-et-plugins)
9. [Gestion de Projets](#gestion-de-projets)
10. [Débogage](#débogage)
11. [Intégration Git](#intégration-git)
12. [Conseils et Astuces](#conseils-et-astuces)
13. [Dépannage](#dépannage)

---

## 🎯 Introduction

**Antigravity IDE** est un environnement de développement intégré (IDE) moderne et puissant, basé sur Electron. Il offre une expérience de développement fluide avec support multi-langages, coloration syntaxique avancée, et de nombreuses fonctionnalités pour améliorer votre productivité.

### Caractéristiques Principales

- ✅ **Multi-langages** : Support natif pour Python, JavaScript, TypeScript, C/C++, Java, Go, Rust, et plus
- ✅ **Interface moderne** : Design épuré et personnalisable
- ✅ **Performances optimales** : Basé sur Chromium pour une expérience rapide
- ✅ **Extensible** : Système de plugins et d'extensions
- ✅ **Intégration Git** : Gestion de version intégrée
- ✅ **Terminal intégré** : Accès direct au shell
- ✅ **IntelliSense** : Autocomplétion intelligente
- ✅ **Débogage avancé** : Outils de débogage intégrés

---

## 💾 Installation

### ✅ Installation Effectuée

L'installation d'Antigravity IDE a été complétée avec succès sur votre système :

**Emplacement d'installation** : `/home/lvx/.local/share/antigravity-ide/`

**Composants installés** :
- Binaire principal : `antigravity-ide` (191 MB)
- Ressources de l'application
- Bibliothèques système (libEGL, libGLESv2, libvulkan)
- Localisations et traductions
- Gestionnaire de crash Chromium

### 📁 Structure des Fichiers

```
/home/lvx/.local/share/antigravity-ide/
├── antigravity-ide          # Exécutable principal
├── bin/                      # Binaires auxiliaires
├── resources/
│   ├── app/                  # Application principale
│   └── completions/          # Fichiers d'autocomplétion
├── locales/                  # Traductions
├── chrome_*.pak              # Ressources Chromium
├── lib*.so                   # Bibliothèques partagées
└── *.dat, *.json            # Fichiers de configuration
```

---

## 🚀 Lancement de l'IDE

### Méthode 1 : Ligne de Commande

Ouvrez un terminal et tapez :

```bash
antigravity-ide
```

Pour ouvrir un fichier spécifique :

```bash
antigravity-ide /chemin/vers/fichier.py
```

Pour ouvrir un dossier/projet :

```bash
antigravity-ide /chemin/vers/projet/
```

### Méthode 2 : Menu Applications

1. Ouvrez le menu des applications de votre système
2. Cherchez "Antigravity IDE" dans la catégorie "Développement"
3. Cliquez sur l'icône pour lancer l'IDE

### Méthode 3 : Clic Droit sur Fichier

Vous pouvez également faire un clic droit sur un fichier de code et sélectionner "Ouvrir avec Antigravity IDE" (si configuré).

---

## 🖥️ Interface Utilisateur

### Disposition Générale

L'interface d'Antigravity IDE est divisée en plusieurs zones principales :

```
┌─────────────────────────────────────────────────────────┐
│  Barre de Menu (Fichier, Édition, Affichage, etc.)     │
├──────────┬──────────────────────────────────┬───────────┤
│          │                                  │           │
│  Barre   │     Éditeur Principal           │  Panneau  │
│  Latérale│     (Zone de Code)              │  Latéral  │
│          │                                  │  Droit    │
│  - Fichiers                                 │  (optionnel)
│  - Recherche                                │           │
│  - Git                                      │           │
│  - Extensions                               │           │
│          │                                  │           │
├──────────┴──────────────────────────────────┴───────────┤
│  Panneau Inférieur (Terminal, Sortie, Problèmes)       │
└─────────────────────────────────────────────────────────┘
```

### Composants de l'Interface

#### 1. **Barre de Menu**
- **Fichier** : Nouveau, Ouvrir, Enregistrer, Fermer
- **Édition** : Copier, Coller, Rechercher, Remplacer
- **Affichage** : Basculer les panneaux, Zoom
- **Terminal** : Ouvrir/Fermer le terminal intégré
- **Aide** : Documentation, À propos

#### 2. **Barre Latérale Gauche**
- **Explorateur de fichiers** : Navigation dans votre projet
- **Recherche** : Recherche globale dans les fichiers
- **Contrôle de source (Git)** : Gestion des versions
- **Débogueur** : Outils de débogage
- **Extensions** : Gestionnaire d'extensions

#### 3. **Éditeur Principal**
- Zone de code avec numérotation des lignes
- Onglets pour plusieurs fichiers ouverts
- Minimap (carte du code) sur la droite
- Breadcrumb (fil d'Ariane) en haut

#### 4. **Panneau Inférieur**
- **Terminal** : Shell intégré
- **Sortie** : Messages de sortie des outils
- **Problèmes** : Erreurs et avertissements
- **Console de débogage** : Logs de débogage

---

## ⚡ Fonctionnalités Principales

### 1. Édition de Code Avancée

#### Coloration Syntaxique
- Support automatique pour plus de 50 langages
- Thèmes personnalisables (clair/sombre)
- Mise en évidence des mots-clés, fonctions, variables

#### IntelliSense (Autocomplétion)
```python
# Tapez "pri" et appuyez sur Ctrl+Espace
print()  # Suggestion automatique
```

**Fonctionnalités IntelliSense** :
- Suggestions de code contextuelles
- Documentation intégrée (hover)
- Signature des fonctions
- Snippets de code

#### Multi-Curseurs
- **Ctrl + Alt + ↑/↓** : Ajouter un curseur au-dessus/en-dessous
- **Alt + Clic** : Ajouter un curseur à la position du clic
- **Ctrl + D** : Sélectionner l'occurrence suivante du mot

```python
# Exemple : Modifier plusieurs lignes simultanément
variable1 = "valeur"
variable2 = "valeur"
variable3 = "valeur"
# Placez des curseurs sur chaque ligne et modifiez en une fois
```

### 2. Recherche et Remplacement

#### Recherche Simple
- **Ctrl + F** : Rechercher dans le fichier actuel
- **Ctrl + H** : Rechercher et remplacer

#### Recherche Globale
- **Ctrl + Shift + F** : Rechercher dans tous les fichiers
- Support des expressions régulières
- Filtrage par type de fichier

```regex
# Exemple de regex pour trouver toutes les fonctions Python
def\s+\w+\s*\(
```

### 3. Navigation Rapide

#### Aller à...
- **Ctrl + P** : Ouvrir rapidement un fichier
- **Ctrl + Shift + O** : Aller à un symbole dans le fichier
- **Ctrl + T** : Aller à un symbole dans le workspace
- **Ctrl + G** : Aller à une ligne spécifique

#### Navigation dans le Code
- **F12** : Aller à la définition
- **Alt + F12** : Aperçu de la définition
- **Shift + F12** : Trouver toutes les références
- **Ctrl + -** : Revenir en arrière
- **Ctrl + Shift + -** : Avancer

### 4. Refactoring

#### Renommage
- **F2** : Renommer un symbole dans tout le projet
```python
# Placez le curseur sur 'ancienNom' et appuyez sur F2
def ancienNom():  # Sera renommé partout
    pass

ancienNom()  # Aussi renommé automatiquement
```

#### Extraction
- Sélectionnez du code → Clic droit → "Extraire en fonction"
- Sélectionnez du code → Clic droit → "Extraire en variable"

### 5. Formatage de Code

#### Formatage Automatique
- **Shift + Alt + F** : Formater le document entier
- **Ctrl + K, Ctrl + F** : Formater la sélection

#### Configuration
```json
// settings.json
{
  "editor.formatOnSave": true,
  "editor.formatOnPaste": true
}
```

---

## ⌨️ Raccourcis Clavier Essentiels

### Édition de Base

| Raccourci | Action |
|-----------|--------|
| **Ctrl + X** | Couper la ligne |
| **Ctrl + C** | Copier la ligne |
| **Ctrl + V** | Coller |
| **Ctrl + Z** | Annuler |
| **Ctrl + Y** | Refaire |
| **Ctrl + /** | Commenter/Décommenter |
| **Alt + ↑/↓** | Déplacer la ligne |
| **Shift + Alt + ↑/↓** | Dupliquer la ligne |
| **Ctrl + Shift + K** | Supprimer la ligne |

### Navigation

| Raccourci | Action |
|-----------|--------|
| **Ctrl + P** | Ouvrir rapidement |
| **Ctrl + Tab** | Basculer entre onglets |
| **Ctrl + B** | Basculer barre latérale |
| **Ctrl + `** | Basculer terminal |
| **Ctrl + \** | Diviser l'éditeur |
| **Ctrl + 1/2/3** | Focus sur groupe d'éditeur |

### Recherche

| Raccourci | Action |
|-----------|--------|
| **Ctrl + F** | Rechercher |
| **Ctrl + H** | Remplacer |
| **Ctrl + Shift + F** | Recherche globale |
| **F3** | Occurrence suivante |
| **Shift + F3** | Occurrence précédente |

### Débogage

| Raccourci | Action |
|-----------|--------|
| **F5** | Démarrer/Continuer |
| **F9** | Basculer point d'arrêt |
| **F10** | Pas à pas principal |
| **F11** | Pas à pas détaillé |
| **Shift + F11** | Sortir |
| **Shift + F5** | Arrêter |

---

## ⚙️ Configuration et Personnalisation

### Accéder aux Paramètres

1. **Menu** : Fichier → Préférences → Paramètres
2. **Raccourci** : **Ctrl + ,**

### Paramètres Recommandés

```json
{
  // Éditeur
  "editor.fontSize": 14,
  "editor.fontFamily": "'Fira Code', 'Courier New', monospace",
  "editor.fontLigatures": true,
  "editor.lineNumbers": "on",
  "editor.minimap.enabled": true,
  "editor.wordWrap": "on",
  "editor.tabSize": 4,
  "editor.insertSpaces": true,
  "editor.formatOnSave": true,
  "editor.rulers": [80, 120],
  
  // Apparence
  "workbench.colorTheme": "Dark+ (default dark)",
  "workbench.iconTheme": "vs-seti",
  
  // Terminal
  "terminal.integrated.fontSize": 13,
  "terminal.integrated.shell.linux": "/bin/bash",
  
  // Fichiers
  "files.autoSave": "afterDelay",
  "files.autoSaveDelay": 1000,
  "files.trimTrailingWhitespace": true,
  "files.insertFinalNewline": true,
  
  // Git
  "git.enableSmartCommit": true,
  "git.confirmSync": false,
  
  // Python (exemple)
  "python.linting.enabled": true,
  "python.linting.pylintEnabled": true,
  "python.formatting.provider": "black"
}
```

### Thèmes

#### Changer de Thème
1. **Ctrl + K, Ctrl + T**
2. Sélectionnez un thème dans la liste

#### Thèmes Populaires
- **Dark+** (par défaut sombre)
- **Light+** (par défaut clair)
- **Monokai**
- **Solarized Dark/Light**
- **Dracula**

### Polices de Code

#### Polices Recommandées avec Ligatures
```json
{
  "editor.fontFamily": "'Fira Code', 'JetBrains Mono', 'Cascadia Code'",
  "editor.fontLigatures": true
}
```

**Installation de Fira Code** :
```bash
sudo apt install fonts-firacode
```

---

## 🧩 Extensions et Plugins

### Gestionnaire d'Extensions

1. Cliquez sur l'icône Extensions dans la barre latérale (ou **Ctrl + Shift + X**)
2. Recherchez une extension
3. Cliquez sur "Installer"

### Extensions Essentielles par Langage

#### Python
- **Python** : Support complet Python
- **Pylance** : IntelliSense avancé
- **Python Docstring Generator** : Génération automatique de docstrings
- **autoDocstring** : Documentation automatique

#### JavaScript/TypeScript
- **ESLint** : Linting JavaScript
- **Prettier** : Formatage de code
- **JavaScript (ES6) code snippets** : Snippets utiles

#### C/C++
- **C/C++** : Support Microsoft
- **C/C++ Themes** : Coloration syntaxique
- **CMake Tools** : Support CMake

#### Web Development
- **Live Server** : Serveur de développement local
- **HTML CSS Support** : Autocomplétion HTML/CSS
- **Auto Rename Tag** : Renommage automatique de balises

#### Git
- **GitLens** : Supercharge Git
- **Git Graph** : Visualisation de l'historique
- **Git History** : Historique des fichiers

#### Productivité
- **Path Intellisense** : Autocomplétion des chemins
- **Bracket Pair Colorizer** : Coloration des parenthèses
- **TODO Highlight** : Mise en évidence des TODO
- **Better Comments** : Commentaires améliorés

---

## 📁 Gestion de Projets

### Ouvrir un Projet

#### Méthode 1 : Menu
1. Fichier → Ouvrir le dossier
2. Sélectionnez le dossier du projet

#### Méthode 2 : Ligne de Commande
```bash
antigravity-ide /chemin/vers/projet
```

### Workspace (Espace de Travail)

Un workspace peut contenir plusieurs dossiers :

1. Fichier → Ajouter un dossier à l'espace de travail
2. Fichier → Enregistrer l'espace de travail sous...

```json
// exemple.code-workspace
{
  "folders": [
    {
      "path": "/home/lvx/projet1"
    },
    {
      "path": "/home/lvx/projet2"
    }
  ],
  "settings": {
    "editor.fontSize": 14
  }
}
```

### Explorateur de Fichiers

#### Actions Disponibles
- **Nouveau fichier** : Clic droit → Nouveau fichier
- **Nouveau dossier** : Clic droit → Nouveau dossier
- **Renommer** : F2 ou clic droit → Renommer
- **Supprimer** : Suppr ou clic droit → Supprimer
- **Copier/Coller** : Ctrl+C / Ctrl+V

#### Filtres
```json
{
  "files.exclude": {
    "**/.git": true,
    "**/__pycache__": true,
    "**/node_modules": true,
    "**/.venv": true
  }
}
```

---

## 🐛 Débogage

### Configuration du Débogueur

#### Créer une Configuration
1. Cliquez sur l'icône Débogage (barre latérale)
2. Cliquez sur "créer un fichier launch.json"
3. Sélectionnez l'environnement (Python, Node.js, etc.)

#### Exemple : Configuration Python
```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Python: Fichier actuel",
      "type": "python",
      "request": "launch",
      "program": "${file}",
      "console": "integratedTerminal",
      "justMyCode": true
    },
    {
      "name": "Python: Module",
      "type": "python",
      "request": "launch",
      "module": "mon_module",
      "args": ["--option", "valeur"]
    }
  ]
}
```

### Utilisation du Débogueur

#### Points d'Arrêt (Breakpoints)
- **F9** : Ajouter/Supprimer un point d'arrêt
- Cliquez dans la marge gauche (à côté des numéros de ligne)
- Point d'arrêt conditionnel : Clic droit → "Ajouter un point d'arrêt conditionnel"

```python
# Exemple de point d'arrêt conditionnel
for i in range(100):
    result = calcul(i)  # Breakpoint si i == 50
```

#### Contrôles de Débogage
- **F5** : Démarrer/Continuer
- **F10** : Pas à pas principal (Step Over)
- **F11** : Pas à pas détaillé (Step Into)
- **Shift + F11** : Sortir (Step Out)
- **Ctrl + Shift + F5** : Redémarrer
- **Shift + F5** : Arrêter

#### Inspection des Variables
- **Panneau Variables** : Affiche toutes les variables locales et globales
- **Hover** : Survolez une variable pour voir sa valeur
- **Watch** : Ajoutez des expressions à surveiller

#### Console de Débogage
- Évaluez des expressions pendant le débogage
- Modifiez des variables en temps réel
```python
# Dans la console de débogage
>>> variable_name
>>> variable_name = nouvelle_valeur
```

---

## 🔀 Intégration Git

### Configuration Initiale

```bash
# Configuration globale Git
git config --global user.name "Votre Nom"
git config --global user.email "votre.email@example.com"
```

### Interface Git dans l'IDE

#### Panneau Contrôle de Source
1. Cliquez sur l'icône Git (barre latérale)
2. Visualisez les modifications
3. Stagez les fichiers (clic sur +)
4. Écrivez un message de commit
5. Cliquez sur ✓ pour commiter

#### Opérations Git Courantes

##### Initialiser un Dépôt
1. Contrôle de source → "Initialiser le dépôt"
2. Ou : **Ctrl + Shift + P** → "Git: Initialize Repository"

##### Cloner un Dépôt
1. **Ctrl + Shift + P** → "Git: Clone"
2. Entrez l'URL du dépôt
3. Sélectionnez le dossier de destination

##### Commit
```
1. Modifiez des fichiers
2. Fichiers apparaissent dans "Modifications"
3. Cliquez sur + pour stager
4. Écrivez le message de commit
5. Cliquez sur ✓
```

##### Push/Pull
- **Sync** : Cliquez sur l'icône de synchronisation (barre d'état)
- Ou : **Ctrl + Shift + P** → "Git: Push" / "Git: Pull"

##### Branches
- **Créer** : Clic sur le nom de branche (barre d'état) → "Créer une nouvelle branche"
- **Changer** : Clic sur le nom de branche → Sélectionner une branche
- **Fusionner** : **Ctrl + Shift + P** → "Git: Merge Branch"

#### Visualisation des Différences

- **Clic sur un fichier modifié** : Affiche le diff
- **Ctrl + Shift + G, D** : Ouvrir les modifications
- Couleurs :
  - 🟢 Vert : Lignes ajoutées
  - 🔴 Rouge : Lignes supprimées
  - 🟡 Jaune : Lignes modifiées

#### GitLens (Extension Recommandée)

Fonctionnalités supplémentaires :
- Blame inline (qui a modifié chaque ligne)
- Historique des fichiers
- Comparaison de branches
- Graphe de commits

---

## 💡 Conseils et Astuces

### 1. Productivité

#### Snippets Personnalisés
Créez vos propres snippets :
1. Fichier → Préférences → Snippets utilisateur
2. Sélectionnez le langage

```json
{
  "Print to console": {
    "prefix": "log",
    "body": [
      "console.log('$1');",
      "$2"
    ],
    "description": "Log output to console"
  }
}
```

#### Emmet (HTML/CSS)
```html
<!-- Tapez : div.container>ul>li*3 puis Tab -->
<div class="container">
  <ul>
    <li></li>
    <li></li>
    <li></li>
  </ul>
</div>
```

#### Commande Palette
**Ctrl + Shift + P** : Accès à toutes les commandes
- Tapez "format" pour formater
- Tapez "theme" pour changer de thème
- Tapez "git" pour les commandes Git

### 2. Organisation

#### Groupes d'Éditeurs
- **Ctrl + \** : Diviser l'éditeur verticalement
- **Ctrl + K, Ctrl + \** : Diviser horizontalement
- Glissez-déposez les onglets entre groupes

#### Breadcrumbs
- Activez : Affichage → Basculer le fil d'Ariane
- Navigation rapide dans la structure du fichier

### 3. Terminal Intégré

#### Multiples Terminaux
- **Ctrl + Shift + `** : Nouveau terminal
- Cliquez sur + dans le panneau terminal
- Basculez entre terminaux avec le menu déroulant

#### Diviser le Terminal
- Cliquez sur l'icône de division dans le panneau terminal
- Travaillez sur plusieurs shells simultanément

### 4. Recherche Avancée

#### Expressions Régulières
Activez le mode regex (icône .*) :
```regex
# Trouver toutes les fonctions Python
def\s+(\w+)\s*\(

# Trouver les imports
^import\s+\w+
^from\s+\w+\s+import
```

#### Recherche par Fichier
```
# Dans la recherche globale
*.py     # Seulement fichiers Python
!test_*  # Exclure fichiers commençant par test_
```

### 5. Zen Mode

**Ctrl + K, Z** : Mode sans distraction
- Cache tous les panneaux
- Plein écran
- Focus total sur le code
- **Échap Échap** pour sortir

---

## 🔧 Dépannage

### Problèmes Courants

#### 1. L'IDE ne se Lance Pas

**Solution 1 : Vérifier les permissions**
```bash
ls -l /home/lvx/.local/bin/antigravity-ide
# Doit être exécutable (-rwxr-xr-x)

# Si nécessaire :
chmod +x /home/lvx/.local/bin/antigravity-ide
```

**Solution 2 : Vérifier les dépendances**
```bash
# Installer les bibliothèques manquantes
sudo apt update
sudo apt install libgtk-3-0 libnotify4 libnss3 libxss1 \
                 libxtst6 xdg-utils libatspi2.0-0 libdrm2 \
                 libgbm1 libxcb-dri3-0
```

**Solution 3 : Lancer depuis le terminal pour voir les erreurs**
```bash
/home/lvx/.local/share/antigravity-ide/antigravity-ide
```

#### 2. Problèmes de Performance

**Solution 1 : Désactiver extensions inutiles**
- Extensions → Désactiver les extensions non utilisées

**Solution 2 : Augmenter la mémoire**
```bash
# Lancer avec plus de mémoire
antigravity-ide --max-memory=4096
```

**Solution 3 : Exclure des dossiers**
```json
{
  "files.watcherExclude": {
    "**/.git/objects/**": true,
    "**/node_modules/**": true,
    "**/.venv/**": true
  }
}
```

#### 3. IntelliSense ne Fonctionne Pas

**Pour Python :**
```bash
# Installer les outils Python
pip install pylint autopep8 black

# Dans l'IDE, installer l'extension Python
```

**Pour JavaScript :**
```bash
# Installer Node.js si nécessaire
sudo apt install nodejs npm
```

#### 4. Terminal ne S'Ouvre Pas

**Solution :**
```json
{
  "terminal.integrated.shell.linux": "/bin/bash",
  "terminal.integrated.shellArgs.linux": []
}
```

#### 5. Git ne Fonctionne Pas

**Vérifier Git :**
```bash
git --version
# Si absent :
sudo apt install git
```

**Configuration :**
```bash
git config --global user.name "Votre Nom"
git config --global user.email "email@example.com"
```

### Réinitialisation

#### Réinitialiser les Paramètres
```bash
# Sauvegarder d'abord
cp -r ~/.config/antigravity-ide ~/.config/antigravity-ide.backup

# Supprimer la configuration
rm -rf ~/.config/antigravity-ide
```

#### Réinstallation Complète
```bash
# Supprimer l'installation
rm -rf /home/lvx/.local/share/antigravity-ide
rm /home/lvx/.local/bin/antigravity-ide
rm /home/lvx/.local/share/applications/antigravity-ide.desktop

# Réinstaller (extraire à nouveau l'archive)
```

### Logs et Diagnostic

#### Afficher les Logs
1. Aide → Basculer les outils de développement
2. Onglet Console pour voir les erreurs

#### Mode Verbose
```bash
antigravity-ide --verbose --log-level=debug
```

---

## 📚 Ressources Supplémentaires

### Documentation

- **Raccourcis clavier** : Aide → Référence des raccourcis clavier
- **Commandes** : Ctrl + Shift + P → Tapez "help"

### Communauté

- Forums de support
- GitHub Issues (si open source)
- Stack Overflow

### Apprentissage

#### Tutoriels Recommandés
1. **Débutants** : Commencez par créer un fichier simple
2. **Intermédiaires** : Explorez le débogage et Git
3. **Avancés** : Créez des extensions personnalisées

#### Exercices Pratiques
```python
# Exercice 1 : Navigation
# Créez un projet avec plusieurs fichiers
# Pratiquez Ctrl+P, F12, Alt+F12

# Exercice 2 : Refactoring
# Écrivez du code répétitif
# Utilisez F2 pour renommer
# Utilisez l'extraction de fonction

# Exercice 3 : Débogage
# Créez un bug intentionnel
# Utilisez des breakpoints
# Inspectez les variables
```

---

## 🎓 Workflow Recommandé

### Pour un Nouveau Projet

1. **Créer le projet**
   ```bash
   mkdir mon_projet
   cd mon_projet
   antigravity-ide .
   ```

2. **Initialiser Git**
   - Ctrl + Shift + P → "Git: Initialize Repository"

3. **Créer la structure**
   ```
   mon_projet/
   ├── src/
   ├── tests/
   ├── docs/
   ├── README.md
   └── .gitignore
   ```

4. **Configurer l'environnement**
   - Installer les extensions nécessaires
   - Configurer le linter
   - Configurer le formateur

5. **Premier commit**
   ```
   Initial commit
   ```

### Workflow Quotidien

1. **Ouvrir le projet** : `antigravity-ide /chemin/projet`
2. **Créer une branche** : Pour chaque nouvelle fonctionnalité
3. **Coder** : Utilisez IntelliSense et snippets
4. **Tester** : Utilisez le débogueur
5. **Commit réguliers** : Messages clairs
6. **Push** : Synchroniser avec le dépôt distant

---

## ✅ Checklist de Démarrage

- [ ] Antigravity IDE installé et lancé avec succès
- [ ] Interface explorée (barre latérale, éditeur, terminal)
- [ ] Thème personnalisé selon préférence
- [ ] Extensions essentielles installées
- [ ] Git configuré (nom, email)
- [ ] Premier projet ouvert
- [ ] Raccourcis clavier de base mémorisés
- [ ] Terminal intégré testé
- [ ] Débogueur configuré et testé
- [ ] Workflow Git pratiqué

---

## 🎯 Conclusion

Antigravity IDE est maintenant installé et prêt à l'emploi sur votre système. Avec ce guide, vous disposez de toutes les informations nécessaires pour :

✅ **Lancer l'IDE** de différentes manières
✅ **Naviguer** efficacement dans l'interface
✅ **Utiliser** les fonctionnalités avancées
✅ **Personnaliser** votre environnement
✅ **Déboguer** votre code
✅ **Gérer** vos projets avec Git
✅ **Résoudre** les problèmes courants

### Prochaines Étapes

1. **Lancez Antigravity IDE** : `antigravity-ide`
2. **Ouvrez un projet existant** ou créez-en un nouveau
3. **Explorez les fonctionnalités** une par une
4. **Installez les extensions** pour vos langages préférés
5. **Pratiquez les raccourcis clavier** pour gagner en productivité

**Bon développement avec Antigravity IDE ! 🚀**

---

*Guide créé le 29 mai 2026*
*Version : 1.0*