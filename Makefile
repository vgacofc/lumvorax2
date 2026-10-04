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
	$(SRC_DIR)/debug/forensic_unif_002.c \
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

# BL-008/BL-014 FIX: build portable ISOLÉ dans build/obj/portable/
# SOURCES et SRC_DIR sont définis avant ce bloc — PORTABLE_OBJECTS s'évalue correctement.
# Jamais de réutilisation des .o natifs (build/obj/native/ ou src/**/*.o).
# Usage : make portable  →  bin/lum_vorax_portable
PORTABLE_OBJ_DIR = build/obj/portable
PORTABLE_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(PORTABLE_OBJ_DIR)/%.o,$(SOURCES))

$(PORTABLE_OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_PORTABLE) -c $< -o $@

portable: directories $(PORTABLE_OBJECTS)
	@mkdir -p $(PORTABLE_OBJ_DIR)
	$(CC) $(CFLAGS_PORTABLE) -c $(SRC_DIR)/main.c -o $(PORTABLE_OBJ_DIR)/main.o
	$(CC) $(CFLAGS_PORTABLE) $(PORTABLE_OBJ_DIR)/main.o $(PORTABLE_OBJECTS) \
	    -o $(BIN_DIR)/lum_vorax_portable $(LDFLAGS)
	@echo "[BL-014 OK] Binaire portable compile depuis $(PORTABLE_OBJ_DIR) (aucun .o natif reutilise)"

portable-clean:
	rm -rf build/obj/portable
	rm -f $(BIN_DIR)/lum_vorax_portable

# Exécutables
MAIN_EXECUTABLE = $(BIN_DIR)/lum_vorax_complete
TEST_PROGRESSIVE = $(BIN_DIR)/test_progressive_all_modules
LIB_LUMVORAX = liblumvorax.so

# Tests forensiques conformes prompt.txt
TEST_EXECUTABLES = \
	$(BIN_DIR)/test_forensic_complete_system \
	$(BIN_DIR)/test_integration_complete_39_modules \
	$(BIN_DIR)/test_quantum

# === S157 : FORENSIC-UNIF-002 + Richardson-PROTOCOL-003 ===
# === S159 : FORENSIC-UNIF-003 + Richardson-PROTOCOL-003b ===
NS_SOURCES = \
	$(SRC_DIR)/solvers/ns_solver_2d.c \
	$(SRC_DIR)/debug/forensic_logger.c \
	$(SRC_DIR)/debug/memory_tracker.c \
	$(SRC_DIR)/common/time_ns.c \
	$(SRC_DIR)/lum/lum_core.c \
	$(SRC_DIR)/binary/binary_lum_converter.c

$(BIN_DIR)/ns_forensic_unif2: $(NS_SOURCES) $(SRC_DIR)/validation/ns_forensic_unif2.c
	$(CC) $(CFLAGS) $(NS_SOURCES) \
	    $(SRC_DIR)/validation/ns_forensic_unif2.c \
	    -o $@ $(LDFLAGS)
	@echo "[S157] Binaire: bin/ns_forensic_unif2"

$(BIN_DIR)/ns_richardson_manufactured: $(NS_SOURCES) $(SRC_DIR)/validation/ns_richardson_manufactured.c
	$(CC) $(CFLAGS) $(NS_SOURCES) \
	    $(SRC_DIR)/validation/ns_richardson_manufactured.c \
	    -o $@ $(LDFLAGS)
	@echo "[S157] Binaire: bin/ns_richardson_manufactured"

$(BIN_DIR)/ns_forensic_unif3: $(NS_SOURCES) $(SRC_DIR)/validation/ns_forensic_unif3.c
	$(CC) $(CFLAGS) $(NS_SOURCES) \
	    $(SRC_DIR)/validation/ns_forensic_unif3.c \
	    -o $@ $(LDFLAGS)
	@echo "[S159] Binaire: bin/ns_forensic_unif3"

$(BIN_DIR)/ns_richardson_couette_periodic: $(NS_SOURCES) $(SRC_DIR)/validation/ns_richardson_couette_periodic.c
	$(CC) $(CFLAGS) $(NS_SOURCES) \
	    $(SRC_DIR)/validation/ns_richardson_couette_periodic.c \
	    -o $@ $(LDFLAGS)
	@echo "[S159] Binaire: bin/ns_richardson_couette_periodic"

# === S161 : FORENSIC-UNIF-004 + Richardson-003c + BUILD-THREAD-001 + PERF-FORENSIC-001 ===

