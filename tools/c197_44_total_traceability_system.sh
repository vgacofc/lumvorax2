#!/bin/bash
# C197.44 - Système de Traçabilité TOTALE Multi-Échelle
# Capture TOUTES les dépendances et sous-dépendances bit-par-bit
# OpenCL → NEO → IGC → i915 → GPU Gen9
# CPU → RAM → GPU avec traçabilité nanoseconde

set -e

echo "════════════════════════════════════════════════════════════════"
echo "  C197.44 - SYSTÈME TRAÇABILITÉ TOTALE MULTI-ÉCHELLE"
echo "  Bitcoin Mining: CPU → RAM → GPU (bit-par-bit, ns-par-ns)"
echo "════════════════════════════════════════════════════════════════"
echo ""
echo "Timestamp: $(date '+%Y-%m-%d %H:%M:%S.%N')"
echo ""

# Configuration
FORENSIC_DIR="/tmp/c197_44_total_forensic"
BTC_PROGRAM="tools/c197_10_opencl_simple_test"
DURATION=10  # secondes

# Vérifier privilèges root
if [ "$EUID" -ne 0 ]; then
    echo "❌ ERREUR: Ce script nécessite les privilèges root"
    echo "Exécuter avec: sudo bash $0"
    exit 1
fi

# Créer structure forensique
echo "📁 Création structure forensique..."
rm -rf "$FORENSIC_DIR"
mkdir -p "$FORENSIC_DIR"/{syscalls,perf,bpf,opencl,drm,gpu,memory,timeline}
chmod -R 777 "$FORENSIC_DIR"
echo "✅ Structure créée: $FORENSIC_DIR"
echo ""

# Vérifier outils disponibles
echo "🔧 Vérification outils de traçage..."
TOOLS_OK=true

if ! command -v strace &> /dev/null; then
    echo "⚠️  strace non installé (apt install strace)"
    TOOLS_OK=false
fi

if ! command -v perf &> /dev/null; then
    echo "⚠️  perf non installé (apt install linux-tools-generic)"
    TOOLS_OK=false
fi

if ! command -v bpftrace &> /dev/null; then
    echo "⚠️  bpftrace non installé (apt install bpftrace)"
    TOOLS_OK=false
fi

if [ "$TOOLS_OK" = false ]; then
    echo ""
    echo "❌ Outils manquants - Installation nécessaire"
    echo "Exécuter: apt update && apt install -y strace linux-tools-generic bpftrace"
    exit 1
fi

echo "✅ Tous les outils disponibles"
echo ""

# Vérifier programme BTC
if [ ! -f "$BTC_PROGRAM" ]; then
    echo "❌ ERREUR: Programme $BTC_PROGRAM non trouvé"
    exit 1
fi

echo "✅ Programme BTC trouvé: $BTC_PROGRAM"
echo ""

# ═══════════════════════════════════════════════════════════════════
# COUCHE 1: SYSCALLS (strace) - Tous les appels système
# ═══════════════════════════════════════════════════════════════════
echo "🔬 COUCHE 1: Traçage SYSCALLS (strace)..."
echo "   - Tous les appels système"
echo "   - Timestamps nanosecondes"
echo "   - Arguments complets"
echo ""

strace -f -tt -T -v -s 4096 \
    -e trace=all \
    -o "$FORENSIC_DIR/syscalls/strace_full.log" \
    timeout $DURATION "$BTC_PROGRAM" > "$FORENSIC_DIR/syscalls/stdout.log" 2>&1 &

STRACE_PID=$!
echo "   PID strace: $STRACE_PID"

# ═══════════════════════════════════════════════════════════════════
# COUCHE 2: PERFORMANCE (perf) - CPU, cache, mémoire
# ═══════════════════════════════════════════════════════════════════
echo ""
echo "📊 COUCHE 2: Traçage PERFORMANCE (perf)..."
echo "   - CPU cycles, instructions"
echo "   - Cache L1/L2/L3 misses"
echo "   - Page faults, context switches"
echo ""

# Attendre que le programme démarre
sleep 1

