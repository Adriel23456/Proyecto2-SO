# ===========================================================================
# Makefile MASTER - Sistema de Procesamiento Distribuido de Imágenes
# ===========================================================================

# ====== Config MPICH (ajustable) ===========================================
MPICH_ROOT ?= /usr/local/mpich-4.3.2
MPICC      := $(MPICH_ROOT)/bin/mpicc
RPATH_FLAG := -Wl,-rpath,$(MPICH_ROOT)/lib

# ====== Flags de compilación/enlace ========================================
CSTD   ?= c99
OPT    ?= -O2
WARN   ?= -Wall -Wextra
DEFS   ?= -DMASTER_BUILD

CFLAGS := $(WARN) $(OPT) -std=$(CSTD) $(DEFS)
LDFLAGS := $(RPATH_FLAG) -lm

# ====== Directorios / Salida ===============================================
SRC_DIR    := .
BUILD_DIR  := .
OUTPUT_DIR := $(HOME)/Documents/Proyecto2-SO/MainSystem
TARGET     := $(OUTPUT_DIR)/main

# ====== Fuentes del MASTER ==================================================
# Ajusta esta lista si renombraste/añadiste archivos.
SOURCES := \
  main.c \
  image_utils.c \
  mpi_comm.c \
  histogram.c

OBJECTS := $(SOURCES:.c=.o)

# ====== Headers (para dependencias) ========================================
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

# ---- Compilación genérica
%.o: %.c $(HEADERS)
	@echo "Compilando: $<"
	$(MPICC) $(CFLAGS) -c $< -o $@

# ---- IMPORTANTE: Solo ESTE .o define stb_* (evita símbolos duplicados)
# Si tus macros están DENTRO del .c, puedes borrar estas dos líneas.
image_utils.o: CFLAGS += -DSTB_IMAGE_IMPLEMENTATION -DSTB_IMAGE_WRITE_IMPLEMENTATION

# ---- Librerías de cabecera (stb) presentes
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

# ---- Utilidades
where:
	@echo "MPICH_ROOT  = $(MPICH_ROOT)"
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
	@echo "make            -> Compila (con RPATH a MPICH)"
	@echo "make clean      -> Limpia"
	@echo "make where      -> Muestra rutas/flags"
	@echo "make verify     -> Muestra dependencias con ldd"
	@echo ""
	@echo "Salida del binario:"
	@echo "  $(TARGET)"
	@echo ""
