CC ?= gcc
# C33 — 0 warning, 0 erreur — Conformite prompt.txt Section 3.2
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200809L \
          -DMODULES_QUANTIQUES_ACTIFS=1 -DLUMVORAX_ENABLED=1
LDFLAGS  = -Wl,-Bdynamic,--as-needed
LDLIBS  ?= -lm -lpthread

# Chemins — LUM_ROOT pointe vers src/advanced_calculations/
LUM_ROOT := ../
LUM_INC  := -I$(LUM_ROOT) -I.

# Sources du logger V4 NEXT (STANDARD_NAMES Section I)
QF_LOGGER_SRC := quantum_forensic_logger.c

# memory_tracker partage entre V3 et V4 NEXT
LUM_MEMORY_SRC := $(LUM_ROOT)debug/memory_tracker.c

# Sources V4 NEXT
SRC_QUANTUM_LIB := quantum_simulator.c
SRC_QUANTUM_MAIN := quantum_v4next_main.c
SRC_FUSION_V3    := quantum_simulator_fusion_v3.c
SRC_FUSION_CLI   := tools/fusion_cli_v3.c

# Binaires
BIN_QUANTUM   := quantum_v4next_runner
BIN_FUSION_V3 := quantum_fusion_v3_cli

ALL_SRCS_QF := $(SRC_QUANTUM_LIB) $(SRC_QUANTUM_MAIN) $(QF_LOGGER_SRC) $(LUM_MEMORY_SRC)
ALL_SRCS_FU := $(SRC_FUSION_V3) $(SRC_FUSION_CLI) $(QF_LOGGER_SRC) $(LUM_MEMORY_SRC)

.PHONY: all clean

all: $(BIN_QUANTUM) $(BIN_FUSION_V3)

$(BIN_QUANTUM): $(ALL_SRCS_QF)
	$(CC) $(CFLAGS) $(LUM_INC) $(LDFLAGS) -o $@ $(ALL_SRCS_QF) $(LDLIBS)

$(BIN_FUSION_V3): $(ALL_SRCS_FU)
	$(CC) $(CFLAGS) $(LUM_INC) $(LDFLAGS) -o $@ $(ALL_SRCS_FU) $(LDLIBS)

clean:
	rm -f $(BIN_QUANTUM) $(BIN_FUSION_V3)
