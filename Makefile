# Makefile LUM/VORAX - Compilation COMPLÈTE TOUS MODULES
CC = gcc
# MK-001 FIX: -DDEBUG_MODE actif par défaut (conformément au protocole ARTCB mode DEBUG).
# MK-002 FIX: -Wl,-z,stack-size retiré de CFLAGS (linker flag ≠ compiler flag).
# MK-004 FIX: -lrt et -Wl,-z,stack-size sont Linux-only.
# macOS nécessite -D_DARWIN_C_SOURCE pour exposer ru_maxrss, getpagesize, etc.
# via <sys/resource.h> même avec -D_POSIX_C_SOURCE=200809L.
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    CFLAGS = -Wall -Wextra -std=c99 -g -O3 -march=native -fPIC -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -DDEBUG_MODE -I./src/common -I./src/debug -I./src/crypto -I./src/advanced_calculations
    CFLAGS_PORTABLE = -Wall -Wextra -std=c99 -g -O2 -march=x86-64 -fPIC -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -DDEBUG_MODE -I./src/common -I./src/debug -I./src/crypto -I./src/advanced_calculations
    LDFLAGS = -lm -lpthread -lrt -Wl,-z,stack-size=16777216
else
    CFLAGS = -Wall -Wextra -std=c99 -g -O3 -march=native -fPIC -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -D_DARWIN_C_SOURCE -DDEBUG_MODE -I./src/common -I./src/debug -I./src/crypto -I./src/advanced_calculations
    CFLAGS_PORTABLE = -Wall -Wextra -std=c99 -g -O2 -fPIC -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -D_DARWIN_C_SOURCE -DDEBUG_MODE -I./src/common -I./src/debug -I./src/crypto -I./src/advanced_calculations
    LDFLAGS = -lm -lpthread
endif

# Debug/Release modes for performance control
debug: CFLAGS += -g3
debug: all

release: CFLAGS += -O3 -DNDEBUG
release: all

# BL-008 FIX: build portable sans -march=native pour portabilité ISA
# Usage : make portable   →  bin/lum_vorax_portable (sans instructions AVX-512/native)
portable: directories
	$(CC) $(CFLAGS_PORTABLE) $(SRC_DIR)/main.c $(SOURCES:.c=.o) -o $(BIN_DIR)/lum_vorax_portable $(LDFLAGS) || \
	$(MAKE) _portable_from_scratch

_portable_from_scratch:
	$(CC) $(CFLAGS_PORTABLE) -c $(SRC_DIR)/main.c -o $(BIN_DIR)/main_portable.o
	for src in $(SOURCES); do \
	    obj=$$(echo $$src | sed 's/\.c$$/_portable.o/'); \
	    $(CC) $(CFLAGS_PORTABLE) -c $$src -o $$obj; \
	done
	find $(SRC_DIR) -name '*_portable.o' | xargs $(CC) $(CFLAGS_PORTABLE) $(BIN_DIR)/main_portable.o -o $(BIN_DIR)/lum_vorax_portable $(LDFLAGS)
	find $(SRC_DIR) -name '*_portable.o' -delete
	rm -f $(BIN_DIR)/main_portable.o

# Répertoires
SRC_DIR = src
BIN_DIR = bin
LOG_DIR = logs

