# LumVorax C198 — Level Zero Native Bitcoin Mining
# Makefile pour compilation indépendante
#
# STANDARD_NAMES.md v4.5 §M-BTC-LEVELZERO-C198
# Phase 13B : Kernel SHA256 OpenCL validé

# ══════════════════════════════════════════════════════════════════════
# CONFIGURATION
# ══════════════════════════════════════════════════════════════════════

CC = gcc
CFLAGS = -Wall -Wextra -O3 -march=native -std=gnu11
CFLAGS += -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS += -pthread

# Chemins
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN_DIR = bin
LOG_DIR = logs/forensic
REPORT_DIR = reports

# Level Zero
LEVELZERO_CFLAGS = -I/usr/include/level_zero
LEVELZERO_LIBS = -lze_loader

# Autres dépendances
LIBS = -lm -lz -lpthread -ldl

# Flags debug
DEBUG_CFLAGS = -g -O0 -DDEBUG -fsanitize=address -fsanitize=undefined
FORENSIC_CFLAGS = -DBTC_L0_FORENSIC_FULL

# ══════════════════════════════════════════════════════════════════════
# FICHIERS
# ══════════════════════════════════════════════════════════════════════

SOURCES = $(SRC_DIR)/btc_levelzero_runner.c $(SRC_DIR)/btc_levelzero_kernel.c $(SRC_DIR)/btc_levelzero_driver_loader.c
HEADERS = $(INC_DIR)/btc_levelzero_runner.h $(SRC_DIR)/btc_levelzero_driver_loader.h
OBJECTS = $(BUILD_DIR)/btc_levelzero_runner.o $(BUILD_DIR)/btc_levelzero_kernel.o $(BUILD_DIR)/btc_levelzero_driver_loader.o

# Binaires
BIN_TEST = $(BIN_DIR)/test_levelzero_init
BIN_TEST_KERNEL = $(BIN_DIR)/test_levelzero_kernel
BIN_BENCHMARK = $(BIN_DIR)/benchmark_levelzero_vs_opencl
BIN_PHASE12H = $(BIN_DIR)/c198_phase12h_bitcoin_mining
BIN_MINING = $(BIN_DIR)/btc_levelzero_mining

# ══════════════════════════════════════════════════════════════════════
# RÈGLES PRINCIPALES
# ══════════════════════════════════════════════════════════════════════

.PHONY: all clean test benchmark install help dirs

all: dirs $(BIN_TEST)

dirs:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR) $(LOG_DIR) $(REPORT_DIR)
	@echo "[C198] Répertoires créés"

# ── Compilation objet ──────────────────────────────────────────────────

$(BUILD_DIR)/btc_levelzero_runner.o: $(SRC_DIR)/btc_levelzero_runner.c $(HEADERS)
	@echo "[C198] Compilation: $<"
	$(CC) $(CFLAGS) $(LEVELZERO_CFLAGS) -I$(INC_DIR) -c $< -o $@

$(BUILD_DIR)/btc_levelzero_kernel.o: $(SRC_DIR)/btc_levelzero_kernel.c $(HEADERS)
	@echo "[C198] Compilation: $<"
	$(CC) $(CFLAGS) $(LEVELZERO_CFLAGS) -I$(INC_DIR) -c $< -o $@

$(BUILD_DIR)/btc_levelzero_driver_loader.o: $(SRC_DIR)/btc_levelzero_driver_loader.c $(SRC_DIR)/btc_levelzero_driver_loader.h
	@echo "[C198] Compilation: $<"
	$(CC) $(CFLAGS) $(LEVELZERO_CFLAGS) -I$(INC_DIR) -c $< -o $@

# ── Test initialisation ────────────────────────────────────────────────

$(BIN_TEST): $(OBJECTS) tests/test_init.c
	@echo "[C198] Linkage: $@"
	$(CC) $(CFLAGS) $(LEVELZERO_CFLAGS) -I$(INC_DIR) \
		tests/test_init.c $(OBJECTS) \
		$(LEVELZERO_LIBS) $(LIBS) -o $@
	@echo "[C198] ✅ Binaire créé: $@"