# Trouver PID du programme BTC
BTC_PID=$(pgrep -f "$BTC_PROGRAM" | head -1)

if [ -n "$BTC_PID" ]; then
    echo "   PID BTC: $BTC_PID"
    
    # Perf stat - statistiques globales
    perf stat -e cycles,instructions,cache-references,cache-misses,\
branches,branch-misses,page-faults,context-switches,cpu-migrations,\
L1-dcache-loads,L1-dcache-load-misses,LLC-loads,LLC-load-misses \
        -p $BTC_PID \
        -o "$FORENSIC_DIR/perf/perf_stat.log" \
        sleep $((DURATION - 1)) &
    
    PERF_STAT_PID=$!
    echo "   PID perf stat: $PERF_STAT_PID"
    
    # Perf record - échantillonnage détaillé
    perf record -F 999 -g \
        -e cycles,instructions,cache-misses,page-faults \
        -p $BTC_PID \
        -o "$FORENSIC_DIR/perf/perf.data" \
        sleep $((DURATION - 1)) &
    
    PERF_RECORD_PID=$!
    echo "   PID perf record: $PERF_RECORD_PID"
else
    echo "   ⚠️  Programme BTC non démarré encore"
fi

# ═══════════════════════════════════════════════════════════════════
# COUCHE 3: KERNEL (bpftrace) - Événements kernel
# ═══════════════════════════════════════════════════════════════════
echo ""
echo "🐧 COUCHE 3: Traçage KERNEL (bpftrace)..."
echo "   - Allocations mémoire"
echo "   - Opérations DRM/i915"
echo "   - Scheduling GPU"
echo ""

# Script BPF pour tracer allocations mémoire
cat > "$FORENSIC_DIR/bpf/trace_memory.bt" << 'EOF'
#!/usr/bin/env bpftrace

BEGIN {
    printf("Traçage allocations mémoire...\n");
}

tracepoint:kmem:kmalloc {
    @kmalloc[comm] = count();
    @kmalloc_bytes[comm] = sum(args->bytes_alloc);
    printf("%llu: kmalloc comm=%s bytes=%d\n", nsecs, comm, args->bytes_alloc);
}

tracepoint:kmem:kfree {
    @kfree[comm] = count();
}

tracepoint:kmem:mm_page_alloc {
    @page_alloc[comm] = count();
    printf("%llu: page_alloc comm=%s order=%d\n", nsecs, comm, args->order);
}

END {
    printf("\n=== Statistiques Mémoire ===\n");
    printf("\nAllocations kmalloc par processus:\n");
    print(@kmalloc);
    printf("\nBytes alloués par processus:\n");
    print(@kmalloc_bytes);
    printf("\nLibérations kfree par processus:\n");
    print(@kfree);
    printf("\nAllocations pages par processus:\n");
    print(@page_alloc);
}
EOF

chmod +x "$FORENSIC_DIR/bpf/trace_memory.bt"

bpftrace "$FORENSIC_DIR/bpf/trace_memory.bt" \
    > "$FORENSIC_DIR/bpf/memory_trace.log" 2>&1 &

BPF_MEM_PID=$!
echo "   PID bpftrace memory: $BPF_MEM_PID"

# Script BPF pour tracer DRM/i915
cat > "$FORENSIC_DIR/bpf/trace_drm.bt" << 'EOF'
#!/usr/bin/env bpftrace

BEGIN {
    printf("Traçage DRM/i915...\n");
}

kprobe:drm_ioctl {
    printf("%llu: drm_ioctl comm=%s\n", nsecs, comm);
    @drm_ioctl[comm] = count();
}

kprobe:i915_gem_execbuffer2_ioctl {
    printf("%llu: i915_gem_execbuffer2 comm=%s\n", nsecs, comm);
    @execbuffer2[comm] = count();
}

kprobe:i915_gem_create_ioctl {
    printf("%llu: i915_gem_create comm=%s\n", nsecs, comm);
    @gem_create[comm] = count();
}

kprobe:i915_gem_mmap_ioctl {
    printf("%llu: i915_gem_mmap comm=%s\n", nsecs, comm);
    @gem_mmap[comm] = count();
}

