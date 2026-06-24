# MDBAI — Makefile forensic library - PHASE 3 INTEGRATION LUMVORAX
# Compile libmdbai_forensic.so (LD_PRELOAD injection)
# Conforme STANDARD_NAMES_MDBAI.md Section 9
# Conforme prompt.txt Règle #1 : traçabilité bit-level LumVorax
# CYCLE C191 - Phase 3: Intégration modules moyenne priorité

CC       = gcc
CFLAGS   = -Wall -Wextra -O2 -std=c11 -fPIC -D_DEFAULT_SOURCE -D_GNU_SOURCE
LDFLAGS  = -shared -ldl -lpthread -llz4

# Chemins sources
LUM_SRC  = ../../../src/lum
DBG_SRC  = ../../../src/debug
CMN_SRC  = ../../../src/common
LOG_SRC  = ../../../src/logging
BIN_SRC  = ../../../src/binary
LOGGER_SRC = ../../../src/logger
FMT_SRC  = ../../../src/file_formats
VORAX_SRC = ../../../src/vorax
OPT_SRC = ../../../src/optimization
SPATIAL_SRC = ../../../src/spatial
PERSIST_SRC = ../../../src/persistence
PARSER_SRC = ../../../src/parser

# Includes
INCLUDES = -I$(LUM_SRC) -I$(DBG_SRC) -I$(CMN_SRC) -I$(LOG_SRC) -I$(BIN_SRC) \
           -I$(LOGGER_SRC) -I$(FMT_SRC) -I$(VORAX_SRC) -I$(OPT_SRC) \
           -I$(OPT_SRC)/async_logging -I$(OPT_SRC)/lockfree \
           -I$(OPT_SRC)/lz4_compression -I$(OPT_SRC)/mmap_io \
           -I$(OPT_SRC)/simd_batch -I$(OPT_SRC)/slab_allocator \
           -I$(SPATIAL_SRC) -I$(PERSIST_SRC) -I$(PARSER_SRC)

# ── PHASE 1: MODULES CRITIQUES (54 fichiers) ──

# Sources /lum (8 fichiers critiques - GPU exclu temporairement)
LUM_SRCS = $(LUM_SRC)/lum_core.c \
           $(LUM_SRC)/lum_memory_tracer.c \
           $(LUM_SRC)/lum_log_encoder.c \
           $(LUM_SRC)/lum_btree.c \
           $(LUM_SRC)/lum_catalog.c \
           $(LUM_SRC)/lum_query.c \
           $(LUM_SRC)/lum_raft.c \
           $(LUM_SRC)/lum_replication.c
# NOTE: lum_gpu_context.c exclu (dépendance DRM Intel i915)

# Sources /debug (5 fichiers - unified_logging exclu: doublon avec forensic_logger)
DBG_SRCS = $(DBG_SRC)/memory_tracker.c \
           $(DBG_SRC)/ultra_forensic_logger.c \
           $(DBG_SRC)/forensic_logger.c \
           $(DBG_SRC)/enhanced_logging.c \
           $(DBG_SRC)/logging_system.c
# NOTE: unified_logging.c exclu (symboles dupliqués avec forensic_logger.c)

# Sources /logging (1 fichier)
LOG_SRCS = $(LOG_SRC)/log_writer.c

# Sources /common (2 fichiers)
CMN_SRCS = $(CMN_SRC)/time_ns.c \
           $(CMN_SRC)/lvx_log_rotate.c

# Sources /binary (1 fichier)
BIN_SRCS = $(BIN_SRC)/binary_lum_converter.c

# Source MDBAI (1 fichier)
MDBAI_SRCS = mdbai_forensic.c

# ── PHASE 2: MODULES HAUTE PRIORITÉ (34 fichiers additionnels) ──

# Sources /logger (2 fichiers)
LOGGER_SRCS = $(LOGGER_SRC)/lum_logger.c \
              $(LOGGER_SRC)/log_manager.c

# Sources /file_formats (3 fichiers)
FMT_SRCS = $(FMT_SRC)/lum_native_file_handler.c \
           $(FMT_SRC)/lum_native_universal_format.c \
           $(FMT_SRC)/lum_secure_serialization.c

# Sources /vorax (3 fichiers - stubs exclus)
VORAX_SRCS = $(VORAX_SRC)/vorax_operations.c \
             $(VORAX_SRC)/vorax_3d_volume.c \
             $(VORAX_SRC)/vorax_sha256.c
# NOTE: Stubs exclus (lum_core_stub, lum_logger_stub, memory_tracker_stub)

# Sources /optimization racine (8 fichiers)
OPT_ROOT_SRCS = $(OPT_SRC)/memory_optimizer.c \
                $(OPT_SRC)/simd_optimizer.c \
                $(OPT_SRC)/pareto_optimizer.c \
                $(OPT_SRC)/pareto_inverse_optimizer.c \
                $(OPT_SRC)/thermal_regulator.c \
                $(OPT_SRC)/zero_copy_allocator.c \
                $(OPT_SRC)/formal_kernel_v40.c \
                $(OPT_SRC)/reasoning_path_tracker.c

# Sources /optimization sous-modules (6 fichiers)
OPT_ASYNC_SRCS = $(OPT_SRC)/async_logging/async_logger.c
OPT_LOCKFREE_SRCS = $(OPT_SRC)/lockfree/lockfree_queue.c
OPT_LZ4_SRCS = $(OPT_SRC)/lz4_compression/lz4_compressor.c
OPT_MMAP_SRCS = $(OPT_SRC)/mmap_io/mmap_persistence.c
OPT_SIMD_SRCS = $(OPT_SRC)/simd_batch/simd_batch_processor.c
OPT_SLAB_SRCS = $(OPT_SRC)/slab_allocator/slab_allocator.c

OPT_SRCS = $(OPT_ROOT_SRCS) $(OPT_ASYNC_SRCS) $(OPT_LOCKFREE_SRCS) \
           $(OPT_LZ4_SRCS) $(OPT_MMAP_SRCS) $(OPT_SIMD_SRCS) $(OPT_SLAB_SRCS)

# ── PHASE 3: MODULES MOYENNE PRIORITÉ (9 fichiers additionnels) ──

# Sources /spatial (1 fichier)
SPATIAL_SRCS = $(SPATIAL_SRC)/lum_instant_displacement.c

# Sources /persistence (7 fichiers)
PERSIST_SRCS = $(PERSIST_SRC)/data_persistence.c \
               $(PERSIST_SRC)/async_io_manager.c \
               $(PERSIST_SRC)/lum_buffer_pool.c \
               $(PERSIST_SRC)/lum_mvcc.c \
               $(PERSIST_SRC)/mmap_wal.c \
               $(PERSIST_SRC)/recovery_manager_extension.c \
               $(PERSIST_SRC)/transaction_wal_extension.c

# Sources /parser (1 fichier)
PARSER_SRCS = $(PARSER_SRC)/vorax_parser.c

# Total sources Phase 1+2+3: 61 fichiers .c
SRCS = $(MDBAI_SRCS) $(LUM_SRCS) $(DBG_SRCS) $(LOG_SRCS) $(CMN_SRCS) $(BIN_SRCS) \
       $(LOGGER_SRCS) $(FMT_SRCS) $(VORAX_SRCS) $(OPT_SRCS) \
       $(SPATIAL_SRCS) $(PERSIST_SRCS) $(PARSER_SRCS)

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
