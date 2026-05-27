# COMMANDES POUR SUIVRE LE CLONE EN TEMPS RÉEL

## 📊 Commande 1 : Voir la progression en %

```bash
watch -n 2 'SIZE=$(du -sm /tmp/levelzero_gen9_build/compute-runtime 2>/dev/null | cut -f1); echo "Taille: ${SIZE} MB / 200 MB"; echo "Progression: $((SIZE * 100 / 200))%"'
```

**Explication** :
- Rafraîchit toutes les 2 secondes
- Calcule automatiquement le pourcentage
- Affiche : "Progression: XX%"

---

## 📋 Commande 2 : Voir le log en temps réel

```bash
tail -f ~/levelzero_gen9_install.log
```

**Explication** :
- Affiche les nouvelles lignes du log au fur et à mesure
- Appuyez sur `Ctrl+C` pour arrêter

---

## 🔍 Commande 3 : Voir taille + dernières lignes log

```bash
watch -n 2 'du -sh /tmp/levelzero_gen9_build/compute-runtime 2>/dev/null && echo "---" && tail -3 ~/levelzero_gen9_install.log'
```

**Explication** :
- Affiche la taille actuelle
- Affiche les 3 dernières lignes du log
- Rafraîchit toutes les 2 secondes

---

## 📈 Commande 4 : Vérification unique (sans watch)

```bash
du -sh /tmp/levelzero_gen9_build/compute-runtime 2>/dev/null && echo "---" && tail -5 ~/levelzero_gen9_install.log
```

**Explication** :
- Exécution unique (pas de rafraîchissement)
- Affiche taille + 5 dernières lignes log

---

## 🎯 ÉTAT ACTUEL

- **Répertoire clone** : `/tmp/levelzero_gen9_build/compute-runtime`
- **Log complet** : `~/levelzero_gen9_install.log`
- **Taille actuelle** : ~62 MB
- **Taille finale** : ~200 MB
- **Progression** : ~31%

---

## ⚠️ IMPORTANT

Le clone est DÉJÀ EN COURS dans le Terminal 3. Ces commandes sont pour le SUIVRE en parallèle dans un autre terminal.

**Terminal 3 actif** :
```bash
cd /home/lvx/LVX/lumvorax2/src/advanced_calculations/bitcoin_quantum_mining/level_zero_native/scripts && bash install_levelzero_gen9.sh 2>&1 | tee ~/levelzero_gen9_install.log
```

Ne PAS interrompre ce terminal !