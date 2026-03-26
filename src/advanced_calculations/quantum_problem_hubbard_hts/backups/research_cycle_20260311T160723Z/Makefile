CC ?= gcc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Wpedantic
LDFLAGS ?=
LDLIBS ?= -lm

BIN_MAIN := hubbard_hts_runner
BIN_RESEARCH := hubbard_hts_research_runner
BIN_RESEARCH_ADV := hubbard_hts_research_runner_advanced_parallel
SRC_MAIN := src/main.c src/hubbard_hts_module.c
SRC_RESEARCH := src/hubbard_hts_research_cycle.c
SRC_RESEARCH_ADV := src/hubbard_hts_research_cycle_advanced_parallel.c

all: $(BIN_MAIN) $(BIN_RESEARCH) $(BIN_RESEARCH_ADV)

$(BIN_MAIN): $(SRC_MAIN)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC_MAIN) $(LDLIBS)

$(BIN_RESEARCH): $(SRC_RESEARCH)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC_RESEARCH) $(LDLIBS)

$(BIN_RESEARCH_ADV): $(SRC_RESEARCH_ADV)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC_RESEARCH_ADV) $(LDLIBS)

clean:
	rm -f $(BIN_MAIN) $(BIN_RESEARCH) $(BIN_RESEARCH_ADV)

.PHONY: all clean
