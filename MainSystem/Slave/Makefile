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

CFLAGS := $(WARN) $(OPT) -std=$(CSTD) $(DEFS)
LDFLAGS := $(RPATH_FLAG) -lm

# ====== Directorios / Salida (MISMA RUTA QUE EN MASTER) ====================
SRC_DIR    := .
OUTPUT_DIR := $(HOME)/Documents/Proyecto2-SO/MainSystem
TARGET     := $(OUTPUT_DIR)/main

# ====== Fuentes del SLAVE ===================================================
# Ajusta esta lista si renombraste/añadiste archivos.
SOURCES := \
  main.c \
  sobel_filter.c \
  image_io.c

OBJECTS := $(SOURCES:.c=.o)

# ====== Headers (para dependencias) ========================================
HEADERS := \
  config.h \
  sobel_filter.h \
  image_io.h \
  stb_image_write.h

# ===========================================================================
# Reglas principales
# ===========================================================================
.PHONY: all clean help check-libs where verify

all: check-libs $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  [SLAVE] Enlazando ejecutable..."
	@echo "═══════════════════════════════════════════════════════════"
	@mkdir -p $(OUTPUT_DIR)
	$(MPICC) -o $@ $^ $(LDFLAGS)
	@echo ""
	@echo "✓ SLAVE compilado -> $(TARGET)"
	@echo "  (Recuerda: este binario es local a este nodo)"
	@echo ""

# ---- Compilación genérica
%.o: %.c $(HEADERS)
	@echo "Compilando: $<"
	$(MPICC) $(CFLAGS) -c $< -o $@

# ---- IMPORTANTE: Solo ESTE .o define stb_image_write_* (evita duplicados)
# Si ya defines las macros dentro de image_io.c, puedes borrar esta línea.
image_io.o: CFLAGS += -DSTB_IMAGE_WRITE_IMPLEMENTATION

# ---- Librerías de cabecera (stb) presentes
check-libs:
	@echo "Verificando librerías de cabecera (stb)..."
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
	@echo "NOTA: No se borra $(TARGET) (compartido por master/slaves)"
	@echo "      Si quieres borrarlo también: rm -f $(TARGET)"
	@echo "✓ Limpieza completada"

help:
	@echo ""
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  Makefile - SLAVE"
	@echo "═══════════════════════════════════════════════════════════"
	@echo "make            -> Compila (con RPATH a MPICH)"
	@echo "make clean      -> Limpia objetos (no borra el binario)"
	@echo "make where      -> Muestra rutas/flags"
	@echo "make verify     -> Muestra dependencias con ldd"
	@echo ""
	@echo "Salida del binario:"
	@echo "  $(TARGET)"
	@echo ""
