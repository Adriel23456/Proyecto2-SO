# ===========================================================================
# Makefile SLAVE - Sistema de Procesamiento Distribuido de Imágenes
# ===========================================================================

# ====== Config OpenMPI ======================================================
OMPI_ROOT ?= /opt/openmpi-4.1.6
MPICC     := $(OMPI_ROOT)/bin/mpicc
RPATH_FLAG := -Wl,-rpath,$(OMPI_ROOT)/lib

# ====== Flags de compilación/enlace ========================================
CSTD   ?= c99
OPT    ?= -O2
WARN   ?= -Wall -Wextra
DEFS   ?= -DSLAVE_BUILD

# AÑADIR SOPORTE OPENMP
CFLAGS := $(WARN) $(OPT) -std=$(CSTD) $(DEFS) -fopenmp
LDFLAGS := $(RPATH_FLAG) -lm -fopenmp

# ====== Directorios / Salida (MISMA RUTA QUE EN MASTER) ====================
SRC_DIR    := .
OUTPUT_DIR := $(HOME)/Documents/Proyecto2-SO/MainSystem
TARGET     := $(OUTPUT_DIR)/main

# ====== Fuentes del SLAVE ===================================================
SOURCES := \
  main.c \
  sobel_filter.c \
  image_io.c

OBJECTS := $(SOURCES:.c=.o)

# ====== Headers ========================================
HEADERS := \
  config.h \
  sobel_filter.h \
  image_io.h \
  stb_image_write.h

# ===========================================================================
# Reglas principales
# ===========================================================================
.PHONY: all clean help check-libs

all: check-libs $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  [SLAVE] Enlazando ejecutable..."
	@echo "═══════════════════════════════════════════════════════════"
	@mkdir -p $(OUTPUT_DIR)
	$(MPICC) -o $@ $^ $(LDFLAGS)
	@echo ""
	@echo "✓ SLAVE compilado -> $(TARGET)"
	@echo ""

%.o: %.c $(HEADERS)
	@echo "Compilando: $<"
	$(MPICC) $(CFLAGS) -c $< -o $@

check-libs:
	@echo "Verificando librerías de cabecera (stb)..."
	@if [ ! -f stb_image_write.h ]; then \
		echo "✗ ERROR: stb_image_write.h no encontrado"; \
		echo "  wget https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h"; \
		exit 1; \
	fi
	@echo "✓ stb headers OK"
	@echo ""

clean:
	@echo "Limpiando objetos..."
	@rm -f $(OBJECTS)
	@echo "✓ Limpieza completada"