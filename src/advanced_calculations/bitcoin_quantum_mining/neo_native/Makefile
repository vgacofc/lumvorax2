# Makefile - Projet A : NEO Wrapper
# Phase 15Y-F26

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -I../src
LDFLAGS = -lOpenCL -lpthread -lm

SRCS = btc_neo_wrapper.c test_neo_wrapper.c test_massive_benchmark.c
OBJS_TEST = btc_neo_wrapper.o test_neo_wrapper.o
OBJS_BENCH = btc_neo_wrapper.o test_massive_benchmark.o
TARGET_TEST = test_neo_wrapper
TARGET_BENCH = test_massive_benchmark

.PHONY: all clean test benchmark

all: $(TARGET_TEST) $(TARGET_BENCH)

$(TARGET_TEST): $(OBJS_TEST)
	$(CC) $(OBJS_TEST) -o $(TARGET_TEST) $(LDFLAGS)

$(TARGET_BENCH): $(OBJS_BENCH)
	$(CC) $(OBJS_BENCH) -o $(TARGET_BENCH) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET_TEST)
	./$(TARGET_TEST)

benchmark: $(TARGET_BENCH)
	./$(TARGET_BENCH)

clean:
	rm -f *.o $(TARGET_TEST) $(TARGET_BENCH) *.log

.PHONY: help
help:
	@echo "Projet A - NEO Wrapper"
	@echo "======================"
	@echo "make         : Compiler"
	@echo "make test    : Compiler et tester"
	@echo "make clean   : Nettoyer"