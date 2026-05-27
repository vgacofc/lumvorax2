CC ?= gcc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Wpedantic
LDFLAGS ?= -lm

BIN_MAIN := hubbard_hts_runner
BIN_RESEARCH := hubbard_hts_research_runner
SRC_MAIN := src/main.c src/hubbard_hts_module.c
SRC_RESEARCH := src/hubbard_hts_research_cycle.c

all: $(BIN_MAIN) $(BIN_RESEARCH)

$(BIN_MAIN): $(SRC_MAIN)
	$(CC) $(CFLAGS) -o $@ $(SRC_MAIN) $(LDFLAGS)

$(BIN_RESEARCH): $(SRC_RESEARCH)
	$(CC) $(CFLAGS) -o $@ $(SRC_RESEARCH) $(LDFLAGS)

clean:
	rm -f $(BIN_MAIN) $(BIN_RESEARCH)

.PHONY: all clean