# ── Test kernel SPIR-V ─────────────────────────────────────────────────

test_kernel: dirs $(BIN_TEST_KERNEL)

$(BIN_TEST_KERNEL): $(OBJECTS) tests/test_kernel.c
	@echo "[C198] Linkage: $@"
	$(CC) $(CFLAGS) $(LEVELZERO_CFLAGS) -I$(INC_DIR) \
		tests/test_kernel.c $(OBJECTS) \
		$(LEVELZERO_LIBS) $(LIBS) -o $@
	@echo "[C198] ✅ Binaire créé: $@"

# ── Benchmark vs OpenCL ────────────────────────────────────────────────

benchmark: dirs $(BIN_BENCHMARK)

$(BIN_BENCHMARK): $(OBJECTS) tests/benchmark_vs_opencl.c
	@echo "[C198] Compilation benchmark"
	$(CC) $(CFLAGS) $(LEVELZERO_CFLAGS) -I$(INC_DIR) \
		tests/benchmark_vs_opencl.c $(OBJECTS) \
		$(LEVELZERO_LIBS) $(LIBS) -o $@
	@echo "[C198] ✅ Benchmark créé: $@"

# ── Mining complet ─────────────────────────────────────────────────────

mining: dirs $(BIN_MINING)

$(BIN_MINING): $(OBJECTS) tests/test_mining.c
	@echo "[C198] Compilation mining"
	$(CC) $(CFLAGS) $(LEVELZERO_CFLAGS) -I$(INC_DIR) \
		tests/test_mining.c $(OBJECTS) \
		$(LEVELZERO_LIBS) $(LIBS) -o $@
	@echo "[C198] ✅ Mining créé: $@"
# ── Test allocation minimal ────────────────────────────────────────────

c198_test_levelzero_alloc_minimal: dirs
	@echo "[C198] Compilation test allocation minimal"
	$(CC) $(CFLAGS) $(LEVELZERO_CFLAGS) -I$(INC_DIR) -I$(SRC_DIR) \
		tests/c198_test_levelzero_alloc_minimal.c \
		$(SRC_DIR)/btc_hybrid_minimal.c \
		$(SRC_DIR)/btc_hybrid_shared_memory.c \
		$(LEVELZERO_LIBS) $(LIBS) -lOpenCL -o $(BIN_DIR)/c198_test_levelzero_alloc_minimal
	@echo "[C198] ✅ Test allocation minimal créé"

# ── Benchmark Device vs Shared ─────────────────────────────────────────

c198_benchmark_device_vs_shared: dirs
	@echo "[C198] Compilation benchmark Device vs Shared"
	$(CC) $(CFLAGS) $(LEVELZERO_CFLAGS) -I$(INC_DIR) -I$(SRC_DIR) \
		tests/c198_benchmark_device_vs_shared.c \
		$(SRC_DIR)/btc_hybrid_minimal.c \
		$(SRC_DIR)/btc_hybrid_shared_memory.c \
		$(LEVELZERO_LIBS) $(LIBS) -o $(BIN_DIR)/c198_benchmark_device_vs_shared
	@echo "[C198] ✅ Benchmark Device vs Shared créé"

# ── Test UMA Benchmark Phase 12G ───────────────────────────────────────

c198_phase12g_uma_benchmark: dirs $(OBJECTS)
	@echo "[C198 Phase 12G] Compilation test UMA benchmark"
	$(CC) $(CFLAGS) $(LEVELZERO_CFLAGS) -I$(INC_DIR) -I$(SRC_DIR) \
		tests/test_c198_phase12g_uma_benchmark.c \
		$(OBJECTS) \
		$(LEVELZERO_LIBS) $(LIBS) -o $(BIN_DIR)/c198_phase12g_uma_benchmark
	@echo "[C198 Phase 12G] ✅ Test UMA benchmark créé"

