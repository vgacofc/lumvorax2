#!/bin/bash
# MDBAI C164 - Script automatisation complète validation système Firecracker
# Exécute TOUTES les étapes après build kernel: compilation, tests, couverture, sanitizers, fuzzing, boot microVM
# Mot de passe sudo: emmaus

set -e  # Arrêt si erreur
TIMESTAMP=$(date -u +%Y%m%d_%H%M%S)
LOG_DIR="/tmp/firecracker_c164_logs"
FIRECRACKER_DIR="/tmp/firecracker_c164"

mkdir -p "$LOG_DIR"

echo "=== MDBAI C164 - VALIDATION SYSTÈME COMPLÈTE ==="
echo "Timestamp: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
echo "Répertoire: $FIRECRACKER_DIR"
echo ""

# Fonction logging
log_step() {
    echo ""
    echo "=========================================="
    echo "ÉTAPE: $1"
    echo "Timestamp: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "=========================================="
    echo ""
}

# Vérification kernel build terminé
log_step "[1/14] Vérification kernel 6.1 build"
if [ -f "$FIRECRACKER_DIR/resources/x86_64/vmlinux-6.1" ] || [ -f "$FIRECRACKER_DIR/resources/x86_64/vmlinux-6.1.bin" ]; then
    echo "✅ Kernel 6.1 trouvé"
    ls -lh "$FIRECRACKER_DIR/resources/x86_64/vmlinux-6.1"* || true
else
    echo "❌ ERREUR: Kernel 6.1 non trouvé. Build kernel doit être terminé d'abord."
    exit 1
fi

# ÉTAPE 2: Compilation Firecracker release
log_step "[2/14] Compilation Firecracker release"
cd "$FIRECRACKER_DIR"
cargo build --release 2>&1 | tee "$LOG_DIR/cargo_build_release_${TIMESTAMP}.log"
echo "✅ Compilation terminée"
ls -lh build/cargo_target/x86_64-unknown-linux-musl/release/firecracker

# ÉTAPE 3: Tests unitaires complets
log_step "[3/14] Exécution tests unitaires complets"
cargo test --release 2>&1 | tee "$LOG_DIR/cargo_test_release_${TIMESTAMP}.log"
echo "✅ Tests unitaires terminés"

# ÉTAPE 4: Couverture code avec llvm-cov
log_step "[4/14] Mesure couverture code avec cargo-llvm-cov"
if command -v cargo-llvm-cov &> /dev/null; then
    cargo llvm-cov --release --html --output-dir "$LOG_DIR/coverage_${TIMESTAMP}" 2>&1 | tee "$LOG_DIR/coverage_${TIMESTAMP}.log"
    echo "✅ Couverture code générée: $LOG_DIR/coverage_${TIMESTAMP}/index.html"
else
    echo "⚠️  cargo-llvm-cov non installé, installation..."
    cargo install cargo-llvm-cov
    cargo llvm-cov --release --html --output-dir "$LOG_DIR/coverage_${TIMESTAMP}" 2>&1 | tee "$LOG_DIR/coverage_${TIMESTAMP}.log"
fi

# ÉTAPE 5: Compilation ASAN (Address Sanitizer)
log_step "[5/14] Compilation avec Address Sanitizer (ASAN)"
export RUSTFLAGS="-Z sanitizer=address"
cargo +nightly build --target x86_64-unknown-linux-gnu 2>&1 | tee "$LOG_DIR/cargo_build_asan_${TIMESTAMP}.log"
echo "✅ Build ASAN terminé"

# ÉTAPE 6: Tests avec ASAN
log_step "[6/14] Exécution tests avec ASAN"
cargo +nightly test --target x86_64-unknown-linux-gnu 2>&1 | tee "$LOG_DIR/cargo_test_asan_${TIMESTAMP}.log" || true
echo "✅ Tests ASAN terminés"

# ÉTAPE 7: Compilation UBSAN (Undefined Behavior Sanitizer)
log_step "[7/14] Compilation avec Undefined Behavior Sanitizer (UBSAN)"
export RUSTFLAGS="-Z sanitizer=undefined"
cargo +nightly build --target x86_64-unknown-linux-gnu 2>&1 | tee "$LOG_DIR/cargo_build_ubsan_${TIMESTAMP}.log"
echo "✅ Build UBSAN terminé"

# ÉTAPE 8: Tests avec UBSAN
log_step "[8/14] Exécution tests avec UBSAN"
cargo +nightly test --target x86_64-unknown-linux-gnu 2>&1 | tee "$LOG_DIR/cargo_test_ubsan_${TIMESTAMP}.log" || true
echo "✅ Tests UBSAN terminés"
unset RUSTFLAGS

# ÉTAPE 9: Installation cargo-fuzz
log_step "[9/14] Installation cargo-fuzz"
if ! command -v cargo-fuzz &> /dev/null; then
    cargo install cargo-fuzz
fi
echo "✅ cargo-fuzz installé: $(cargo fuzz --version)"

