#!/bin/bash
# C197.47 : Analyse Progressive des Dépendances Couche par Couche
# Extraction de TOUTES les dépendances et sous-dépendances

STRACE_LOG="/tmp/c197_47_layer_all_syscalls.log"
OUTPUT_DIR="/tmp/c197_47_analysis"

echo "═══════════════════════════════════════════════════════════════"
echo "  C197.47 : ANALYSE PROGRESSIVE DÉPENDANCES MULTI-COUCHE"
echo "═══════════════════════════════════════════════════════════════"
echo ""

# Créer répertoire de sortie
mkdir -p "$OUTPUT_DIR"

echo "[1/10] Extraction COUCHE 1 : Bibliothèques chargées (openat)..."
grep "openat.*\.so" "$STRACE_LOG" | \
    awk -F'"' '{print $2}' | \
    sort | uniq > "$OUTPUT_DIR/layer1_libraries.txt"

LAYER1_COUNT=$(wc -l < "$OUTPUT_DIR/layer1_libraries.txt")
echo "  ✓ $LAYER1_COUNT bibliothèques uniques détectées"
echo ""

echo "[2/10] Extraction COUCHE 2 : Appels système (syscalls)..."
awk '{print $2}' "$STRACE_LOG" | \
    cut -d'(' -f1 | \
    sort | uniq -c | \
    sort -rn > "$OUTPUT_DIR/layer2_syscalls_stats.txt"

LAYER2_COUNT=$(wc -l < "$OUTPUT_DIR/layer2_syscalls_stats.txt")
echo "  ✓ $LAYER2_COUNT types de syscalls uniques"
echo ""

echo "[3/10] Extraction COUCHE 3 : ioctls DRM/GPU..."
grep "ioctl.*DRM" "$STRACE_LOG" > "$OUTPUT_DIR/layer3_drm_ioctls.txt"
LAYER3_COUNT=$(wc -l < "$OUTPUT_DIR/layer3_drm_ioctls.txt")
echo "  ✓ $LAYER3_COUNT appels ioctl DRM capturés"
echo ""

echo "[4/10] Extraction COUCHE 4 : Allocations mémoire (mmap)..."
grep "mmap" "$STRACE_LOG" | \
    awk '{print $0}' > "$OUTPUT_DIR/layer4_memory_allocations.txt"
LAYER4_COUNT=$(wc -l < "$OUTPUT_DIR/layer4_memory_allocations.txt")
echo "  ✓ $LAYER4_COUNT allocations mémoire"
echo ""

echo "[5/10] Extraction COUCHE 5 : Fichiers accédés..."
grep -E "openat|open" "$STRACE_LOG" | \
    awk -F'"' '{print $2}' | \
    grep -v "\.so" | \
    sort | uniq > "$OUTPUT_DIR/layer5_files_accessed.txt"
LAYER5_COUNT=$(wc -l < "$OUTPUT_DIR/layer5_files_accessed.txt")
echo "  ✓ $LAYER5_COUNT fichiers uniques accédés"
echo ""

echo "[6/10] Analyse DÉPENDANCES : Arbre hiérarchique..."
cat > "$OUTPUT_DIR/layer6_dependency_tree.txt" << 'EOF'
ARBRE DÉPENDANCES COMPLET
========================

NIVEAU 0 : Application
  └─ tools/c197_17_reverse_engineering_opencl_deps

NIVEAU 1 : Bibliothèques Principales
EOF

while IFS= read -r lib; do
    echo "  ├─ $lib" >> "$OUTPUT_DIR/layer6_dependency_tree.txt"
done < "$OUTPUT_DIR/layer1_libraries.txt"

echo "  ✓ Arbre de dépendances créé"
echo ""

echo "[7/10] Analyse SOUS-DÉPENDANCES : Bibliothèques chargées dynamiquement..."
# Extraire les bibliothèques chargées par dlopen
grep "dlopen" "$STRACE_LOG" | \
    awk -F'"' '{print $2}' | \
    sort | uniq > "$OUTPUT_DIR/layer7_dynamic_libraries.txt"
LAYER7_COUNT=$(wc -l < "$OUTPUT_DIR/layer7_dynamic_libraries.txt")
echo "  ✓ $LAYER7_COUNT bibliothèques chargées dynamiquement"
echo ""

echo "[8/10] Analyse PERFORMANCE : Temps d'exécution par syscall..."
# Top 20 syscalls les plus fréquents
head -20 "$OUTPUT_DIR/layer2_syscalls_stats.txt" > "$OUTPUT_DIR/layer8_top_syscalls.txt"
echo "  ✓ Top 20 syscalls extraits"
echo ""

echo "[9/10] Analyse SÉCURITÉ : Accès fichiers sensibles..."
grep -E "/dev/|/proc/|/sys/" "$OUTPUT_DIR/layer5_files_accessed.txt" > "$OUTPUT_DIR/layer9_sensitive_files.txt"
LAYER9_COUNT=$(wc -l < "$OUTPUT_DIR/layer9_sensitive_files.txt")
echo "  ✓ $LAYER9_COUNT fichiers sensibles accédés"
echo ""

