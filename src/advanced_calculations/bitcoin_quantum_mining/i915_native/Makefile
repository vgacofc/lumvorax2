# LumVorax C198 Phase 15U — Makefile i915 Native (Répertoire Séparé)
# 0% OpenCL, 0% Level Zero, 100% i915 DRM Direct
# Module: i915_native (indépendant de level_zero_native)

CC = gcc
CFLAGS = -Wall -Wextra -O3 -march=native -std=gnu11 -D_GNU_SOURCE
INCLUDES = -Iinclude -I/usr/include/libdrm
LIBS = -ldrm -lpthread -lm

# Répertoires
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
TEST_DIR = tests
LOG_DIR = logs/forensic

# Fichiers
SRC = $(SRC_DIR)/btc_gen9_native_runner.c
SRC_TRACKER = $(SRC_DIR)/btc_bit_level_tracker.c
SRC_MEMTRACK = $(SRC_DIR)/btc_memory_tracker_gpu.c
OBJ = $(BUILD_DIR)/btc_gen9_native_runner.o
OBJ_TRACKER = $(BUILD_DIR)/btc_bit_level_tracker.o
OBJ_MEMTRACK = $(BUILD_DIR)/btc_memory_tracker_gpu.o
TEST_SRC = $(TEST_DIR)/test_gen9_native.c
TEST_BIN = $(BIN_DIR)/test_gen9_native
TEST_BTC_SRC = $(TEST_DIR)/test_btc_mining_native.c
TEST_BTC_BIN = $(BIN_DIR)/test_btc_mining_native
TEST_TRACKER_SRC = $(TEST_DIR)/test_bit_level_tracker.c
TEST_TRACKER_BIN = $(BIN_DIR)/test_bit_level_tracker
TEST_MEMTRACK_SRC = $(TEST_DIR)/test_memory_tracker.c
TEST_MEMTRACK_BIN = $(BIN_DIR)/test_memory_tracker

# Cibles
.PHONY: all clean test test_btc test_tracker test_memtrack dirs

all: dirs $(TEST_BIN) $(TEST_TRACKER_BIN) $(TEST_MEMTRACK_BIN)

dirs:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR) $(LOG_DIR)

$(OBJ): $(SRC) | dirs
	@echo "[C198] Compilation: $<"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_TRACKER): $(SRC_TRACKER) | dirs
	@echo "[C198] Compilation: $<"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_MEMTRACK): $(SRC_MEMTRACK) | dirs
	@echo "[C198] Compilation: $<"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(TEST_BIN): $(OBJ) $(OBJ_TRACKER) $(TEST_SRC)
	@echo "[C198] Linkage: $@"
	$(CC) $(CFLAGS) $(INCLUDES) $(TEST_SRC) $(OBJ) $(OBJ_TRACKER) $(LIBS) -o $@
	@echo "[C198] ✅ Binaire créé: $@"

$(TEST_BTC_BIN): $(OBJ) $(OBJ_TRACKER) $(TEST_BTC_SRC)
	@echo "[C198] Linkage: $@"
	$(CC) $(CFLAGS) $(INCLUDES) $(TEST_BTC_SRC) $(OBJ) $(OBJ_TRACKER) $(LIBS) -o $@
	@echo "[C198] ✅ Binaire créé: $@"

$(TEST_TRACKER_BIN): $(OBJ_TRACKER) $(TEST_TRACKER_SRC) | dirs
	@echo "[C198] Linkage: $@"
	$(CC) $(CFLAGS) $(INCLUDES) $(TEST_TRACKER_SRC) $(OBJ_TRACKER) $(LIBS) -o $@
	@echo "[C198] ✅ Binaire créé: $@"

$(TEST_MEMTRACK_BIN): $(OBJ_MEMTRACK) $(TEST_MEMTRACK_SRC) | dirs
	@echo "[C198] Linkage: $@"
	$(CC) $(CFLAGS) $(INCLUDES) $(TEST_MEMTRACK_SRC) $(OBJ_MEMTRACK) $(LIBS) -o $@
	@echo "[C198] ✅ Binaire créé: $@"

test: $(TEST_BIN)
	@echo ""
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  TEST GEN9 NATIVE — 0% OpenCL, 0% Level Zero"
	@echo "═══════════════════════════════════════════════════════════"
	@echo ""
	@$(TEST_BIN)

test_tracker: $(TEST_TRACKER_BIN)
	@echo ""
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  TEST BIT-LEVEL TRACKER — Format .lum"
	@echo "═══════════════════════════════════════════════════════════"
	@echo ""
	@$(TEST_TRACKER_BIN)

test_memtrack: $(TEST_MEMTRACK_BIN)
	@echo ""
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  TEST MEMORY TRACKER RAM+GPU — Phase 15W"
	@echo "═══════════════════════════════════════════════════════════"
	@echo ""
	@$(TEST_MEMTRACK_BIN)

test_btc: $(TEST_BTC_BIN)
	@echo ""
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  TEST BITCOIN MINING SHA256 — Kernel 42KB"
	@echo "  Test Stabilité : 20 Dispatches Consécutifs"
	@echo "═══════════════════════════════════════════════════════════"
	@echo ""
	@$(TEST_BTC_BIN)

clean:
	@echo "[C198] Nettoyage..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "[C198] ✅ Nettoyage terminé"

# Aide
help:
	@echo "LumVorax C198 Phase 15C — Gen9 Native Runner"
	@echo ""
	@echo "Cibles disponibles:"
	@echo "  make all           - Compiler tout"
	@echo "  make test          - Tester Gen9 native"
	@echo "  make test_btc      - Tester Bitcoin mining"
	@echo "  make test_tracker  - Tester bit-level tracker"
	@echo "  make test_memtrack - Tester memory tracker RAM+GPU"
	@echo "  make clean         - Nettoyer"
	@echo "  make help          - Afficher cette aide"
	@echo ""
	@echo "Dépendances:"
	@echo "  - libdrm-dev (apt install libdrm-dev)"
	@echo "  - Kernel Gen9 ISA (kernels/btc_sha256_mining_gen9.bin)"
	@echo "  - Permissions GPU (/dev/dri/renderD128)"
	@echo ""
	@echo "Répertoire: i915_native/ (séparé de level_zero_native/)"