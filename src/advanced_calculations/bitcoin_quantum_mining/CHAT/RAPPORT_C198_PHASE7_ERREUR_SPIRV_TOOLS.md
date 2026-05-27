# RAPPORT C198 PHASE 7 - DIAGNOSTIC ERREUR SPIRV-TOOLS
**Date**: 2026-05-07 21:12 UTC+2  
**Conversation**: C198 (suite C197)  
**Code Root**: emmaus  
**Progression Globale**: 79% (47/59 tâches)

---

## 🚨 PROBLÈME IDENTIFIÉ

### Erreur Critique
**Type**: Incompatibilité de version SPIRV-Tools  
**Localisation**: `~/SPIRV-Tools/source/operand.cpp`  
**Cause**: Version SPIRV-Tools v2024.1 (trop récente) incompatible avec IGC v1.0.17791.9 (2021)

### Symptômes
```
error: 'SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_REDUCE' was not declared
error: 'SPV_OPERAND_TYPE_TENSOR_CLAMP_MODE' was not declared
error: 'SPV_OPERAND_TYPE_TENSOR_ADDRESSING_OPERANDS' was not declared
error: 'SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS' was not declared
error: 'SPV_OPERAND_TYPE_FPENCODING' was not declared
error: 'SPV_OPERAND_TYPE_COOPERATIVE_VECTOR_MATRIX_LAYOUT' was not declared
error: 'SPV_OPERAND_TYPE_COMPONENT_TYPE' was not declared
error: 'SPV_OPERAND_TYPE_TENSOR_OPERANDS' was not declared
```

### Analyse Technique
- **SPIRV-Tools v2024.1** : Inclut nouveaux types SPIR-V 1.6+ (Cooperative Matrix, Tensor ops)
- **IGC v1.0.17791.9** : Compilé pour SPIR-V 1.5 (2021)
- **Incompatibilité** : Les nouveaux enum types n'existent pas dans les headers anciens

---

## 🔍 ANALYSE FORENSIQUE

### Chemin de Compilation
```
IGC v1.0.17791.9 (2021)
  ↓
external/SPIRV-Tools/CMakeLists.txt
  ↓
Cherche: ~/SPIRV-Tools (../../SPIRV-Tools)
  ↓
Trouve: ~/SPIRV-Tools v2024.1 (ERREUR - trop récent)
  ↓
Compilation échoue à 43%
```

### Versions Attendues
D'après la date de release IGC v1.0.17791.9 (Novembre 2021), les versions compatibles sont :
- **SPIRV-Tools** : v2021.3 ou v2021.4 (SDK 2021.14.0)
- **SPIRV-Headers** : v1.5.4 ou v1.5.5
- **LLVM** : 14.0.x ✅ (déjà installé)

### Fichiers Affectés
```
~/SPIRV-Tools/                    ← Version 2024.1 (MAUVAISE)
~/SPIRV-Headers/                  ← Version 2024.1 (MAUVAISE)
~/igc_full/build/                 ← Build corrompu (458 fichiers .o)
/tmp/igc_compile_*.log            ← Logs d'erreur
```

---

## 💡 SOLUTION

### Stratégie de Correction
1. **Supprimer** versions incompatibles (SPIRV-Tools v2024.1, SPIRV-Headers v2024.1)
2. **Cloner** versions compatibles (v2021.4)
3. **Nettoyer** build directory IGC
4. **Reconfigurer** CMake IGC
5. **Recompiler** avec -j4 (RAM limitée)

### Commandes de Correction
```bash
# 1. Arrêter compilation en cours
pkill -f "make.*igc"

# 2. Supprimer versions incompatibles
rm -rf ~/SPIRV-Tools ~/SPIRV-Headers

# 3. Cloner versions compatibles (2021.4 - SDK 2021.14.0)
cd ~
git clone --depth 1 --branch v2021.4 \
  https://github.com/KhronosGroup/SPIRV-Tools.git

git clone --depth 1 --branch sdk-1.2.198.1 \
  https://github.com/KhronosGroup/SPIRV-Headers.git

# 4. Nettoyer build IGC
cd ~/igc_full/build
rm -rf IGC/Release/external/SPIRV-Tools
find . -name "*.o" -delete
find . -name "CMakeCache.txt" -delete

# 5. Reconfigurer CMake
cd ~/igc_full/build
CC=gcc-9 CXX=g++-9 cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DIGC_OPTION__ARCHITECTURE_TARGET=Linux64 \
  -DIGC_OPTION__ARCHITECTURE_HOST=Linux64 \
  -DCMAKE_PREFIX_PATH=~/gmmlib_local/usr/local

# 6. Recompiler avec -j4
nohup make -j4 -k > /tmp/igc_compile_$(date +%Y%m%d_%H%M%S).log 2>&1 &
```

---

## 📊 IMPACT

### Temps Perdu
- Compilation IGC 43% : ~30 min
- Diagnostic erreur : ~10 min
- **Total** : ~40 min

### Temps Restant Estimé
- Clonage SPIRV-Tools v2021.4 : 2 min
- Nettoyage + reconfiguration : 3 min
- Recompilation IGC complète : 2h (avec -j4)
- **Total** : ~2h05