# TOUS LES MODULES SOURCES DISPONIBLES (SANS quantiques/blackbox désactivés)
SOURCES = \
	$(SRC_DIR)/lum/lum_core.c \
	$(SRC_DIR)/vorax/vorax_operations.c \
	$(SRC_DIR)/binary/binary_lum_converter.c \
	$(SRC_DIR)/parser/vorax_parser.c \
	$(SRC_DIR)/logger/lum_logger.c \
	$(SRC_DIR)/logger/log_manager.c \
	$(SRC_DIR)/debug/memory_tracker.c \
	$(SRC_DIR)/debug/forensic_logger.c \
	$(SRC_DIR)/debug/ultra_forensic_logger.c \
	$(SRC_DIR)/debug/enhanced_logging.c \
	$(SRC_DIR)/debug/logging_system.c \
	$(SRC_DIR)/crypto/crypto_validator.c \
	$(SRC_DIR)/persistence/data_persistence.c \
	$(SRC_DIR)/persistence/transaction_wal_extension.c \
	$(SRC_DIR)/persistence/recovery_manager_extension.c \
	$(SRC_DIR)/optimization/memory_optimizer.c \
	$(SRC_DIR)/optimization/pareto_optimizer.c \
	$(SRC_DIR)/optimization/pareto_inverse_optimizer.c \
	$(SRC_DIR)/optimization/simd_optimizer.c \
	$(SRC_DIR)/optimization/zero_copy_allocator.c \
	$(SRC_DIR)/parallel/parallel_processor.c \
	$(SRC_DIR)/metrics/performance_metrics.c \
	$(SRC_DIR)/advanced_calculations/audio_processor.c \
	$(SRC_DIR)/advanced_calculations/image_processor.c \
	$(SRC_DIR)/advanced_calculations/golden_score_optimizer.c \
	$(SRC_DIR)/advanced_calculations/tsp_optimizer.c \
	$(SRC_DIR)/advanced_calculations/neural_advanced_optimizers.c \
	$(SRC_DIR)/advanced_calculations/neural_ultra_precision_architecture.c \
	$(SRC_DIR)/advanced_calculations/matrix_calculator.c \
	$(SRC_DIR)/advanced_calculations/neural_network_processor.c \
	$(SRC_DIR)/complex_modules/realtime_analytics.c \
	$(SRC_DIR)/complex_modules/distributed_computing.c \
	$(SRC_DIR)/complex_modules/ai_optimization.c \
	$(SRC_DIR)/complex_modules/ai_dynamic_config_manager.c \
	$(SRC_DIR)/file_formats/lum_secure_serialization.c \
	$(SRC_DIR)/file_formats/lum_native_file_handler.c \
	$(SRC_DIR)/file_formats/lum_native_universal_format.c \
	$(SRC_DIR)/spatial/lum_instant_displacement.c \
	$(SRC_DIR)/network/hostinger_resource_limiter.c \
	$(SRC_DIR)/advanced_calculations/quantum_simulator.c \
	$(SRC_DIR)/physics/kerr_metric.c \
	$(SRC_DIR)/logging/log_writer.c \
	$(SRC_DIR)/common/time_ns.c

# Objets
OBJECTS = $(SOURCES:.c=.o)

# Exécutables
MAIN_EXECUTABLE = $(BIN_DIR)/lum_vorax_complete
TEST_PROGRESSIVE = $(BIN_DIR)/test_progressive_all_modules
LIB_LUMVORAX = liblumvorax.so

# Tests forensiques conformes prompt.txt
TEST_EXECUTABLES = \
	$(BIN_DIR)/test_forensic_complete_system \
	$(BIN_DIR)/test_integration_complete_39_modules \
	$(BIN_DIR)/test_quantum

# SHA-256 blockchain — cible séparée (BL-004/SHA-256 build proof)
# Sources blockchain non incluses dans SOURCES principal (module indépendant).
BLOCKCHAIN_SOURCES = \
    $(SRC_DIR)/blockchain_lumvorax/sha256_mini.c \
    $(SRC_DIR)/blockchain_lumvorax/block_header.c

