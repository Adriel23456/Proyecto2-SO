#!/bin/bash

echo "🔧 Aplicando TODOS los parches para ARM 32-bit en OpenMPI 4.1.6..."
echo "=================================================="

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Contador de parches
PATCHES_APPLIED=0

# PARCHE 1: Arreglar todos los archivos hwloc
echo -e "\n${YELLOW}[1/3] Parcheando archivos hwloc...${NC}"
for file in $(find . -name "*hwloc*.c" | grep -E "(hwloc\.c|rtc_hwloc\.c)"); do
    if [ -f "$file" ]; then
        echo "  Parcheando: $file"
        
        # Hacer backup solo si no existe
        [ ! -f "$file.backup" ] && cp "$file" "$file.backup"
        
        # Aplicar parches para use_hole
        sed -i 's/return use_hole(0, begin, addrp, size);/{ unsigned long addr_temp = (unsigned long)*addrp; return use_hole(0, begin, \&addr_temp, size); }/g' "$file"
        
        sed -i 's/return use_hole(prevend, begin-prevend, addrp, size);/{ unsigned long addr_temp = (unsigned long)*addrp; return use_hole(prevend, begin-prevend, \&addr_temp, size); }/g' "$file"
        
        sed -i 's/return use_hole(biggestbegin, biggestsize, addrp, size);/{ unsigned long addr_temp = (unsigned long)*addrp; int ret = use_hole(biggestbegin, biggestsize, \&addr_temp, size); *addrp = (size_t)addr_temp; return ret; }/g' "$file"
        
        ((PATCHES_APPLIED++))
    fi
done
echo -e "  ${GREEN}✓ Archivos hwloc parcheados${NC}"

# PARCHE 2: Arreglar ROMIO get_bytoff.c
ROMIO_FILE="ompi/mca/io/romio321/romio/mpi-io/get_bytoff.c"
echo -e "\n${YELLOW}[2/3] Parcheando ROMIO (get_bytoff.c)...${NC}"
if [ -f "$ROMIO_FILE" ]; then
    echo "  Parcheando: $ROMIO_FILE"
    
    # Hacer backup
    [ ! -f "$ROMIO_FILE.backup" ] && cp "$ROMIO_FILE" "$ROMIO_FILE.backup"
    
    # Crear archivo temporal con el parche
    cat > /tmp/romio_patch.txt << 'PATCH'
--- a/ompi/mca/io/romio321/romio/mpi-io/get_bytoff.c
+++ b/ompi/mca/io/romio321/romio/mpi-io/get_bytoff.c
@@ -63,7 +63,12 @@
     }
 
     ADIOI_TEST_DEFERRED(adio_fh, myname, &error_code);
-    ADIOI_Get_byte_offset(adio_fh, offset, disp);
+    
+    /* Fix for 32-bit systems where MPI_Offset != ADIO_Offset */
+    ADIO_Offset temp_disp;
+    ADIOI_Get_byte_offset(adio_fh, offset, &temp_disp);
+    *disp = (MPI_Offset)temp_disp;
+    
     *disp = *disp + adio_fh->disp;
 
 fn_exit:
PATCH
    
    # Aplicar el parche
    if patch -p1 -N --dry-run < /tmp/romio_patch.txt >/dev/null 2>&1; then
        patch -p1 -N < /tmp/romio_patch.txt
        echo -e "  ${GREEN}✓ ROMIO parcheado con patch${NC}"
    else
        # Si patch no funciona, usar sed
        echo "  Aplicando parche alternativo con sed..."
        sed -i '66s/.*/    ADIO_Offset temp_disp;\n    ADIOI_Get_byte_offset(adio_fh, offset, \&temp_disp);\n    *disp = (MPI_Offset)temp_disp;/' "$ROMIO_FILE"
        echo -e "  ${GREEN}✓ ROMIO parcheado con sed${NC}"
    fi
    rm -f /tmp/romio_patch.txt
    ((PATCHES_APPLIED++))
else
    echo -e "  ${RED}✗ Archivo ROMIO no encontrado${NC}"
fi

# PARCHE 3: Arreglar otros archivos ROMIO potencialmente problemáticos
echo -e "\n${YELLOW}[3/3] Buscando otros archivos ROMIO con problemas de tipos...${NC}"

# Lista de archivos ROMIO que suelen tener problemas en 32-bit
ROMIO_PROBLEM_FILES=(
    "ompi/mca/io/romio321/romio/adio/common/ad_read.c"
    "ompi/mca/io/romio321/romio/adio/common/ad_write.c"
    "ompi/mca/io/romio321/romio/adio/common/ad_seek.c"
)

for file in "${ROMIO_PROBLEM_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "  Verificando: $file"
        
        # Hacer backup
        [ ! -f "$file.backup" ] && cp "$file" "$file.backup"
        
        # Buscar y arreglar conversiones problemáticas MPI_Offset <-> ADIO_Offset
        if grep -q "ADIO_Offset.*MPI_Offset\|MPI_Offset.*ADIO_Offset" "$file"; then
            echo "    Aplicando correcciones de tipos..."
            
            # Agregar casts explícitos donde sea necesario
            sed -i 's/\(ADIO_Offset[[:space:]]*\*\)\([[:alnum:]_]*\)[[:space:]]*=[[:space:]]*\(.*MPI_Offset[[:space:]]*\*.*\)/\1\2 = (ADIO_Offset *)\3/g' "$file" 2>/dev/null
            sed -i 's/\(MPI_Offset[[:space:]]*\*\)\([[:alnum:]_]*\)[[:space:]]*=[[:space:]]*\(.*ADIO_Offset[[:space:]]*\*.*\)/\1\2 = (MPI_Offset *)\3/g' "$file" 2>/dev/null
            
            ((PATCHES_APPLIED++))
        fi
    fi
done

echo ""
echo "=================================================="
echo -e "${GREEN}✅ Total de parches aplicados: $PATCHES_APPLIED${NC}"
echo ""
echo -e "${YELLOW}SIGUIENTE PASO:${NC}"
echo "  Continúa la compilación con:"
echo -e "  ${GREEN}make -j\$(nproc)${NC}"
echo ""
echo "Si aún hay errores en ROMIO, puedes deshabilitarlo:"
echo -e "  ${YELLOW}./configure_arm32.sh${NC}  (ya incluye --disable-io-romio)"
echo "  ${GREEN}make clean && make -j\$(nproc)${NC}"
echo "=================================================="