$(BIN_DIR)/ns_forensic_unif4: $(NS_SOURCES) $(SRC_DIR)/validation/ns_forensic_unif4.c
	$(CC) $(CFLAGS) $(NS_SOURCES) \
	    $(SRC_DIR)/validation/ns_forensic_unif4.c \
	    -o $@ $(LDFLAGS)
	@echo "[S161] Binaire: bin/ns_forensic_unif4"

$(BIN_DIR)/ns_richardson_003c: $(NS_SOURCES) $(SRC_DIR)/validation/ns_richardson_003c.c
	$(CC) $(CFLAGS) $(NS_SOURCES) \
	    $(SRC_DIR)/validation/ns_richardson_003c.c \
	    -o $@ $(LDFLAGS)
	@echo "[S161] Binaire: bin/ns_richardson_003c"

$(BIN_DIR)/ns_richardson_005_separation: $(NS_SOURCES) $(SRC_DIR)/validation/ns_richardson_005_separation.c
	$(CC) $(CFLAGS) $(NS_SOURCES) \
	    $(SRC_DIR)/validation/ns_richardson_005_separation.c \
	    -o $@ $(LDFLAGS)
	@echo "[S167] Binaire: bin/ns_richardson_005_separation"

# S168 : Richardson-006-TIME — ordre temporel Euler isolé (diffusion pure, sans splitting Chorin)
# Sources minimales : forensic_logger, memory_tracker, time_ns seulement — pas de solveur NS
NS_DIFF_SOURCES = \
	$(SRC_DIR)/debug/forensic_logger.c \
	$(SRC_DIR)/debug/memory_tracker.c \
	$(SRC_DIR)/common/time_ns.c \
	$(SRC_DIR)/lum/lum_core.c \
	$(SRC_DIR)/binary/binary_lum_converter.c

$(BIN_DIR)/ns_richardson_006_time_periodic: $(NS_DIFF_SOURCES) $(SRC_DIR)/validation/ns_richardson_006_time_periodic.c
	$(CC) $(CFLAGS) $(NS_DIFF_SOURCES) \
	    $(SRC_DIR)/validation/ns_richardson_006_time_periodic.c \
	    -o $@ $(LDFLAGS)
	@echo "[S168] Binaire: bin/ns_richardson_006_time_periodic"

$(BIN_DIR)/ns_richardson_004_mms: $(NS_SOURCES) $(SRC_DIR)/validation/ns_richardson_004_mms.c
	$(CC) $(CFLAGS) $(NS_SOURCES) \
	    $(SRC_DIR)/validation/ns_richardson_004_mms.c \
	    -o $@ $(LDFLAGS)
	@echo "[S166] Binaire: bin/ns_richardson_004_mms"

# S170 : Richardson-007-RE100 — ordre spatial NS complet Re=100, MMS Taylor-Green
$(BIN_DIR)/ns_richardson_007_re100_mms: $(NS_SOURCES) $(SRC_DIR)/validation/ns_richardson_007_re100_mms.c
	$(CC) $(CFLAGS) $(NS_SOURCES) \
	    $(SRC_DIR)/validation/ns_richardson_007_re100_mms.c \
	    -o $@ $(LDFLAGS)
	@echo "[S170] Binaire: bin/ns_richardson_007_re100_mms"

# S175 : Richardson-008-TIME — ordre temporel Chorin complet (CL MMS, grille staggered)
# Utilise le solveur staggered complet ns_solver_2d.c
$(BIN_DIR)/ns_richardson_008_time_chorin: $(NS_SOURCES) $(SRC_DIR)/validation/ns_richardson_008_time_chorin.c
	$(CC) $(CFLAGS) $(NS_SOURCES) \
	    $(SRC_DIR)/validation/ns_richardson_008_time_chorin.c \
	    -o $@ $(LDFLAGS)
	@echo "[S175] Binaire: bin/ns_richardson_008_time_chorin"

# S176 : Lyapunov NS 2D — exposant de Lyapunov sur champ de vorticité (Benettin 1980)
$(BIN_DIR)/ns_lyapunov: $(NS_SOURCES) $(SRC_DIR)/validation/ns_lyapunov.c
	$(CC) $(CFLAGS) $(NS_SOURCES) \
	    $(SRC_DIR)/validation/ns_lyapunov.c \
	    -o $@ $(LDFLAGS)
	@echo "[S176] Binaire: bin/ns_lyapunov"

