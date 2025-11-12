# ===========================================================================
# Makefile para Master - Sistema de Procesamiento Distribuido de Imágenes
# ===========================================================================

# Compilador y flags
CC = /usr/local/mpich-4.3.2/bin/mpicc
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS = -lm

# Directorios
SRC_DIR = .
BUILD_DIR = .
OUTPUT_DIR = $(HOME)/Documents/Proyecto2-SO/MainSystem

# Nombre del ejecutable (debe estar en MainSystem para que coincida con slaves)
TARGET = $(OUTPUT_DIR)/main

# Archivos fuente
SOURCES = main.c \
          image_utils.c \
          mpi_comm.c \
          histogram.c

# Archivos objeto
OBJECTS = $(SOURCES:.c=.o)

# Headers
HEADERS = config.h \
          image_utils.h \
          mpi_comm.h \
          histogram.h \
          stb_image.h \
          stb_image_write.h

# ===========================================================================
# Reglas principales
# ===========================================================================

.PHONY: all clean run help check-libs

all: check-libs $(TARGET)

# Compilar el ejecutable
$(TARGET): $(OBJECTS)
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  Enlazando ejecutable..."
	@echo "═══════════════════════════════════════════════════════════"
	@mkdir -p $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo ""
	@echo "✓ Compilación exitosa!"
	@echo "✓ Ejecutable creado en: $(TARGET)"
	@echo ""

# Compilar archivos objeto
%.o: %.c $(HEADERS)
	@echo "Compilando: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Verificar que las librerías stb estén disponibles
check-libs:
	@echo "Verificando librerías requeridas..."
	@if [ ! -f stb_image.h ]; then \
		echo ""; \
		echo "✗ ERROR: stb_image.h no encontrado"; \
		echo ""; \
		echo "Descárgalo con:"; \
		echo "  wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h"; \
		echo ""; \
		exit 1; \
	fi
	@if [ ! -f stb_image_write.h ]; then \
		echo ""; \
		echo "✗ ERROR: stb_image_write.h no encontrado"; \
		echo ""; \
		echo "Descárgalo con:"; \
		echo "  wget https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h"; \
		echo ""; \
		exit 1; \
	fi
	@echo "✓ Todas las librerías encontradas"
	@echo ""

# Limpiar archivos compilados
clean:
	@echo "Limpiando archivos objeto..."
	rm -f $(OBJECTS)
	@echo "Limpiando ejecutable..."
	rm -f $(TARGET)
	@echo "✓ Limpieza completada"

# Ejecutar (solo para pruebas locales, normalmente se usa run_mpi_safe.sh)
run: $(TARGET)
	@echo "Para ejecutar el programa, usa:"
	@echo "  ./run_mpi_safe.sh"

# Ayuda
help:
	@echo ""
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  Makefile - Sistema de Procesamiento Distribuido"
	@echo "═══════════════════════════════════════════════════════════"
	@echo ""
	@echo "Comandos disponibles:"
	@echo "  make          - Compila el proyecto"
	@echo "  make clean    - Limpia archivos compilados"
	@echo "  make help     - Muestra esta ayuda"
	@echo ""
	@echo "Para ejecutar:"
	@echo "  ./run_mpi_safe.sh"
	@echo ""
	@echo "Ubicación del ejecutable:"
	@echo "  $(TARGET)"
	@echo ""
	@echo "═══════════════════════════════════════════════════════════"
	@echo ""

# ===========================================================================
# Dependencias explícitas
# ===========================================================================

main.o: main.c config.h image_utils.h mpi_comm.h histogram.h
image_utils.o: image_utils.c image_utils.h config.h stb_image.h stb_image_write.h
mpi_comm.o: mpi_comm.c mpi_comm.h config.h
histogram.o: histogram.c histogram.h config.h stb_image_write.h