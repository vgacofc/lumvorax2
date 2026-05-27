# Makefile Tests Unitaires LumVorax
# Conformité : STANDARD_NAMES.md v4.2
# Compilation : -Wall -Wextra -Werror -std=c11

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -O2 -g
CFLAGS += -D_POSIX_C_SOURCE=200809L
CFLAGS += -march=native -msse4.2

# Directories
SRC_DIR = ../src
TEST_DIR = .
BUILD_DIR = build
BIN_DIR = bin

# Include paths
INCLUDES = -I$(SRC_DIR)/execution_intelligence
INCLUDES += -I$(SRC_DIR)/distributed_mining
INCLUDES += -I$(SRC_DIR)/lum
INCLUDES += -I$(SRC_DIR)/common
INCLUDES += -I$(SRC_DIR)/debug
INCLUDES += -I$(TEST_DIR)

# Libraries
LIBS = -lpthread -lm -lrt

# Test framework
TEST_FRAMEWORK_SRC = $(TEST_DIR)/test_framework.c
TEST_FRAMEWORK_OBJ = $(BUILD_DIR)/test_framework.o

# Test sources
TEST_SOURCES = $(wildcard $(TEST_DIR)/test_*.c)
TEST_BINARIES = $(patsubst $(TEST_DIR)/test_%.c,$(BIN_DIR)/test_%,$(TEST_SOURCES))

# Module sources (à compiler avec tests)
MODULE_SOURCES = $(SRC_DIR)/execution_intelligence/nx_trace.c
MODULE_SOURCES += $(SRC_DIR)/distributed_mining/lum_load_balancer.c
MODULE_SOURCES += $(SRC_DIR)/distributed_mining/contribution_tracker.c
MODULE_SOURCES += $(SRC_DIR)/distributed_mining/lum_mining_agent.c
MODULE_SOURCES += $(SRC_DIR)/distributed_mining/proof_of_computation.c

MODULE_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(MODULE_SOURCES))

# Colors
COLOR_RESET = \033[0m
COLOR_GREEN = \033[32m
COLOR_YELLOW = \033[33m
COLOR_BLUE = \033[34m
COLOR_CYAN = \033[36m

.PHONY: all clean test coverage help

all: $(BIN_DIR) $(BUILD_DIR) $(TEST_BINARIES)
	@echo "$(COLOR_GREEN)✓ All tests compiled successfully$(COLOR_RESET)"

# Create directories
$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/execution_intelligence
	@mkdir -p $(BUILD_DIR)/distributed_mining

# Compile test framework
$(TEST_FRAMEWORK_OBJ): $(TEST_FRAMEWORK_SRC)
	@echo "$(COLOR_BLUE)[CC]$(COLOR_RESET) $<"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile module sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "$(COLOR_BLUE)[CC]$(COLOR_RESET) $<"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Link test binaries
$(BIN_DIR)/test_%: $(TEST_DIR)/test_%.c $(TEST_FRAMEWORK_OBJ) $(MODULE_OBJECTS)
	@echo "$(COLOR_CYAN)[LD]$(COLOR_RESET) $@"
	@$(CC) $(CFLAGS) $(INCLUDES) $< $(TEST_FRAMEWORK_OBJ) $(MODULE_OBJECTS) $(LIBS) -o $@

# Run all tests
test: all
	@echo ""
	@echo "$(COLOR_CYAN)╔════════════════════════════════════════════════════════════╗$(COLOR_RESET)"
	@echo "$(COLOR_CYAN)║$(COLOR_RESET)  $(COLOR_YELLOW)Running All Tests$(COLOR_RESET)                                      $(COLOR_CYAN)║$(COLOR_RESET)"
	@echo "$(COLOR_CYAN)╚════════════════════════════════════════════════════════════╝$(COLOR_RESET)"
	@echo ""
	@for test in $(TEST_BINARIES); do \
		echo "$(COLOR_BLUE)Running:$(COLOR_RESET) $$test"; \
		$$test || exit 1; \
		echo ""; \
	done
	@echo "$(COLOR_GREEN)╔════════════════════════════════════════════════════════════╗$(COLOR_RESET)"
	@echo "$(COLOR_GREEN)║$(COLOR_RESET)  $(COLOR_GREEN)✓ ALL TESTS PASSED$(COLOR_RESET)                                     $(COLOR_GREEN)║$(COLOR_RESET)"
	@echo "$(COLOR_GREEN)╚════════════════════════════════════════════════════════════╝$(COLOR_RESET)"
	@echo ""

# Run specific test
test_%: $(BIN_DIR)/test_%
	@echo ""
	@echo "$(COLOR_CYAN)Running test: $<$(COLOR_RESET)"
	@$<
	@echo ""

# Coverage analysis (requires gcov)
coverage: CFLAGS += -fprofile-arcs -ftest-coverage
coverage: clean all test
	@echo "$(COLOR_YELLOW)Generating coverage report...$(COLOR_RESET)"
	@gcov $(MODULE_SOURCES)
	@echo "$(COLOR_GREEN)✓ Coverage report generated$(COLOR_RESET)"

# Clean build artifacts
clean:
	@echo "$(COLOR_YELLOW)Cleaning build artifacts...$(COLOR_RESET)"
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@rm -f *.gcov *.gcda *.gcno
	@echo "$(COLOR_GREEN)✓ Clean complete$(COLOR_RESET)"

# Help
help:
	@echo "LumVorax Test Framework - Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Compile all tests"
	@echo "  test      - Run all tests"
	@echo "  test_X    - Run specific test X"
	@echo "  coverage  - Generate coverage report"
	@echo "  clean     - Clean build artifacts"
	@echo "  help      - Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make test              # Run all tests"
	@echo "  make test_nx_trace     # Run NX-Trace tests only"
	@echo "  make coverage          # Generate coverage report"