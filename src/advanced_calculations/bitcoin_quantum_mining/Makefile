# LumVorax — Module 17 — Bitcoin Quantum Mining Engine
# Makefile — Cycle C62 — STANDARD_NAMES.md v4.1
#
# Cibles :
#   make all          → compile btc_mining_runner (binaire principal)
#   make clean        → supprime les binaires
#   make test         → lance le gate BTC_SHA256_INTEGRITY_GATE
#   make benchmark    → run 60s benchmark mode
#
# Conformité STANDARD_NAMES.md §A :
#   FORENSIC_LOG_MODULE_METRIC = NOM D'ORIGINE — NE PAS RENOMMER

CC      ?= gcc
CFLAGS  ?= -O3 -std=c11 -Wall -Wextra \
	-D_POSIX_C_SOURCE=200809L \
	-DLUMVORAX_ENABLED=1 \
	-march=native \
	-ftree-vectorize \
	-fopenmp

# AVX2 pour SHA-256 8-way (si CPU le supporte — détection auto make)
HAVE_AVX2 := $(shell echo | gcc -mavx2 -dM -E - 2>/dev/null | grep -c '__AVX2__')
ifeq ($(HAVE_AVX2),1)
CFLAGS += -mavx2 -DBTC_HAVE_AVX2=1
endif

# SHA-NI hardware (AMD EPYC 9B14 Replit + Intel Kaby Lake Ubuntu)
HAVE_SHA_NI := $(shell grep -m1 sha_ni /proc/cpuinfo 2>/dev/null | wc -l)
ifeq ($(HAVE_SHA_NI),1)
CFLAGS += -msha -DBTC_HAVE_SHA_NI=1
endif
# Supprimer les warnings OpenSSL 3.x API dépréciée (EC_KEY → EVP migration)
CFLAGS += -Wno-deprecated-declarations

LDFLAGS = -Wl,-Bdynamic,--as-needed
# OpenSSL — chemin déterminé dynamiquement via pkg-config (compatibilité Nix/standard)
OPENSSL_LIBS := $(shell pkg-config --libs openssl 2>/dev/null || echo "-lcrypto -lssl")
LDLIBS  = -lm -lpthread -lgomp $(OPENSSL_LIBS)

# Racine LumVorax :
#   bitcoin_quantum_mining/ -> advanced_calculations/ -> src/
#   => ../../ depuis bitcoin_quantum_mining/ = workspace/src/
LUM_ROOT := ../../

# Répertoire include lumvorax_integration.h (HTS)
HTS_ROOT := $(LUM_ROOT)advanced_calculations/quantum_problem_hubbard_hts/

# Sources forensic LumVorax partagées (ultra_forensic_logger + memory_tracker)
LUM_SRC  := $(LUM_ROOT)debug/ultra_forensic_logger.c \
	$(LUM_ROOT)debug/memory_tracker.c

# Chemin include lumvorax_integration.c si nécessaire
LUM_INTG_SRC := $(HTS_ROOT)lumvorax/lumvorax_integration.c

# Sources Module 17
BTC_SRC  := src/sha256_lumvorax.c \
	src/nx48_btc_controller.c \
	src/btc_mining_engine.c \
	src/btc_wallet.c \
	src/btc_block_validator.c \
	src/main_btc_mining.c

# Binaire principal
BIN := btc_mining_runner

# Include paths :
#   -I$(LUM_ROOT)                    → workspace/src/  → trouve debug/ultra_forensic_logger.h
#   -I$(HTS_ROOT)include             → workspace/src/advanced_calculations/quantum_problem_hubbard_hts/include/
#                                      → trouve lumvorax_integration.h
INCLUDES := -I$(LUM_ROOT) \
	-I$(HTS_ROOT)include \
	-Iinclude \
	-Isrc

all: $(BIN)

$(BIN): $(BTC_SRC) $(LUM_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) \
	-o $@ $(BTC_SRC) $(LUM_SRC) $(LDLIBS)
	@echo "[MODULE 17] Compilation OK → $(BIN)"
	@ls -lh $(BIN)

# Gate : test intégrité SHA-256 (vecteur NIST)
test: $(BIN)
	@echo "[BTC_GATE] BTC_SHA256_INTEGRITY_GATE..."
	./$(BIN) --mode BENCHMARK --duration-s 1 --threads 1
	@echo "[BTC_GATE] Test terminé"

# Benchmark 60 secondes
benchmark: $(BIN)
	@echo "[BTC_QM] Benchmark 60s — 16 threads"
	./$(BIN) --mode BENCHMARK --duration-s 60 --threads 16

clean:
	rm -f $(BIN)
	@echo "[MODULE 17] Nettoyage OK"

.PHONY: all clean test benchmark
