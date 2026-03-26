CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror -pedantic
LDFLAGS = -lm

SRC = main.c nqbit_nx.c nx_forensic.c
OBJ = $(SRC:.c=.o)
TEST_SRC = tests/test_nqbit_nx.c nqbit_nx.c
TEST_EXT_SRC = tests/test_nqbit_nx_extended.c nqbit_nx.c
TEST_PHASE_SRC = tests/test_nqbit_phase_p0_p1.c

all: nqbit_nx_runner

nqbit_nx_runner: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

test_nqbit_nx: $(TEST_SRC)
	$(CC) $(CFLAGS) -o $@ $(TEST_SRC) $(LDFLAGS)

test_nqbit_nx_extended: $(TEST_EXT_SRC)
	$(CC) $(CFLAGS) -o $@ $(TEST_EXT_SRC) $(LDFLAGS)

test_nqbit_phase_p0_p1: $(TEST_PHASE_SRC)
	$(CC) $(CFLAGS) -o $@ $(TEST_PHASE_SRC) $(LDFLAGS)

run: nqbit_nx_runner
	./nqbit_nx_runner

test: test_nqbit_nx
	./test_nqbit_nx

test_extended: test_nqbit_nx_extended
	./test_nqbit_nx_extended

test_phase: test_nqbit_phase_p0_p1
	./test_nqbit_phase_p0_p1

test_all: test test_extended test_phase

clean:
	rm -f *.o nqbit_nx_runner test_nqbit_nx test_nqbit_nx_extended test_nqbit_phase_p0_p1

.PHONY: all run test test_extended test_phase test_all clean