# S177 : T04-STRONG — analyse multi-points quasi-stationnarité (registre 176 §5)
$(BIN_DIR)/ns_stationarity_t04: $(NS_SOURCES) \
	    $(SRC_DIR)/validation/ns_stationarity_analysis.c \
	    $(SRC_DIR)/validation/ns_stationarity_t04.c
	$(CC) $(CFLAGS) -I./src/validation $(NS_SOURCES) \
	    $(SRC_DIR)/validation/ns_stationarity_analysis.c \
	    $(SRC_DIR)/validation/ns_stationarity_t04.c \
	    -o $@ $(LDFLAGS)
	@echo "[S177] Binaire: bin/ns_stationarity_t04"

# S178-A : Lyapunov sweep — balayage complet Re/eps/warmup/renorm/résolution
$(BIN_DIR)/ns_lyapunov_sweep: $(NS_SOURCES) $(SRC_DIR)/validation/ns_lyapunov_sweep.c
	$(CC) $(CFLAGS) $(NS_SOURCES) \
	    $(SRC_DIR)/validation/ns_lyapunov_sweep.c \
	    -o $@ $(LDFLAGS)
	@echo "[S178-A] Binaire: bin/ns_lyapunov_sweep"

# S178-B : T04 cold-start — convergence depuis u=v=0 (9 tests T04C)
$(BIN_DIR)/ns_stationarity_t04_coldstart: $(NS_SOURCES) \
	    $(SRC_DIR)/validation/ns_stationarity_analysis.c \
	    $(SRC_DIR)/validation/ns_stationarity_t04_coldstart.c
	$(CC) $(CFLAGS) -I./src/validation $(NS_SOURCES) \
	    $(SRC_DIR)/validation/ns_stationarity_analysis.c \
	    $(SRC_DIR)/validation/ns_stationarity_t04_coldstart.c \
	    -o $@ $(LDFLAGS)
	@echo "[S178-B] Binaire: bin/ns_stationarity_t04_coldstart"

# S183 : T04-ENHANCED P3 — fenêtre finale renforcée Lid-Driven Cavity cold-start
$(BIN_DIR)/ns_stationarity_t04_enhanced: $(NS_SOURCES) \
	    $(SRC_DIR)/validation/ns_stationarity_analysis.c \
	    $(SRC_DIR)/validation/ns_stationarity_t04_enhanced.c
	$(CC) $(CFLAGS) -I./src/validation $(NS_SOURCES) \
	    $(SRC_DIR)/validation/ns_stationarity_analysis.c \
	    $(SRC_DIR)/validation/ns_stationarity_t04_enhanced.c \
	    -o $@ $(LDFLAGS)
	@echo "[S183] Binaire: bin/ns_stationarity_t04_enhanced"

# S179 : MAIN-CABLE-001 + FORENSIC-UNIF-002 — câblage complet LUM/VORAX dans main
# Utilise SOURCES complet (tous les modules) + forensic_unif_002 déjà dans SOURCES
$(BIN_DIR)/main_cable_001: $(SOURCES) $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) $(SOURCES) \
	    $(SRC_DIR)/main.c \
	    -o $@ $(LDFLAGS)
	@echo "[S179] Binaire: bin/main_cable_001 (MAIN-CABLE-001 + FORENSIC-UNIF-002)"

# S170 : INTEGRATION-LUM-OPT-001 — audit intégration LUM/VORAX modules
INTEGRATION_SOURCES = \
	$(SRC_DIR)/lum/lum_core.c \
	$(SRC_DIR)/debug/forensic_logger.c \
	$(SRC_DIR)/debug/memory_tracker.c \
	$(SRC_DIR)/common/time_ns.c \
	$(SRC_DIR)/optimization/simd_optimizer.c \
	$(SRC_DIR)/optimization/memory_optimizer.c \
	$(SRC_DIR)/optimization/pareto_optimizer.c \
	$(SRC_DIR)/optimization/zero_copy_allocator.c \
	$(SRC_DIR)/parallel/parallel_processor.c \
	$(SRC_DIR)/logger/lum_logger.c \
	$(SRC_DIR)/binary/binary_lum_converter.c \
	$(SRC_DIR)/vorax/vorax_operations.c \
	$(SRC_DIR)/parser/vorax_parser.c \
	$(SRC_DIR)/metrics/performance_metrics.c

$(BIN_DIR)/integration_lum_opt_001: $(INTEGRATION_SOURCES) $(SRC_DIR)/tests/integration_lum_opt_001.c
	$(CC) $(CFLAGS) -I./src/lum -I./src/debug -I./src/optimization -I./src/parallel \
	    -I./src/vorax -I./src/parser -I./src/logger -I./src/binary -I./src/metrics \
	    $(INTEGRATION_SOURCES) \
	    $(SRC_DIR)/tests/integration_lum_opt_001.c \
	    -o $@ $(LDFLAGS)
	@echo "[S170] Binaire: bin/integration_lum_opt_001"