run_c198_phase12g_uma_benchmark: c198_phase12g_uma_benchmark
	@echo ""
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  TEST C198 Phase 12G — UMA Benchmark"
	@echo "  Objectif: Éliminer cold start DMA (78 ms → < 1 ms)"
	@echo "═══════════════════════════════════════════════════════════"
	@echo ""
	@$(BIN_DIR)/c198_phase12g_uma_benchmark 2>&1 | tee logs/c198_phase12g_uma_benchmark.log
	@echo ""
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  Logs: logs/c198_phase12g_uma_benchmark.log"
	@echo "  Logs forensiques: logs/c198_phase12g_uma_benchmark.lum"
	@echo "═══════════════════════════════════════════════════════════"
	@echo ""

# ── Debug build ────────────────────────────────────────────────────────

debug: CFLAGS += $(DEBUG_CFLAGS)
debug: clean all
	@echo "[C198] ✅ Build debug avec AddressSanitizer"

# ── Forensic build ─────────────────────────────────────────────────────

forensic: CFLAGS += $(FORENSIC_CFLAGS)
forensic: clean all
	@echo "[C198] ✅ Build forensique avec logging complet"

# ══════════════════════════════════════════════════════════════════════
# TESTS ET VALIDATION
# ══════════════════════════════════════════════════════════════════════

test: $(BIN_TEST)
	@echo ""
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  TEST C198 — Level Zero Initialisation"
	@echo "═══════════════════════════════════════════════════════════"
	@echo ""
	@$(BIN_TEST)
	@echo ""
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  Logs forensiques: $(LOG_DIR)/"
	@echo "═══════════════════════════════════════════════════════════"
	@ls -lh $(LOG_DIR)/ 2>/dev/null || echo "Aucun log généré"
	@echo ""

run-test-kernel: $(BIN_TEST_KERNEL)
	@echo ""
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  TEST C198 — Level Zero Kernel SPIR-V"
	@echo "═══════════════════════════════════════════════════════════"
	@echo ""
	@$(BIN_TEST_KERNEL)
	@echo ""

run-benchmark: $(BIN_BENCHMARK)
	@echo ""
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  BENCHMARK C198 — Level Zero vs OpenCL"
	@echo "═══════════════════════════════════════════════════════════"
	@echo ""
	@$(BIN_BENCHMARK)
	@echo ""

run-mining: $(BIN_MINING)
	@echo ""
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  MINING C198 — Bitcoin Level Zero"
	@echo "═══════════════════════════════════════════════════════════"
	@echo ""
	@$(BIN_MINING)
	@echo ""

# ══════════════════════════════════════════════════════════════════════
# RAPPORTS FORENSIQUES
# ══════════════════════════════════════════════════════════════════════

report: test
	@echo "[C198] Génération rapport forensique..."
	@python3 ../../../tools/generate_levelzero_report.py \
		--log-dir $(LOG_DIR) \
		--output $(REPORT_DIR)/RAPPORT_C198_LEVELZERO_INIT.md
	@echo "[C198] ✅ Rapport: $(REPORT_DIR)/RAPPORT_C198_LEVELZERO_INIT.md"

# ══════════════════════════════════════════════════════════════════════
# NETTOYAGE
# ══════════════════════════════════════════════════════════════════════

clean:
	@echo "[C198] Nettoyage..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "[C198] ✅ Nettoyage terminé"