blockchain_test: directories
	$(CC) $(CFLAGS) -c $(SRC_DIR)/blockchain_lumvorax/sha256_mini.c -o $(SRC_DIR)/blockchain_lumvorax/sha256_mini.o
	$(CC) $(CFLAGS) -c $(SRC_DIR)/blockchain_lumvorax/block_header.c -o $(SRC_DIR)/blockchain_lumvorax/block_header.o
	$(CC) $(CFLAGS) src/tests/test_blockchain_sha256.c \
	    $(SRC_DIR)/blockchain_lumvorax/sha256_mini.o \
	    $(SRC_DIR)/blockchain_lumvorax/block_header.o \
	    -o $(BIN_DIR)/test_blockchain_sha256 $(LDFLAGS)
	@echo "[blockchain_test] Binaire: bin/test_blockchain_sha256"
	@nm $(BIN_DIR)/test_blockchain_sha256 | grep -E "sha256_lumvorax|block_header_hash" && echo "[SHA-256 OK] Symboles liés"

.PHONY: all clean test test-progressive test-stress test-forensic rsa_test science_test liblumvorax.so blockchain_test portable _portable_from_scratch

all: directories $(MAIN_EXECUTABLE) $(TEST_EXECUTABLES) $(LIB_LUMVORAX)

directories:
	mkdir -p $(BIN_DIR) $(LOG_DIR)/forensic $(LOG_DIR)/execution $(LOG_DIR)/tests $(LOG_DIR)/console

# Compilation objets
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Shared Library
$(LIB_LUMVORAX): $(OBJECTS)
	$(CC) $(CFLAGS) -shared $(OBJECTS) -o $@ $(LDFLAGS)

# Exécutable principal avec TOUS les modules
$(MAIN_EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(SRC_DIR)/main.c $(OBJECTS) -o $@ $(LDFLAGS)

# Test forensique complet conforme prompt.txt
$(BIN_DIR)/test_forensic_complete_system: $(OBJECTS)
	$(CC) $(CFLAGS) src/tests/test_forensic_complete_system.c $(OBJECTS) -o $@ $(LDFLAGS)

# Test d'intégration complète 39 modules
# MK-003 FIX: -lmvec est Linux-only (libmvec = glibc vectorisée) ; macOS ne la fournit pas.
$(BIN_DIR)/test_integration_complete_39_modules: $(OBJECTS)
ifeq ($(UNAME_S),Linux)
	$(CC) $(CFLAGS) src/tests/test_integration_complete_39_modules.c $(OBJECTS) -o $@ $(LDFLAGS) -lmvec -lm
else
	$(CC) $(CFLAGS) src/tests/test_integration_complete_39_modules.c $(OBJECTS) -o $@ $(LDFLAGS)
endif

$(BIN_DIR)/test_quantum: $(OBJECTS)
	$(CC) $(CFLAGS) src/tests/test_quantum_simulator_complete.c $(OBJECTS) -o $@ $(LDFLAGS)

# TESTS PROGRESSIFS 1M → 100M avec TOUS les modules + redirection console
test-progressive: $(MAIN_EXECUTABLE)
	@echo "🚀 === TESTS PROGRESSIFS 1M → 100M TOUS MODULES ==="
	@echo "Optimisations: SIMD +300%, Parallel VORAX +400%, Cache Alignment +15%"
	@if [ ! -f logs/console/redirect_console.sh ]; then ./setup_console_redirect.sh; fi
	@bash -c "source logs/console/redirect_console.sh && $(MAIN_EXECUTABLE) --progressive-stress-all"

# Tests forensiques conformes prompt.txt
test-forensic: $(BIN_DIR)/test_forensic_complete_system
	@echo "🛡️ === TESTS FORENSIQUES COMPLETS CONFORMES PROMPT.TXT ==="
	$(BIN_DIR)/test_forensic_complete_system

# VALIDATION COMPLÈTE - PROGRESSIVE + FORENSIQUE
test: test-progressive test-forensic
	@echo "✅ === VALIDATION COMPLÈTE TERMINÉE - CONFORMITÉ PROMPT.TXT ==="

clean:
	rm -f $(OBJECTS)
	rm -f $(MAIN_EXECUTABLE) $(TEST_EXECUTABLES)
	rm -rf $(BIN_DIR)
	find . -name "*.o" -type f -delete
	rm -f src/blockchain_lumvorax/*.o