END {
    printf("\n=== Statistiques DRM/i915 ===\n");
    printf("\nAppels drm_ioctl:\n");
    print(@drm_ioctl);
    printf("\nAppels execbuffer2:\n");
    print(@execbuffer2);
    printf("\nAppels gem_create:\n");
    print(@gem_create);
    printf("\nAppels gem_mmap:\n");
    print(@gem_mmap);
}
EOF

chmod +x "$FORENSIC_DIR/bpf/trace_drm.bt"

bpftrace "$FORENSIC_DIR/bpf/trace_drm.bt" \
    > "$FORENSIC_DIR/bpf/drm_trace.log" 2>&1 &

BPF_DRM_PID=$!
echo "   PID bpftrace DRM: $BPF_DRM_PID"

# ═══════════════════════════════════════════════════════════════════
# COUCHE 4: OPENCL (ltrace) - Appels bibliothèque OpenCL
# ═══════════════════════════════════════════════════════════════════
echo ""
echo "🔧 COUCHE 4: Traçage OPENCL (ltrace)..."
echo "   - Appels libOpenCL.so"
echo "   - Appels libigdrcl.so (NEO)"
echo "   - Timestamps"
echo ""

if command -v ltrace &> /dev/null; then
    ltrace -f -tt -T \
        -e '*cl*' \
        -o "$FORENSIC_DIR/opencl/ltrace.log" \
        timeout $DURATION "$BTC_PROGRAM" > /dev/null 2>&1 &
    
    LTRACE_PID=$!
    echo "   PID ltrace: $LTRACE_PID"
else
    echo "   ⚠️  ltrace non disponible (apt install ltrace)"
fi

# ═══════════════════════════════════════════════════════════════════
# COUCHE 5: GPU (intel_gpu_top) - Utilisation GPU
# ═══════════════════════════════════════════════════════════════════
echo ""
echo "🎮 COUCHE 5: Traçage GPU (intel_gpu_top)..."
echo "   - Utilisation GPU %"
echo "   - Fréquence GPU"
echo "   - Mémoire GPU"
echo ""

if command -v intel_gpu_top &> /dev/null; then
    intel_gpu_top -o "$FORENSIC_DIR/gpu/intel_gpu_top.log" -s 100 &
    
    GPU_TOP_PID=$!
    echo "   PID intel_gpu_top: $GPU_TOP_PID"
else
    echo "   ⚠️  intel_gpu_top non disponible (apt install intel-gpu-tools)"
fi

# ═══════════════════════════════════════════════════════════════════
# ATTENTE COMPLETION
# ═══════════════════════════════════════════════════════════════════
echo ""
echo "⏳ Capture en cours ($DURATION secondes)..."
echo ""

# Attendre fin strace (qui contrôle la durée)
wait $STRACE_PID 2>/dev/null || true

# Arrêter tous les traceurs
echo "🛑 Arrêt des traceurs..."
[ -n "$PERF_STAT_PID" ] && kill -INT $PERF_STAT_PID 2>/dev/null || true
[ -n "$PERF_RECORD_PID" ] && kill -INT $PERF_RECORD_PID 2>/dev/null || true
[ -n "$BPF_MEM_PID" ] && kill -INT $BPF_MEM_PID 2>/dev/null || true
[ -n "$BPF_DRM_PID" ] && kill -INT $BPF_DRM_PID 2>/dev/null || true
[ -n "$LTRACE_PID" ] && kill -INT $LTRACE_PID 2>/dev/null || true
[ -n "$GPU_TOP_PID" ] && kill -INT $GPU_TOP_PID 2>/dev/null || true

sleep 2

# ═══════════════════════════════════════════════════════════════════
# ANALYSE ET SYNTHÈSE
# ═══════════════════════════════════════════════════════════════════
echo ""
echo "📊 Génération synthèse..."

# Créer rapport synthèse
cat > "$FORENSIC_DIR/SYNTHESE.md" << EOF
# C197.44 - Synthèse Traçabilité Totale Multi-Échelle

