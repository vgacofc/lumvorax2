# LumVorax — Module 18 — QDAYPRIZE Quantum Attack Simulator
# Makefile — Compilation du simulateur d'attaque quantique
#
# STANDARD_NAMES.md v4.3 §M-QDPR18-C55 — Cycle C55 — 2026-04-17
#
# Usage :
#   make              # Compilation optimisée
#   make debug        # Compilation debug + ASAN
#   make clean        # Nettoyage
#   make run          # Test rapide (sim_bits=12, 30s)

CC      = gcc
BINARY  = qdayprize_runner

# Flags de compilation — identiques au BTC mining (-march=native safe sur i5-8265U)
CFLAGS  = -std=c11 -O3 -march=native -ffast-math -funroll-loops \
          -Wall -Wextra -Wno-unused-parameter \
          -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L

# Flags debug
DFLAGS  = -std=c11 -O0 -g3 -fsanitize=address,undefined \
          -Wall -Wextra -D_GNU_SOURCE

LDFLAGS = -lm -lpthread

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
