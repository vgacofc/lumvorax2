# LumVorax — Module 18 — QDAYPRIZE Quantum Attack Simulator
# Makefile — Compilation du simulateur d'attaque quantique
#
# STANDARD_NAMES.md v4.3 — Cycle C59 (FIX SIGILL + TAB fix)
#
# Usage :
#   make              # Compilation optimisée
#   make debug        # Compilation debug + ASAN
#   make clean        # Nettoyage
#   make run          # Test rapide (sim_bits=12, 30s)

CC      = gcc
BINARY  = qdayprize_runner

# ─── CFLAGS cross-platform ────────────────────────────────────────
# -march=native : compile pour le CPU LOCAL (ne jamais commiter les .o !)
# -mno-avx512f etc : désactive AVX-512 (absent sur i5-8265U)
# -fPIE : requis Ubuntu 16.04+ (PIE par défaut)
CFLAGS  = -std=c11 -O3 -march=native \
          -mno-avx512f -mno-avx512vl -mno-avx512bw -mno-avx512dq -mno-avx512cd \
          -ffast-math -funroll-loops \
          -Wall -Wextra -Wno-unused-parameter \
          -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
          -fPIE

# Flags debug
DFLAGS  = -std=c11 -O0 -g3 -fsanitize=address,undefined \
          -Wall -Wextra -D_GNU_SOURCE

# -pie dans LDFLAGS → exécutable PIE Ubuntu
LDFLAGS = -lm -lpthread -pie

SRCS    = main_qdayprize.c qdayprize_engine.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all debug clean run

all: $(BINARY)
	@echo "[QDPR] Compilation OK → $(BINARY)"
	@ls -lh $(BINARY)

$(BINARY): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c qdayprize_engine.h
	$(CC) $(CFLAGS) -c -o $@ $<

debug:
	$(CC) $(DFLAGS) -o $(BINARY)_debug $(SRCS) $(LDFLAGS)
	@echo "[QDPR] Debug build → $(BINARY)_debug"

clean:
	rm -f $(BINARY) $(BINARY)_debug $(OBJS)
	@echo "[QDPR] Nettoyage OK"

run: $(BINARY)
	./$(BINARY) --sim-bits 12 --duration 30 --verbose