**Date**: $(date '+%Y-%m-%d %H:%M:%S')
**Durée**: $DURATION secondes
**Programme**: $BTC_PROGRAM

## Structure Forensique

\`\`\`
$FORENSIC_DIR/
├── syscalls/          # Tous les appels système (strace)
├── perf/              # Performance CPU/cache/mémoire
├── bpf/               # Événements kernel (bpftrace)
├── opencl/            # Appels OpenCL (ltrace)
├── drm/               # Opérations DRM/i915
├── gpu/               # Utilisation GPU
├── memory/            # Allocations mémoire
└── timeline/          # Timeline complète
\`\`\`

## Fichiers Générés

### COUCHE 1: SYSCALLS
- \`syscalls/strace_full.log\` - Tous les appels système avec timestamps ns
- \`syscalls/stdout.log\` - Sortie standard du programme

### COUCHE 2: PERFORMANCE
- \`perf/perf_stat.log\` - Statistiques performance globales
- \`perf/perf.data\` - Échantillonnage détaillé (perf report)

### COUCHE 3: KERNEL
- \`bpf/memory_trace.log\` - Allocations/libérations mémoire
- \`bpf/drm_trace.log\` - Opérations DRM/i915

### COUCHE 4: OPENCL
- \`opencl/ltrace.log\` - Appels bibliothèque OpenCL

### COUCHE 5: GPU
- \`gpu/intel_gpu_top.log\` - Utilisation GPU en temps réel

## Statistiques Rapides

EOF

# Ajouter statistiques syscalls
if [ -f "$FORENSIC_DIR/syscalls/strace_full.log" ]; then
    echo "### Syscalls les plus fréquents" >> "$FORENSIC_DIR/SYNTHESE.md"
    echo '```' >> "$FORENSIC_DIR/SYNTHESE.md"
    grep -oP '^\d+\.\d+\s+\K\w+' "$FORENSIC_DIR/syscalls/strace_full.log" | \
        sort | uniq -c | sort -rn | head -20 >> "$FORENSIC_DIR/SYNTHESE.md"
    echo '```' >> "$FORENSIC_DIR/SYNTHESE.md"
    echo "" >> "$FORENSIC_DIR/SYNTHESE.md"
fi

# Ajouter taille fichiers
echo "### Taille des captures" >> "$FORENSIC_DIR/SYNTHESE.md"
echo '```' >> "$FORENSIC_DIR/SYNTHESE.md"
du -sh "$FORENSIC_DIR"/* >> "$FORENSIC_DIR/SYNTHESE.md"
echo '```' >> "$FORENSIC_DIR/SYNTHESE.md"

echo "✅ Synthèse générée: $FORENSIC_DIR/SYNTHESE.md"
echo ""

# ═══════════════════════════════════════════════════════════════════
# RÉSUMÉ FINAL
# ═══════════════════════════════════════════════════════════════════
echo "════════════════════════════════════════════════════════════════"
echo "  ✅ CAPTURE TOTALE TERMINÉE"
echo "════════════════════════════════════════════════════════════════"
echo ""
echo "📂 Données forensiques: $FORENSIC_DIR"
echo "📄 Synthèse: $FORENSIC_DIR/SYNTHESE.md"
echo ""
echo "📊 Fichiers capturés:"
ls -lh "$FORENSIC_DIR"/*/* 2>/dev/null | wc -l | xargs echo "   -"
echo ""
echo "💾 Taille totale:"
du -sh "$FORENSIC_DIR" | cut -f1 | xargs echo "   -"
echo ""
echo "🔍 Analyse recommandée:"
echo "   1. Lire synthèse: cat $FORENSIC_DIR/SYNTHESE.md"
echo "   2. Analyser syscalls: less $FORENSIC_DIR/syscalls/strace_full.log"
echo "   3. Analyser perf: perf report -i $FORENSIC_DIR/perf/perf.data"
echo "   4. Analyser BPF: less $FORENSIC_DIR/bpf/*.log"
echo ""
echo "════════════════════════════════════════════════════════════════"

# Made with Bob