### Leçons Apprises
1. **Toujours vérifier compatibilité versions** avant clonage
2. **Lire documentation dépendances** (README, DEPS files)
3. **Utiliser versions SDK officielles** plutôt que latest
4. **Tester compilation incrémentale** avant full build

---

## 🔧 SCRIPT DE CORRECTION AUTOMATIQUE

### Fichier: `fix_spirv_tools_version.sh`
```bash
#!/bin/bash
# Script de correction version SPIRV-Tools pour IGC v1.0.17791.9
# Usage: ./fix_spirv_tools_version.sh

set -e

echo "=== CORRECTION VERSION SPIRV-TOOLS POUR IGC ==="
echo "Date: $(date)"
echo ""

# 1. Arrêter compilation en cours
echo "📛 Arrêt compilation IGC en cours..."
pkill -f "make.*igc" 2>/dev/null || true
sleep 2

# 2. Vérifier versions actuelles
echo "🔍 Versions actuelles:"
if [ -d ~/SPIRV-Tools ]; then
    echo "   SPIRV-Tools: $(cd ~/SPIRV-Tools && git describe --tags 2>/dev/null || echo 'unknown')"
fi
if [ -d ~/SPIRV-Headers ]; then
    echo "   SPIRV-Headers: $(cd ~/SPIRV-Headers && git describe --tags 2>/dev/null || echo 'unknown')"
fi
echo ""

# 3. Sauvegarder si nécessaire
if [ -d ~/SPIRV-Tools ]; then
    echo "💾 Sauvegarde SPIRV-Tools actuel..."
    mv ~/SPIRV-Tools ~/SPIRV-Tools.backup.$(date +%Y%m%d_%H%M%S)
fi
if [ -d ~/SPIRV-Headers ]; then
    echo "💾 Sauvegarde SPIRV-Headers actuel..."
    mv ~/SPIRV-Headers ~/SPIRV-Headers.backup.$(date +%Y%m%d_%H%M%S)
fi

# 4. Cloner versions compatibles
echo "📥 Clonage SPIRV-Tools v2021.4..."
cd ~
git clone --depth 1 --branch v2021.4 \
    https://github.com/KhronosGroup/SPIRV-Tools.git

echo "📥 Clonage SPIRV-Headers sdk-1.2.198.1..."
git clone --depth 1 --branch sdk-1.2.198.1 \
    https://github.com/KhronosGroup/SPIRV-Headers.git

# 5. Vérifier clonage
echo ""
echo "✅ Versions installées:"
echo "   SPIRV-Tools: $(cd ~/SPIRV-Tools && git describe --tags)"
echo "   SPIRV-Headers: $(cd ~/SPIRV-Headers && git describe --tags)"
echo ""

# 6. Nettoyer build IGC
echo "🧹 Nettoyage build directory IGC..."
cd ~/igc_full/build
rm -rf IGC/Release/external/SPIRV-Tools 2>/dev/null || true
find . -name "*.o" -delete 2>/dev/null || true
find . -name "CMakeCache.txt" -delete 2>/dev/null || true
echo "   Fichiers .o supprimés"

# 7. Reconfigurer CMake
echo "⚙️  Reconfiguration CMake IGC..."
CC=gcc-9 CXX=g++-9 cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DIGC_OPTION__ARCHITECTURE_TARGET=Linux64 \
    -DIGC_OPTION__ARCHITECTURE_HOST=Linux64 \
    -DCMAKE_PREFIX_PATH=~/gmmlib_local/usr/local \
    > /tmp/igc_cmake_reconfig.log 2>&1

if [ $? -eq 0 ]; then
    echo "✅ CMake reconfiguration réussie"
else
    echo "❌ Erreur CMake - voir /tmp/igc_cmake_reconfig.log"
    exit 1
fi

# 8. Vérifier RAM disponible
FREE_RAM=$(free -m | awk '/^Mem:/{print $7}')
echo ""
echo "💾 RAM disponible: ${FREE_RAM} MB"
if [ $FREE_RAM -lt 500 ]; then
    echo "⚠️  RAM faible - utilisation -j2"
    JOBS=2
elif [ $FREE_RAM -lt 1000 ]; then
    echo "⚙️  RAM moyenne - utilisation -j4"
    JOBS=4
else
    echo "🚀 RAM suffisante - utilisation -j8"
    JOBS=8
fi

# 9. Lancer compilation
echo ""
echo "🔨 Lancement compilation IGC avec -j${JOBS}..."
LOG_FILE="/tmp/igc_compile_$(date +%Y%m%d_%H%M%S).log"
nohup make -j${JOBS} -k > "$LOG_FILE" 2>&1 &
PID=$!

echo "✅ Compilation lancée (PID: $PID)"
echo "📝 Log: $LOG_FILE"
echo ""
echo "📊 Surveillance:"
echo "   watch -n 30 'cd ~/igc_full/build && find . -name \"*.o\" | wc -l'"
echo "   tail -f $LOG_FILE | grep -E '\[.*%\]'"
echo ""
echo "⏱️  Temps estimé: 2h (avec -j${JOBS})"