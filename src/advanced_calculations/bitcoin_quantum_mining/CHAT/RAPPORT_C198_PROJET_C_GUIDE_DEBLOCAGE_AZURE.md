# GUIDE DÉBLOCAGE PROJET C — AZURE FPGA
## Phase 15Y-F28 — Bitcoin Mining Xilinx U250

**Date** : 2026-05-11 18:23 CEST  
**Conversation** : C198  
**Status** : ❌ **BLOQUÉ** — Authentification Azure requise  
**Avancement** : **90%** (code prêt, déploiement bloqué)

---

## 🚨 PROBLÈME ACTUEL

### Erreur Rencontrée
```bash
$ az account show
ERROR: Please run 'az login' to setup account.
```

### Cause
- Pas de session Azure active
- `az login` nécessite authentification interactive (navigateur)
- Pas de credentials service principal dans Doppler

### Impact
**Projet C (Azure FPGA)** bloqué à 90% :
- ✅ Code Verilog SHA-256 prêt (350 lignes)
- ✅ Driver PCIe prêt (500 lignes)
- ✅ Script déploiement prêt (450 lignes)
- ❌ **Déploiement impossible** sans authentification

---

## 🔓 SOLUTION : AUTHENTIFICATION AZURE

### Option 1 : Login Interactif (RECOMMANDÉ)

**Étape 1** : Ouvrir terminal et exécuter :
```bash
az login
```

**Étape 2** : Un navigateur s'ouvre automatiquement

**Étape 3** : Se connecter avec le compte Azure :
- Email : `supermicro202613@outlook.com`
- Mot de passe : [Votre mot de passe Azure]

**Étape 4** : Vérifier la connexion :
```bash
az account show
```

**Résultat attendu** :
```json
{
  "environmentName": "AzureCloud",
  "id": "SUBSCRIPTION_ID",
  "isDefault": true,
  "name": "SUBSCRIPTION_NAME",
  "state": "Enabled",
  "tenantId": "TENANT_ID",
  "user": {
    "name": "supermicro202613@outlook.com",
    "type": "user"
  }
}
```

### Option 2 : Service Principal (AUTOMATISATION)

Si vous voulez automatiser sans interaction :

**Étape 1** : Créer service principal dans Azure Portal
```bash
az ad sp create-for-rbac --name "lumvorax-fpga-sp" --role Contributor
```

**Résultat** :
```json
{
  "appId": "CLIENT_ID",
  "displayName": "lumvorax-fpga-sp",
  "password": "CLIENT_SECRET",
  "tenant": "TENANT_ID"
}
```

**Étape 2** : Ajouter credentials dans Doppler
```bash
doppler secrets set AZURE_CLIENT_ID="CLIENT_ID"
doppler secrets set AZURE_CLIENT_SECRET="CLIENT_SECRET"
doppler secrets set AZURE_TENANT_ID="TENANT_ID"
doppler secrets set AZURE_SUBSCRIPTION_ID="SUBSCRIPTION_ID"
```

**Étape 3** : Login avec service principal
```bash
az login --service-principal \
  -u $AZURE_CLIENT_ID \
  -p $AZURE_CLIENT_SECRET \
  --tenant $AZURE_TENANT_ID
```

---

## 🚀 APRÈS AUTHENTIFICATION : DÉPLOIEMENT AUTOMATIQUE

### Vérification Subscription

**Commande** :
```bash
az account list --output table
```

**Vérifier** :
- Subscription active (State = "Enabled")
- Quota FPGA disponible (Standard_NP10s)

### Lancement Déploiement

**Commande** :
```bash
cd src/advanced_calculations/bitcoin_quantum_mining/azure_fpga
./deploy_azure_fpga.sh
```

### Phases Automatiques (8 phases)

**Phase 1** : Vérification prérequis
- Azure CLI installé ✅
- Authentification active ⏳
- Doppler configuré ✅

**Phase 2** : Création Resource Group
```bash
az group create \
  --name lumvorax-fpga-rg \
  --location eastus
```

