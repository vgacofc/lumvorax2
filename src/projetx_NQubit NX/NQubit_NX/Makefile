CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror -pedantic
LDFLAGS = -lm

SRC = main.c nqbit_nx.c nx_forensic.c
OBJ = $(SRC:.c=.o)
TEST_SRC = tests/test_nqbit_nx.c nqbit_nx.c

all: nqbit_nx_runner

nqbit_nx_runner: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

test_nqbit_nx: $(TEST_SRC)
	$(CC) $(CFLAGS) -o $@ $(TEST_SRC) $(LDFLAGS)

run: nqbit_nx_runner
	./nqbit_nx_runner

test: test_nqbit_nx
	./test_nqbit_nx

clean:
	rm -f *.o nqbit_nx_runner test_nqbit_nx

.PHONY: all run test clean