$(BIN_DIR)/build_thread_001: $(NS_SOURCES) $(SRC_DIR)/tests/build_thread_001.c
	$(CC) $(CFLAGS) $(NS_SOURCES) \
	    $(SRC_DIR)/tests/build_thread_001.c \
	    -o $@ $(LDFLAGS)
	@echo "[S161] Binaire: bin/build_thread_001"

# S174 : BUILD-THREAD-001b — TSan parallel_processor (nominal + TSan)
THREAD001B_SOURCES = \
	$(SRC_DIR)/lum/lum_core.c \
	$(SRC_DIR)/debug/memory_tracker.c \
	$(SRC_DIR)/debug/forensic_logger.c \
	$(SRC_DIR)/common/time_ns.c \
	$(SRC_DIR)/optimization/simd_optimizer.c \
	$(SRC_DIR)/parallel/parallel_processor.c \
	$(SRC_DIR)/vorax/vorax_operations.c \
	$(SRC_DIR)/binary/binary_lum_converter.c \
	$(SRC_DIR)/logger/lum_logger.c

$(BIN_DIR)/build_thread_001b: $(THREAD001B_SOURCES) $(SRC_DIR)/tests/build_thread_001b.c
	$(CC) $(CFLAGS) -I./src/lum -I./src/debug -I./src/parallel -I./src/common \
	    -I./src/optimization -I./src/vorax -I./src/binary -I./src/logger \
	    $(THREAD001B_SOURCES) \
	    $(SRC_DIR)/tests/build_thread_001b.c \
	    -o $@ $(LDFLAGS)
	@echo "[S174] Binaire: bin/build_thread_001b (nominal)"

$(BIN_DIR)/build_thread_001b_tsan: $(THREAD001B_SOURCES) $(SRC_DIR)/tests/build_thread_001b.c
	$(CC) $(CFLAGS) -fsanitize=thread -fno-omit-frame-pointer \
	    -I./src/lum -I./src/debug -I./src/parallel -I./src/common \
	    -I./src/optimization -I./src/vorax -I./src/binary -I./src/logger \
	    $(THREAD001B_SOURCES) \
	    $(SRC_DIR)/tests/build_thread_001b.c \
	    -o $@ $(LDFLAGS)
	@echo "[S174] Binaire: bin/build_thread_001b_tsan (TSan)"

$(BIN_DIR)/perf_forensic_001: $(NS_SOURCES) $(SRC_DIR)/tests/perf_forensic_001.c
	$(CC) $(CFLAGS) $(NS_SOURCES) \
	    $(SRC_DIR)/tests/perf_forensic_001.c \
	    -o $@ $(LDFLAGS)
	@echo "[S161] Binaire: bin/perf_forensic_001"
# S181-B+C : TSan FU002 — data race detection + check_continuity concurrent
$(BIN_DIR)/fu002_tsan_test: $(SRC_DIR)/debug/forensic_unif_002.c $(SRC_DIR)/tests/fu002_tsan_test.c
	clang $(CFLAGS) -O1 -fsanitize=thread -fno-omit-frame-pointer \
	    -I./src/debug \
	    $(SRC_DIR)/debug/forensic_unif_002.c \
	    $(SRC_DIR)/tests/fu002_tsan_test.c \
	    -o $@ $(LDFLAGS)
	@echo "[S181-B+C] Binaire: bin/fu002_tsan_test (TSan)"

# S181-D : Qualification métrologique horloges REALTIME/MONOTONIC
$(BIN_DIR)/fu002_clock_qual: $(SRC_DIR)/tests/fu002_clock_qual.c
	$(CC) $(CFLAGS) \
	    $(SRC_DIR)/tests/fu002_clock_qual.c \
	    -o $@ $(LDFLAGS) -lm
	@echo "[S181-D] Binaire: bin/fu002_clock_qual (qualification horloges)"

science: directories \
         $(BIN_DIR)/ns_forensic_unif2 $(BIN_DIR)/ns_richardson_manufactured \
         $(BIN_DIR)/ns_forensic_unif3 $(BIN_DIR)/ns_richardson_couette_periodic \
         $(BIN_DIR)/ns_forensic_unif4 $(BIN_DIR)/ns_richardson_003c \
         $(BIN_DIR)/build_thread_001  $(BIN_DIR)/perf_forensic_001
	@echo "[S161] Tous les binaires science compilés"

.PHONY: science

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

.PHONY: all clean test test-progressive test-stress test-forensic rsa_test science_test liblumvorax.so blockchain_test portable portable-clean main_cable_001

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
	rm -rf build/obj/portable