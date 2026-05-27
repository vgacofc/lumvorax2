# GUIDE CONNEXION AZURE - PROJET C FPGA
## Phase 15Y-F28 : Setup Azure FPGA

**Date** : 2026-05-11  
**Environnement** : emmaus (Python 3.12.3)  
**Azure CLI** : 2.86.0  
**Statut** : ✅ Azure CLI installé, prêt pour connexion

---

## 📋 ÉTAPE 1 : CONNEXION AZURE (ACTION REQUISE)

### Commande à exécuter :

```bash
cd /home/lvx/LVX/lumvorax2
source emmaus/bin/activate
az login
```

### Ce qui va se passer :

1. **Ouverture navigateur** : Une page web s'ouvrira automatiquement
2. **Authentification** : Connectez-vous avec vos identifiants Azure
3. **Code device** : Si le navigateur ne s'ouvre pas, un code sera affiché
4. **Confirmation** : Une fois connecté, le terminal affichera vos subscriptions

### Informations nécessaires :

- **Email Azure** : Votre compte Microsoft/Azure
- **Mot de passe** : Votre mot de passe Azure
- **Subscription ID** : Sera affiché après connexion

---

## 📋 ÉTAPE 2 : VÉRIFICATION SUBSCRIPTION

### Après connexion, exécuter :

```bash
az account list --output table
```

### Informations à noter :

- **Subscription ID** : UUID de votre subscription
- **Subscription Name** : Nom de votre subscription
- **IsDefault** : True/False

### Si plusieurs subscriptions :

```bash
# Définir la subscription par défaut
az account set --subscription "VOTRE_SUBSCRIPTION_ID"
```

---

## 📋 ÉTAPE 3 : VÉRIFICATION FPGA DISPONIBLES

### Commande :

```bash
# Lister les VM FPGA disponibles
az vm list-sizes --location eastus --output table | grep NP
```

### Types FPGA Azure :

- **Standard_NP10s** : 1x Xilinx U250 FPGA
- **Standard_NP20s** : 2x Xilinx U250 FPGA
- **Standard_NP40s** : 4x Xilinx U250 FPGA

---

## 📋 ÉTAPE 4 : INFORMATIONS REQUISES POUR BOB

### Une fois connecté, fournir à Bob :

1. **Subscription ID** : `az account show --query id -o tsv`
2. **Tenant ID** : `az account show --query tenantId -o tsv`
3. **Location préférée** : eastus, westus2, etc.
4. **Resource Group** : Nom du groupe de ressources (ou créer nouveau)

### Commandes pour obtenir les infos :

```bash
# Afficher toutes les infos
az account show --output json

# Subscription ID
az account show --query id -o tsv

# Tenant ID
az account show --query tenantId -o tsv

# Lister resource groups existants
az group list --output table
```

---

## 📋 ÉTAPE 5 : CRÉATION RESOURCE GROUP (OPTIONNEL)

### Si besoin de créer un nouveau groupe :

```bash
# Créer resource group pour LumVorax FPGA
az group create \
  --name lumvorax-fpga-rg \
  --location eastus
```

---

## 🔐 SÉCURITÉ

### Tokens et credentials :

- **Ne jamais partager** : Subscription ID, Tenant ID, Access tokens
- **Stockage sécurisé** : Les credentials sont dans `~/.azure/`
- **Rotation** : Changer les tokens régulièrement

### Vérifier la connexion :

```bash
# Test connexion
az account show

# Lister les ressources
az resource list --output table
```

---

## 📊 PROCHAINES ÉTAPES APRÈS CONNEXION

### Bob pourra alors :

1. ✅ Créer VM FPGA (Standard_NP10s)
2. ✅ Installer Xilinx Runtime (XRT)
3. ✅ Développer design SHA-256 hardware
4. ✅ Flasher bitstream sur FPGA
5. ✅ Développer driver PCIe
6. ✅ Intégrer LumVorax
7. ✅ Benchmarks vs GPU

---

## 🆘 DÉPANNAGE

### Erreur "az: command not found" :

```bash
# Réactiver environnement
source /home/lvx/LVX/lumvorax2/emmaus/bin/activate
```

### Erreur de connexion :

```bash
# Nettoyer cache
az account clear
az login
```

### Problème navigateur :

```bash
# Utiliser device code
az login --use-device-code
```

---

## 📝 LOGS

### Tous les logs sont dans :

- **Installation** : `azure_cli_install.log`
- **Version** : `azure_cli_version.log`
- **Connexion** : `azure_login.log` (à créer)

### Commande avec logging :

```bash
az login 2>&1 | tee src/advanced_calculations/bitcoin_quantum_mining/azure_fpga/azure_login.log
```

---

## ✅ CHECKLIST

- [x] Azure CLI installé (v2.86.0)
- [x] Environnement emmaus créé
- [ ] **Connexion Azure** ← VOUS ÊTES ICI
- [ ] Subscription ID obtenu
- [ ] Resource Group créé/sélectionné
- [ ] VM FPGA créée
- [ ] XRT installé
- [ ] Design SHA-256 développé
- [ ] Bitstream flashé
- [ ] Driver PCIe développé
- [ ] LumVorax intégré
- [ ] Benchmarks effectués

---

## 🎯 OBJECTIF FINAL

**Valider technologie LumVorax sur FPGA Azure** :
- ✅ CPU (existant)
- ⏳ GPU (Projets A+B en cours)
- ⏳ FPGA (Projet C - VOUS ÊTES ICI)

**Timeline** : 1 mois pour validation complète FPGA

---

**NOTEZ-MOI DÈS QUE VOUS ÊTES CONNECTÉ !**