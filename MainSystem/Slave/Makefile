# ===========================================================================
# Makefile para Slave - Sistema de Procesamiento Distribuido de Imágenes
# ===========================================================================

# Compilador y flags
CC = /usr/local/mpich-4.3.2/bin/mpicc
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS = -lm

# Directorios
SRC_DIR = .
OUTPUT_DIR = $(HOME)/Documents/Proyecto2-SO/MainSystem

# Nombre del ejecutable (MISMO NOMBRE que el master, pero diferente código)
TARGET = $(OUTPUT_DIR)/main

# Archivos fuente
SOURCES = main.c \
          sobel_filter.c \
          image_io.c

# Archivos objeto
OBJECTS = $(SOURCES:.c=.o)

# Headers
HEADERS = config.h \
          sobel_filter.h \
          image_io.h \
          stb_image_write.h

# ===========================================================================
# Reglas principales
# ===========================================================================

.PHONY: all clean help check-libs

all: check-libs $(TARGET)

# Compilar el ejecutable
$(TARGET): $(OBJECTS)
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  Enlazando ejecutable del SLAVE..."
	@echo "═══════════════════════════════════════════════════════════"
	@mkdir -p $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo ""
	@echo "✓ Compilación del SLAVE exitosa!"
	@echo "✓ Ejecutable creado en: $(TARGET)"
	@echo ""
	@echo "IMPORTANTE: Este ejecutable del SLAVE sobrescribe el del MASTER."
	@echo "            Asegúrate de compilar en TODOS los nodos slaves."
	@echo ""

# Compilar archivos objeto
%.o: %.c $(HEADERS)
	@echo "Compilando: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Verificar que las librerías stb estén disponibles
check-libs:
	@echo "Verificando librerías requeridas..."
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
	@echo "✓ Limpieza completada"
	@echo ""
	@echo "NOTA: El ejecutable $(TARGET) NO se eliminó"
	@echo "      (es compartido entre master y slaves)"

# Ayuda
help:
	@echo ""
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  Makefile - SLAVE (Procesamiento Distribuido)"
	@echo "═══════════════════════════════════════════════════════════"
	@echo ""
	@echo "Comandos disponibles:"
	@echo "  make          - Compila el proyecto del slave"
	@echo "  make clean    - Limpia archivos compilados"
	@echo "  make help     - Muestra esta ayuda"
	@echo ""
	@echo "IMPORTANTE:"
	@echo "  Este Makefile compila el SLAVE y lo coloca en:"
	@echo "  $(TARGET)"
	@echo ""
	@echo "  El master y todos los slaves DEBEN tener el ejecutable"
	@echo "  en la MISMA RUTA pero con código diferente."
	@echo ""
	@echo "═══════════════════════════════════════════════════════════"
	@echo ""

# ===========================================================================
# Dependencias explícitas
# ===========================================================================

main.o: main.c config.h sobel_filter.h image_io.h
sobel_filter.o: sobel_filter.c sobel_filter.h config.h
image_io.o: image_io.c image_io.h config.h stb_image_write.h