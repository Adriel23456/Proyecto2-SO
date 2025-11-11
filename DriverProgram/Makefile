###################################################################################
# Makefile para Sistema TFT Driver
#
# COMPONENTES:
# 1. Módulos del kernel:
#    - gpio_controller.ko: Controla GPIOs del hardware
#    - tft_driver.ko: Driver principal del display
#
# 2. Biblioteca de usuario:
#    - libtft.a: Biblioteca estática para programas de usuario
#
# 3. Herramientas de prueba:
#    - test_tft: Programa de prueba que usa libtft.a
#    - generate_histogram: Generador de archivos .cvc de ejemplo
#
# USO:
#   make          - Compila todo (módulos, biblioteca, herramientas)
#   make clean    - Limpia todos los archivos generados
#   make modules  - Solo compila módulos del kernel
#   make library  - Solo compila biblioteca
#   make tools    - Solo compila herramientas
###################################################################################

###############################################################################
# CONFIGURACIÓN DE MÓDULOS DEL KERNEL
###############################################################################

# Lista de módulos a compilar (sin extensión .ko)
# obj-m indica que se compilarán como módulos cargables
obj-m += gpio_controller.o
obj-m += tft_driver.o

# Directorio de headers del kernel actual
# $(shell uname -r) obtiene la versión del kernel en ejecución
KDIR = /lib/modules/$(shell uname -r)/build

###############################################################################
# CONFIGURACIÓN DE COMPILACIÓN USERSPACE
###############################################################################

# Compilador C
CC = gcc

# Archivador para crear biblioteca estática
AR = ar

# Flags de compilación
# -Wall: Habilita todos los warnings
# -O2: Nivel de optimización 2 (balance velocidad/tamaño)
CFLAGS = -Wall -O2

# Flags de enlace
# -lm: Enlazar con librería matemática (para funciones como fmod, fabs)
LDFLAGS = -lm

###############################################################################
# NOMBRES DE ARCHIVOS GENERADOS
###############################################################################

# Biblioteca estática
LIBRARY = libtft.a

# Objeto de la biblioteca
LIB_OBJ = libtft.o

# Programas ejecutables
TEST_PROG = test_tft
HISTOGRAM_GEN = generate_histogram

###############################################################################
# TARGET PRINCIPAL: Compila todo el sistema
###############################################################################

all: modules library tools
	@echo ""
	@echo "========================================="
	@echo "  COMPILACIÓN COMPLETADA EXITOSAMENTE"
	@echo "========================================="
	@echo ""
	@echo "MÓDULOS DEL KERNEL generados:"
	@ls -lh *.ko 2>/dev/null || echo "  (ninguno)"
	@echo ""
	@echo "BIBLIOTECA generada:"
	@ls -lh $(LIBRARY) 2>/dev/null || echo "  (ninguna)"
	@echo ""
	@echo "HERRAMIENTAS generadas:"
	@ls -lh $(TEST_PROG) $(HISTOGRAM_GEN) 2>/dev/null || echo "  (ninguna)"
	@echo ""
	@echo "SIGUIENTE PASO:"
	@echo "  1. Cargar drivers: sudo insmod gpio_controller.ko && sudo insmod tft_driver.ko"
	@echo "  2. Verificar: lsmod | grep -E 'tft|gpio'"
	@echo "  3. Probar: sudo ./test_tft fill F800"
	@echo ""

###############################################################################
# COMPILACIÓN DE MÓDULOS DEL KERNEL
###############################################################################

modules:
	@echo "==== Compilando módulos del kernel ===="
	@echo "Invocando sistema de compilación del kernel..."
	make -C $(KDIR) M=$(shell pwd) modules
	@echo "✓ Módulos compilados"
	@echo ""

###############################################################################
# COMPILACIÓN DE BIBLIOTECA ESTÁTICA
###############################################################################

library: $(LIBRARY)
	@echo "✓ Biblioteca compilada exitosamente"
	@echo ""

# Crear biblioteca estática desde objeto
# ar rcs: r=reemplazar, c=crear, s=crear índice
$(LIBRARY): $(LIB_OBJ)
	@echo "==== Creando biblioteca estática ===="
	$(AR) rcs $@ $^
	@echo "Biblioteca $(LIBRARY) creada"

# Compilar objeto de la biblioteca
$(LIB_OBJ): libtft.c libtft.h
	@echo "==== Compilando biblioteca libtft ===="
	$(CC) $(CFLAGS) -c libtft.c -o $(LIB_OBJ)
	@echo "Objeto $(LIB_OBJ) compilado"

###############################################################################
# COMPILACIÓN DE HERRAMIENTAS
###############################################################################

tools: $(TEST_PROG) $(HISTOGRAM_GEN)
	@echo "✓ Herramientas compiladas exitosamente"
	@echo ""

# Compilar programa de prueba (enlazado con biblioteca)
$(TEST_PROG): test_tft.c $(LIBRARY)
	@echo "==== Compilando programa de prueba ===="
	$(CC) $(CFLAGS) -o $@ test_tft.c -L. -ltft
	@echo "Programa $(TEST_PROG) compilado"

# Compilar generador de histogramas
$(HISTOGRAM_GEN): generate_histogram.c
	@echo "==== Compilando generador de histogramas ===="
	$(CC) $(CFLAGS) -o $@ generate_histogram.c $(LDFLAGS)
	@echo "Programa $(HISTOGRAM_GEN) compilado"

###############################################################################
# LIMPIEZA
###############################################################################

clean:
	@echo "==== Limpiando archivos generados ===="
	@echo "Limpiando módulos del kernel..."
	make -C $(KDIR) M=$(shell pwd) clean
	@echo "Limpiando biblioteca y herramientas..."
	rm -f $(LIBRARY) $(LIB_OBJ) $(TEST_PROG) $(HISTOGRAM_GEN)
	rm -f *.o histogram.cvc
	@echo "✓ Limpieza completada"
	@echo ""

###############################################################################
# PHONY TARGETS
# Estos targets no representan archivos reales
###############################################################################

.PHONY: all modules library tools clean