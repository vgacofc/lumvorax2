# MDBAI — Makefile forensic library
# Compile libmdbai_forensic.so (LD_PRELOAD injection)
# Conforme STANDARD_NAMES_MDBAI.md Section 9
# Conforme prompt.txt Règle #1 : traçabilité bit-level LumVorax

CC       = gcc
CFLAGS   = -Wall -Wextra -O2 -std=c11 -fPIC -D_DEFAULT_SOURCE -D_GNU_SOURCE
LDFLAGS  = -shared -ldl -lpthread

LUM_SRC  = ../../../src/lum
DBG_SRC  = ../../../src/debug
CMN_SRC  = ../../../src/common

INCLUDES = -I$(LUM_SRC) -I$(DBG_SRC) -I$(CMN_SRC)

SRCS     = mdbai_forensic.c
TARGET   = libmdbai_forensic.so

.PHONY: all clean check

all: $(TARGET)
	@echo "[MDBAI] libmdbai_forensic.so compilée ✅"
	@ls -lh $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $^

check: $(TARGET)
	@echo "[MDBAI] Vérification symboles exportés:"
	@nm -D $(TARGET) | grep mdbai || echo "  (aucun symbole mdbai)"
	@echo "[MDBAI] Dépendances:"
	@ldd $(TARGET)

clean:
	rm -f $(TARGET) *.o *.lum *.log

install: $(TARGET)
	@echo "MDBAI_FORENSIC_LIB_PATH=$(shell pwd)/$(TARGET)"
	@echo "[MDBAI] Définir dans Replit Secrets: MDBAI_FORENSIC_LIB_PATH=$(shell pwd)/$(TARGET)"
