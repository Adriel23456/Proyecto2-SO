# ===========================================================================
# Makefile MASTER - Sistema de Procesamiento Distribuido de Imágenes
# ===========================================================================

# ====== Config OpenMPI ======================================================
OMPI_ROOT ?= /opt/openmpi-4.1.6
MPICC     := $(OMPI_ROOT)/bin/mpicc
RPATH_FLAG := -Wl,-rpath,$(OMPI_ROOT)/lib

# ====== Config TFT ==========================================================
TFT_LIB_DIR ?= /usr/local/lib

# ====== Flags de compilación/enlace ========================================
CSTD   ?= c99
OPT    ?= -O2
WARN   ?= -Wall -Wextra
DEFS   ?= -DMASTER_BUILD

# === AÑADIR OPENMP ===
CFLAGS  := $(WARN) $(OPT) -std=$(CSTD) $(DEFS) -fopenmp
LDFLAGS := $(RPATH_FLAG) -L$(TFT_LIB_DIR) -ltft -lm -fopenmp

# ====== Directorios / Salida ===============================================
SRC_DIR    := .
BUILD_DIR  := .
OUTPUT_DIR := $(HOME)/Documents/Proyecto2-SO/MainSystem
TARGET     := $(OUTPUT_DIR)/main

# ====== Fuentes del MASTER ==================================================
SOURCES := \
  main.c \
  image_utils.c \
  mpi_comm.c \
  histogram.c

OBJECTS := $(SOURCES:.c=.o)

# ====== Headers =============================================================
HEADERS := \
  config.h \
  image_utils.h \
  mpi_comm.h \
  histogram.h \
  stb_image.h \
  stb_image_write.h

# ===========================================================================
# Reglas principales
# ===========================================================================
.PHONY: all clean help check-libs where verify

all: check-libs $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  [MASTER] Enlazando ejecutable..."
	@echo "═══════════════════════════════════════════════════════════"
	@mkdir -p $(OUTPUT_DIR)
	$(MPICC) -o $@ $^ $(LDFLAGS)
	@echo ""
	@echo "✓ MASTER compilado -> $(TARGET)"
	@echo ""

%.o: %.c $(HEADERS)
	@echo "Compilando: $<"
	$(MPICC) $(CFLAGS) -c $< -o $@

# Solo ESTE archivo implementa stb_image (evita símbolos duplicados)
image_utils.o: CFLAGS += -DSTB_IMAGE_IMPLEMENTATION -DSTB_IMAGE_WRITE_IMPLEMENTATION

# ---- Librerías de cabecera
check-libs:
	@echo "Verificando librerías de cabecera (stb)..."
	@if [ ! -f stb_image.h ]; then \
		echo "✗ ERROR: stb_image.h no encontrado"; \
		echo "  wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h"; \
		exit 1; \
	fi
	@if [ ! -f stb_image_write.h ]; then \
		echo "✗ ERROR: stb_image_write.h no encontrado"; \
		echo "  wget https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h"; \
		exit 1; \
	fi
	@echo "✓ stb headers OK"
	@echo ""

where:
	@echo "OMPI_ROOT   = $(OMPI_ROOT)"
	@echo "MPICC       = $(MPICC)"
	@echo "TARGET      = $(TARGET)"
	@echo "RPATH_FLAG  = $(RPATH_FLAG)"

verify: $(TARGET)
	@echo "ldd del ejecutable:"
	@ldd $(TARGET) | egrep 'libmpi|libz|libpng' || true

clean:
	@echo "Limpiando objetos..."
	@rm -f $(OBJECTS)
	@echo "Limpiando ejecutable..."
	@rm -f $(TARGET)
	@echo "✓ Limpieza completada"

help:
	@echo ""
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  Makefile - MASTER"
	@echo "═══════════════════════════════════════════════════════════"
	@echo "make            -> Compila"
	@echo "make clean      -> Limpia"
	@echo "make where      -> Muestra rutas"
	@echo "make verify     -> Revisa dependencias"
	@echo ""