clean-logs:
	@echo "[C198] Nettoyage logs (ATTENTION: perte données forensiques)"
	@rm -rf $(LOG_DIR)/*
	@echo "[C198] ✅ Logs supprimés"

clean-all: clean clean-logs
	@rm -rf $(REPORT_DIR)/*
	@echo "[C198] ✅ Nettoyage complet"

# ══════════════════════════════════════════════════════════════════════
# INSTALLATION
# ══════════════════════════════════════════════════════════════════════

install: all
	@echo "[C198] Installation Level Zero SDK..."
	@echo "Vérification: /usr/include/level_zero/ze_api.h"
	@test -f /usr/include/level_zero/ze_api.h || \
		(echo "ERREUR: Level Zero SDK non installé" && \
		 echo "Installation: sudo apt install level-zero-dev" && \
		 exit 1)
	@echo "[C198] ✅ Level Zero SDK détecté"

check-deps:
	@echo "[C198] Vérification dépendances..."
	@echo -n "  - Level Zero headers: "
	@test -f /usr/include/level_zero/ze_api.h && echo "✅" || echo "❌"
	@echo -n "  - Level Zero loader: "
	@ldconfig -p | grep -q libze_loader && echo "✅" || echo "❌"
	@echo -n "  - zlib: "
	@ldconfig -p | grep -q libz.so && echo "✅" || echo "❌"
	@echo -n "  - pthread: "
	@ldconfig -p | grep -q libpthread && echo "✅" || echo "❌"

# ══════════════════════════════════════════════════════════════════════
# AIDE
# ══════════════════════════════════════════════════════════════════════

help:
	@echo ""
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  LumVorax C198 — Level Zero Native Bitcoin Mining"
	@echo "═══════════════════════════════════════════════════════════"
	@echo ""
	@echo "Cibles principales:"
	@echo "  make all           - Compile test initialisation"
	@echo "  make test          - Exécute test initialisation"
	@echo "  make test_kernel   - Compile test kernel SPIR-V"
	@echo "  make benchmark     - Compile benchmark vs OpenCL"
	@echo "  make mining        - Compile mining complet"
	@echo "  make debug         - Build avec AddressSanitizer"
	@echo "  make forensic      - Build avec logging complet"
	@echo ""
	@echo "Tests:"
	@echo "  make run-test-kernel - Exécute test kernel SPIR-V"
	@echo "  make run-benchmark   - Exécute benchmark"
	@echo "  make run-mining      - Exécute mining"
	@echo "  make report          - Génère rapport forensique"
	@echo ""
	@echo "Utilitaires:"
	@echo "  make check-deps   - Vérifie dépendances"
	@echo "  make install      - Vérifie installation Level Zero"
	@echo "  make clean        - Nettoie build"
	@echo "  make clean-all    - Nettoie tout (build + logs + rapports)"
	@echo ""
	@echo "Dépendances requises:"
	@echo "  - Level Zero SDK (level-zero-dev)"
	@echo "  - Intel GPU driver avec support Level Zero"
	@echo "  - zlib, pthread"
	@echo ""
	@echo "═══════════════════════════════════════════════════════════"
	@echo ""

# Made with Bob - LumVorax C198
# C198 Phase 12H : Bitcoin Mining 1 Minute
c198_phase12h_bitcoin_mining: dirs $(OBJECTS)
	$(CC) $(CFLAGS) $(LEVELZERO_CFLAGS) -I$(INC_DIR) -I$(SRC_DIR) \
		tests/test_c198_phase12h_bitcoin_mining.c \
		$(OBJECTS) \
		$(LEVELZERO_LIBS) $(LIBS) -o $(BIN_PHASE12H)

# C198 Phase 12J : Test SHA256 Bitcoin Réel
BIN_PHASE12J = bin/c198_phase12j_sha256_real

c198_phase12j_sha256_real: dirs $(OBJECTS)
	$(CC) $(CFLAGS) $(LEVELZERO_CFLAGS) -I$(INC_DIR) -I$(SRC_DIR) \
		tests/test_c198_phase12j_sha256_real.c \
		$(OBJECTS) \
		$(LEVELZERO_LIBS) $(LIBS) -o $(BIN_PHASE12J)


# C198 Phase 13B : Test SHA256 OpenCL Validé
BIN_PHASE13B = bin/test_c198_phase13b_sha256_real

c198_phase13b_sha256_real: dirs $(OBJECTS)
	$(CC) $(CFLAGS) $(LEVELZERO_CFLAGS) -I$(INC_DIR) -I$(SRC_DIR) \
		tests/test_c198_phase13b_sha256_real.c \
		$(OBJECTS) \
		$(LEVELZERO_LIBS) $(LIBS) -o $(BIN_PHASE13B)
	@echo "[C198-13B] ✅ Binaire créé: $(BIN_PHASE13B)"