**Phase 3** : Création VM FPGA
```bash
az vm create \
  --resource-group lumvorax-fpga-rg \
  --name lumvorax-fpga-vm \
  --size Standard_NP10s \
  --image UbuntuLTS \
  --admin-username azureuser \
  --generate-ssh-keys
```

**Spécifications Standard_NP10s** :
- **FPGA** : Xilinx Alveo U250
- **vCPUs** : 10
- **RAM** : 168 GB
- **Stockage** : 736 GB SSD
- **Coût** : ~$3.06/heure

**Phase 4** : Installation Xilinx Runtime (XRT)
```bash
ssh azureuser@VM_IP << 'EOF'
wget https://www.xilinx.com/bin/public/openDownload?filename=xrt_202220.2.14.354_20.04-amd64-xrt.deb
sudo apt install -y ./xrt_*.deb
EOF
```

**Phase 5** : Upload Design Verilog
```bash
scp verilog/btc_sha256_fpga_lumvorax.v azureuser@VM_IP:~/
```

**Phase 6** : Synthèse avec Vivado
```bash
ssh azureuser@VM_IP << 'EOF'
source /opt/Xilinx/Vivado/2022.2/settings64.sh
vivado -mode batch -source synthesize_fpga.tcl
EOF
```

**Durée estimée** : 2-4 heures

**Phase 7** : Flash Bitstream sur FPGA
```bash
ssh azureuser@VM_IP << 'EOF'
sudo xbutil program --device 0 --user btc_sha256_fpga.xclbin
EOF
```

**Phase 8** : Tests Validation LumVorax
```bash
ssh azureuser@VM_IP << 'EOF'
cd driver
make
sudo insmod btc_fpga_pcie_driver.ko
./test_fpga_mining
EOF
```

---

## 📊 MÉTRIQUES ATTENDUES

### Performance FPGA Xilinx U250

**Spécifications** :
- **LUTs** : 1,728,000
- **DSPs** : 12,288
- **BRAM** : 2,688 Mb
- **Fréquence** : 300 MHz

**Performance SHA-256** :
- **Cores parallèles** : 64
- **Hashes/seconde** : ~19.2 GH/s (64 cores × 300 MHz)
- **Comparaison GPU** : 880× plus rapide que UHD 620 (21.86 MH/s)

### Coûts Azure

**VM Standard_NP10s** :
- **Coût horaire** : $3.06
- **Coût journalier** : $73.44 (24h)
- **Coût mensuel** : ~$2,203 (30 jours)

**Recommandation** :
- Tests courts (2-4h) : $6-12
- Validation complète (1 jour) : $73
- Production : Évaluer ROI vs coût

---

## 🔍 DIAGNOSTIC PROBLÈMES COURANTS

### Problème 1 : "No subscriptions found"

**Cause** : Compte sans subscription active

**Solution** :
1. Aller sur https://portal.azure.com
2. Créer subscription (Free Trial ou Pay-As-You-Go)
3. Relancer `az login`

### Problème 2 : "Quota exceeded for Standard_NP10s"

**Cause** : Quota FPGA par défaut = 0

**Solution** :
```bash
az vm list-usage --location eastus --query "[?name.value=='standardNPFamily']"
```

Si quota = 0, demander augmentation :
1. Azure Portal → Support → New support request
2. Issue type : Service and subscription limits (quotas)
3. Quota type : Compute-VM (cores-vCPUs)
4. Region : East US
5. SKU family : NP Series
6. New limit : 10 vCPUs

**Délai** : 1-2 jours ouvrables

### Problème 3 : "SSH connection refused"

**Cause** : VM pas encore démarrée ou NSG bloquant

**Solution** :
```bash
# Vérifier état VM
az vm show -g lumvorax-fpga-rg -n lumvorax-fpga-vm --query "provisioningState"

# Ouvrir port SSH
az network nsg rule create \
  --resource-group lumvorax-fpga-rg \
  --nsg-name lumvorax-fpga-nsg \
  --name AllowSSH \
  --priority 1000 \
  --source-address-prefixes '*' \
  --destination-port-ranges 22 \
  --access Allow \
  --protocol Tcp
```

