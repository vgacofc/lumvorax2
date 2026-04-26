# LumVorax — Module 17 — Bitcoin Quantum Mining Engine
# Makefile — Cycle C69 — STANDARD_NAMES.md v4.1
#
# Cibles :
#   make all          -> compile btc_mining_runner (binaire principal)
#   make clean        -> supprime les binaires
#   make test         -> lance le gate BTC_SHA256_INTEGRITY_GATE
#   make benchmark    -> run 60s benchmark mode
#
# Conformite STANDARD_NAMES.md par A :
#   FORENSIC_LOG_MODULE_METRIC = NOM D'ORIGINE - NE PAS RENOMMER

CC      ?= gcc
CFLAGS  ?= -O3 -std=c11 -Wall -Wextra \
        -D_POSIX_C_SOURCE=200809L \
        -DLUMVORAX_ENABLED=1 \
        -march=native \
        -ftree-vectorize \
        -fopenmp

# AVX2 pour SHA-256 8-way (si CPU le supporte - detection auto make)
HAVE_AVX2 := $(shell echo | gcc -mavx2 -dM -E - 2>/dev/null | grep -c '__AVX2__')
ifeq ($(HAVE_AVX2),1)
CFLAGS += -mavx2 -DBTC_HAVE_AVX2=1
endif

# SHA-NI hardware (AMD EPYC 9B14 Replit + Intel Kaby Lake Ubuntu)
HAVE_SHA_NI := $(shell grep -m1 sha_ni /proc/cpuinfo 2>/dev/null | wc -l)
ifeq ($(HAVE_SHA_NI),1)
CFLAGS += -msha -DBTC_HAVE_SHA_NI=1
endif
# Supprimer les warnings OpenSSL 3.x API deprecie (EC_KEY -> EVP migration)
CFLAGS += -Wno-deprecated-declarations

LDFLAGS = -Wl,-Bdynamic,--as-needed
# OpenSSL - chemin determine dynamiquement via pkg-config (compatibilite Nix/standard)
OPENSSL_LIBS := $(shell pkg-config --libs openssl 2>/dev/null || echo "-lcrypto -lssl")
LDLIBS  = -lm -lpthread -lgomp $(OPENSSL_LIBS)

# Racine LumVorax :
#   bitcoin_quantum_mining/ -> advanced_calculations/ -> src/
#   => ../../ depuis bitcoin_quantum_mining/ = workspace/src/
LUM_ROOT := ../../

# Repertoire include lumvorax_integration.h (HTS)
HTS_ROOT := $(LUM_ROOT)advanced_calculations/quantum_problem_hubbard_hts/

# Sources forensic LumVorax partagees (ultra_forensic_logger + memory_tracker)
LUM_SRC  := $(LUM_ROOT)debug/ultra_forensic_logger.c \
	$(LUM_ROOT)debug/memory_tracker.c \
	$(LUM_ROOT)common/lvx_log_rotate.c \
	$(LUM_ROOT)neural_network/nx48_neuro_coupler.c \
	$(LUM_ROOT)neural_network/izhikevich_spiking.c \
	$(LUM_ROOT)neural_network/stdp_plasticity.c

# Chemin include lumvorax_integration.c si necessaire
LUM_INTG_SRC := $(HTS_ROOT)lumvorax/lumvorax_integration.c


# ── OpenCL detection automatique (Intel UHD 620 Ubuntu) ─────────────
# C63 : btc_opencl_runner.c active si libOpenCL disponible
HAVE_OPENCL := $(shell pkg-config --exists OpenCL 2>/dev/null && echo 1 || \
	       ls /usr/lib/x86_64-linux-gnu/libOpenCL.so* 2>/dev/null | head -1 | wc -l | tr -d ' ')
ifeq ($(HAVE_OPENCL),1)
CFLAGS   += -DBTC_OPENCL_ENABLED=1
OCL_LIBS  := $(shell pkg-config --libs OpenCL 2>/dev/null || echo "-lOpenCL")
LDLIBS   += $(OCL_LIBS)
$(info [OCL] OpenCL detecte -> GPU SHA-256 active cible 50-200 MHs)
else
$(info [OCL] OpenCL non disponible -> CPU SHA-256 uniquement + stubs GPU)
endif

# C69-GPU-INTEGRATE : btc_opencl_runner.c TOUJOURS compile.
# Sans OpenCL : compile les stubs (btc_opencl_init retourne BTC_OCL_ERR_NO_DEVICE).
# Avec OpenCL  : compile le vrai code GPU (appel reel btc_opencl_mine_batch).
# AVANT C69 : OCL_SRC vide sans OpenCL -> btc_opencl_mine_batch undefined au link.
# APRES C69 : toujours lie -> stubs ou code reel selon HAVE_OPENCL.
OCL_SRC   := src/btc_opencl_runner.c

# Sources Module 17
BTC_SRC  := src/sha256_lumvorax.c \
	src/nx48_btc_controller.c \
	src/btc_mining_engine.c \
	src/btc_wallet.c \
	src/btc_block_validator.c \
	src/main_btc_mining.c \
	src/nx48_alltime_record.c \
	src/nx48_coupler_bridge.c \
	src/btc_testnet3_runtime.c \
	$(OCL_SRC)

# Binaire principal
BIN := btc_mining_runner

# Include paths :
#   -I$(LUM_ROOT)                    -> workspace/src/  -> trouve debug/ultra_forensic_logger.h
#   -I$(HTS_ROOT)include             -> workspace/src/advanced_calculations/quantum_problem_hubbard_hts/include/
#                                       -> trouve lumvorax_integration.h
INCLUDES := -I$(LUM_ROOT) \
	-I$(HTS_ROOT)include \
	-Iinclude \
	-Isrc

all: $(BIN)

$(BIN): $(BTC_SRC) $(LUM_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) \
	-o $@ $(BTC_SRC) $(LUM_SRC) $(LDLIBS)
	@echo "[MODULE 17] Compilation OK -> $(BIN)"
	@ls -lh $(BIN)

# Gate : test integrite SHA-256 (vecteur NIST)
test: $(BIN)
	@echo "[BTC_GATE] BTC_SHA256_INTEGRITY_GATE..."
	./$(BIN) --mode BENCHMARK --duration-s 1 --threads 1
	@echo "[BTC_GATE] Test termine"

# Benchmark 60 secondes
benchmark: $(BIN)
	@echo "[BTC_QM] Benchmark 60s - 16 threads"
	./$(BIN) --mode BENCHMARK --duration-s 60 --threads 16

clean:
	rm -f $(BIN)
	@echo "[MODULE 17] Nettoyage OK"

.PHONY: all clean test benchmark