echo "[10/10] Génération RAPPORT FINAL..."
cat > "$OUTPUT_DIR/RAPPORT_FINAL_C197_47.txt" << EOF
═══════════════════════════════════════════════════════════════
  RAPPORT FINAL C197.47 : ANALYSE DÉPENDANCES MULTI-COUCHE
═══════════════════════════════════════════════════════════════

Date: $(date)
Fichier analysé: $STRACE_LOG
Total lignes: $(wc -l < "$STRACE_LOG")

RÉSUMÉ PAR COUCHE
=================

COUCHE 1 : Bibliothèques ($LAYER1_COUNT)
----------------------------------------
EOF

cat "$OUTPUT_DIR/layer1_libraries.txt" >> "$OUTPUT_DIR/RAPPORT_FINAL_C197_47.txt"

cat >> "$OUTPUT_DIR/RAPPORT_FINAL_C197_47.txt" << EOF

COUCHE 2 : Top 10 Syscalls
---------------------------
EOF

head -10 "$OUTPUT_DIR/layer2_syscalls_stats.txt" >> "$OUTPUT_DIR/RAPPORT_FINAL_C197_47.txt"

cat >> "$OUTPUT_DIR/RAPPORT_FINAL_C197_47.txt" << EOF

COUCHE 3 : ioctls DRM ($LAYER3_COUNT appels)
--------------------------------------------
EOF

head -5 "$OUTPUT_DIR/layer3_drm_ioctls.txt" >> "$OUTPUT_DIR/RAPPORT_FINAL_C197_47.txt"
echo "..." >> "$OUTPUT_DIR/RAPPORT_FINAL_C197_47.txt"

cat >> "$OUTPUT_DIR/RAPPORT_FINAL_C197_47.txt" << EOF

COUCHE 4 : Allocations Mémoire ($LAYER4_COUNT)
----------------------------------------------
Total mémoire allouée: $(grep "mmap" "$STRACE_LOG" | wc -l) appels mmap

COUCHE 5 : Fichiers Accédés ($LAYER5_COUNT)
-------------------------------------------
EOF

head -10 "$OUTPUT_DIR/layer5_files_accessed.txt" >> "$OUTPUT_DIR/RAPPORT_FINAL_C197_47.txt"

cat >> "$OUTPUT_DIR/RAPPORT_FINAL_C197_47.txt" << EOF

COUCHE 6 : Arbre Dépendances
-----------------------------
Voir: layer6_dependency_tree.txt

COUCHE 7 : Bibliothèques Dynamiques ($LAYER7_COUNT)
---------------------------------------------------
EOF

cat "$OUTPUT_DIR/layer7_dynamic_libraries.txt" >> "$OUTPUT_DIR/RAPPORT_FINAL_C197_47.txt"

cat >> "$OUTPUT_DIR/RAPPORT_FINAL_C197_47.txt" << EOF

COUCHE 8 : Performance
----------------------
Voir: layer8_top_syscalls.txt

COUCHE 9 : Fichiers Sensibles ($LAYER9_COUNT)
---------------------------------------------
EOF

cat "$OUTPUT_DIR/layer9_sensitive_files.txt" >> "$OUTPUT_DIR/RAPPORT_FINAL_C197_47.txt"

cat >> "$OUTPUT_DIR/RAPPORT_FINAL_C197_47.txt" << EOF

═══════════════════════════════════════════════════════════════
  ANALYSE COMPLÈTE TERMINÉE
═══════════════════════════════════════════════════════════════

Tous les fichiers d'analyse sont dans: $OUTPUT_DIR/

Fichiers générés:
  - layer1_libraries.txt          : Bibliothèques chargées
  - layer2_syscalls_stats.txt     : Statistiques syscalls
  - layer3_drm_ioctls.txt         : Appels DRM/GPU
  - layer4_memory_allocations.txt : Allocations mémoire
  - layer5_files_accessed.txt     : Fichiers accédés
  - layer6_dependency_tree.txt    : Arbre dépendances
  - layer7_dynamic_libraries.txt  : Bibliothèques dynamiques
  - layer8_top_syscalls.txt       : Top syscalls
  - layer9_sensitive_files.txt    : Fichiers sensibles
  - RAPPORT_FINAL_C197_47.txt     : Ce rapport

EOF

echo "  ✓ Rapport final généré"
echo ""

echo "═══════════════════════════════════════════════════════════════"
echo "  ANALYSE TERMINÉE AVEC SUCCÈS"
echo "═══════════════════════════════════════════════════════════════"
echo ""
echo "Rapport final: $OUTPUT_DIR/RAPPORT_FINAL_C197_47.txt"
echo ""
echo "Pour voir le rapport:"
echo "  cat $OUTPUT_DIR/RAPPORT_FINAL_C197_47.txt"
echo ""

# Made with Bob