---

## 📝 CHECKLIST DÉBLOCAGE

### Avant Déploiement
- [ ] Compte Azure créé (supermicro202613@outlook.com)
- [ ] Subscription active (Free Trial ou Pay-As-You-Go)
- [ ] `az login` réussi
- [ ] `az account show` affiche subscription
- [ ] Quota Standard_NP10s disponible (10 vCPUs minimum)

### Pendant Déploiement
- [ ] Resource Group créé
- [ ] VM Standard_NP10s provisionnée
- [ ] XRT installé sur VM
- [ ] Design Verilog uploadé
- [ ] Synthèse Vivado complétée (2-4h)
- [ ] Bitstream flashé sur FPGA
- [ ] Driver PCIe chargé

### Après Déploiement
- [ ] Tests validation réussis
- [ ] Métriques performance capturées
- [ ] Logs forensiques générés
- [ ] Rapport comparatif OpenCL vs FPGA
- [ ] Cleanup ressources (si tests terminés)

---

## 🎯 PROCHAINES ACTIONS

### Action Immédiate (UTILISATEUR)
1. **Ouvrir terminal**
2. **Exécuter** : `az login`
3. **Authentifier** dans navigateur
4. **Vérifier** : `az account show`
5. **Notifier Bob** : "Azure connecté, lance déploiement"

### Action Automatique (BOB)
Après notification utilisateur :
1. Vérifier subscription active
2. Lancer `deploy_azure_fpga.sh`
3. Monitorer progression 8 phases
4. Générer rapport forensique complet
5. Comparer performance OpenCL vs FPGA

---

## 📚 RÉFÉRENCES

### Fichiers Projet C

- [`azure_fpga/verilog/btc_sha256_fpga_lumvorax.v`](../azure_fpga/verilog/btc_sha256_fpga_lumvorax.v) — Design SHA-256 (350 lignes)
- [`azure_fpga/driver/btc_fpga_pcie_driver.c`](../azure_fpga/driver/btc_fpga_pcie_driver.c) — Driver PCIe (500 lignes)
- [`azure_fpga/deploy_azure_fpga.sh`](../azure_fpga/deploy_azure_fpga.sh) — Script déploiement (450 lignes)

### Documentation Azure

- [Azure FPGA Documentation](https://docs.microsoft.com/en-us/azure/virtual-machines/sizes-fpga)
- [Xilinx Alveo U250](https://www.xilinx.com/products/boards-and-kits/alveo/u250.html)
- [Azure CLI Reference](https://docs.microsoft.com/en-us/cli/azure/)

### Rapports LumVorax

- [`RAPPORT_C198_PROJET_A_NEO_WRAPPER_COMPLET.md`](RAPPORT_C198_PROJET_A_NEO_WRAPPER_COMPLET.md) — Baseline OpenCL
- [`RAPPORT_SYNTHESE_FINALE_C197_17_51.md`](RAPPORT_SYNTHESE_FINALE_C197_17_51.md) — Performance validée

---

## 💡 NOTES IMPORTANTES

### Sécurité
- Ne jamais commiter credentials Azure dans Git
- Utiliser Doppler pour secrets
- Activer MFA sur compte Azure
- Restreindre accès NSG (IP whitelisting)

### Coûts
- **Toujours** arrêter VM après tests : `az vm deallocate`
- Supprimer ressources inutilisées : `az group delete`
- Monitorer coûts : Azure Cost Management

### Performance
- FPGA 880× plus rapide que GPU (théorique)
- Validation réelle nécessaire (Projet C)
- Comparaison OpenCL vs FPGA critique pour ROI

---

**Guide créé le** : 2026-05-11 18:23:34 CEST  
**Auteur** : Bob (Mode Advanced)  
**Status** : ⏳ En attente authentification utilisateur  
**Prochaine étape** : `az login` puis déploiement automatique