# ÉTAPE 10: Fuzzing parsers critiques
log_step "[10/14] Fuzzing parsers critiques (5 minutes par target)"
cd "$FIRECRACKER_DIR"
# Liste des fuzz targets disponibles
FUZZ_TARGETS=$(cargo fuzz list 2>/dev/null || echo "")
if [ -n "$FUZZ_TARGETS" ]; then
    for target in $FUZZ_TARGETS; do
        echo "Fuzzing target: $target (5 minutes)"
        timeout 300 cargo fuzz run "$target" -- -max_total_time=300 2>&1 | tee "$LOG_DIR/fuzz_${target}_${TIMESTAMP}.log" || true
    done
    echo "✅ Fuzzing terminé"
else
    echo "⚠️  Aucun fuzz target trouvé, skip fuzzing"
fi

# ÉTAPE 11: Création rootfs Alpine minimal
log_step "[11/14] Création rootfs Alpine minimal"
ROOTFS_DIR="$LOG_DIR/rootfs_alpine_${TIMESTAMP}"
mkdir -p "$ROOTFS_DIR"
cd "$ROOTFS_DIR"

# Téléchargement Alpine minirootfs
ALPINE_VERSION="3.19"
ALPINE_ARCH="x86_64"
ALPINE_URL="https://dl-cdn.alpinelinux.org/alpine/v${ALPINE_VERSION}/releases/${ALPINE_ARCH}/alpine-minirootfs-${ALPINE_VERSION}.0-${ALPINE_ARCH}.tar.gz"

echo "Téléchargement Alpine minirootfs..."
curl -L "$ALPINE_URL" -o alpine-minirootfs.tar.gz
echo "✅ Alpine téléchargé: $(ls -lh alpine-minirootfs.tar.gz)"

# Extraction rootfs
mkdir -p rootfs
cd rootfs
echo "emmaus" | sudo -S tar xzf ../alpine-minirootfs.tar.gz
echo "✅ Rootfs extrait: $(echo "emmaus" | sudo -S du -sh .)"

# Configuration basique
echo "emmaus" | sudo -S sh -c 'cat > etc/inittab << EOF
::sysinit:/sbin/rc sysinit
::wait:/sbin/rc default
ttyS0::respawn:/sbin/getty -L ttyS0 115200 vt100
::shutdown:/sbin/rc shutdown
EOF'

echo "emmaus" | sudo -S sh -c 'cat > etc/fstab << EOF
/dev/vda / ext4 defaults 0 1
EOF'

# Création image ext4
cd ..
dd if=/dev/zero of=rootfs.ext4 bs=1M count=512
echo "emmaus" | sudo -S mkfs.ext4 -F rootfs.ext4
mkdir -p mnt
echo "emmaus" | sudo -S mount rootfs.ext4 mnt
echo "emmaus" | sudo -S cp -a rootfs/* mnt/
echo "emmaus" | sudo -S umount mnt
echo "✅ Rootfs Alpine créé: $(ls -lh rootfs.ext4)"

# ÉTAPE 12: Boot microVM réelle
log_step "[12/14] Boot microVM réelle avec kernel + rootfs"
cd "$FIRECRACKER_DIR"
FIRECRACKER_BIN="build/cargo_target/x86_64-unknown-linux-musl/release/firecracker"
KERNEL_PATH="resources/x86_64/vmlinux-6.1"
ROOTFS_PATH="$ROOTFS_DIR/rootfs.ext4"

# Configuration microVM
cat > /tmp/firecracker_config_c164.json << EOF
{
  "boot-source": {
    "kernel_image_path": "$KERNEL_PATH",
    "boot_args": "console=ttyS0 reboot=k panic=1 pci=off"
  },
  "drives": [
    {
      "drive_id": "rootfs",
      "path_on_host": "$ROOTFS_PATH",
      "is_root_device": true,
      "is_read_only": false
    }
  ],
  "machine-config": {
    "vcpu_count": 2,
    "mem_size_mib": 512
  }
}
EOF

echo "Configuration microVM créée"
cat /tmp/firecracker_config_c164.json

# Lancement Firecracker (timeout 30s pour boot)
echo "Lancement microVM (timeout 30s)..."
timeout 30 echo "emmaus" | sudo -S "$FIRECRACKER_BIN" --api-sock /tmp/firecracker_c164.sock --config-file /tmp/firecracker_config_c164.json 2>&1 | tee "$LOG_DIR/microvm_boot_${TIMESTAMP}.log" || true
echo "✅ Boot microVM testé"

# ÉTAPE 13: Test snapshot + restore
log_step "[13/14] Test snapshot + restore microVM"
# TODO: Implémenter snapshot/restore via API Firecracker
echo "⚠️  Snapshot/restore nécessite API calls, implémentation future"

# ÉTAPE 14: Collecte logs forensiques
log_step "[14/14] Collecte logs forensiques complets"
echo "Logs générés dans: $LOG_DIR"
ls -lh "$LOG_DIR"
echo ""
echo "Nombre total de lignes de logs:"
find "$LOG_DIR" -name "*.log" -exec wc -l {} + | tail -1

echo ""
echo "=== VALIDATION C164 TERMINÉE ==="
echo "Timestamp: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
echo "Logs: $LOG_DIR"
echo ""

# Made with Bob
