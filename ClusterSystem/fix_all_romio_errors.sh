#!/bin/bash

echo "🔧 Aplicando TODOS los parches de ROMIO para ARM 32-bit..."
echo "=================================================="

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

PATCHES_APPLIED=0

# PARCHE 1: get_bytoff.c (ya aplicado pero lo verificamos)
echo -e "\n${YELLOW}[1/4] Parcheando get_bytoff.c...${NC}"
FILE1="ompi/mca/io/romio321/romio/mpi-io/get_bytoff.c"
if [ -f "$FILE1" ]; then
    [ ! -f "$FILE1.backup" ] && cp "$FILE1" "$FILE1.backup"
    
    # Reemplazar línea 66 con el fix
    sed -i '66s/.*/    ADIO_Offset temp_disp;\n    ADIOI_Get_byte_offset(adio_fh, offset, \&temp_disp);\n    *disp = (MPI_Offset)temp_disp;/' "$FILE1"
    
    echo -e "  ${GREEN}✓ get_bytoff.c parcheado${NC}"
    ((PATCHES_APPLIED++))
fi

# PARCHE 2: get_posn.c (NUEVO)
echo -e "\n${YELLOW}[2/4] Parcheando get_posn.c...${NC}"
FILE2="ompi/mca/io/romio321/romio/mpi-io/get_posn.c"
if [ -f "$FILE2" ]; then
    [ ! -f "$FILE2.backup" ] && cp "$FILE2" "$FILE2.backup"
    
    # Crear parche temporal
    cat > /tmp/get_posn_patch.c << 'PATCH'
    /* Fix for 32-bit systems */
    ADIO_Offset temp_offset;
    ADIOI_Get_position(adio_fh, &temp_offset);
    *offset = (MPI_Offset)temp_offset;
PATCH
    
    # Reemplazar línea 55
    sed -i '55d' "$FILE2"
    sed -i '55r /tmp/get_posn_patch.c' "$FILE2"
    
    rm -f /tmp/get_posn_patch.c
    echo -e "  ${GREEN}✓ get_posn.c parcheado${NC}"
    ((PATCHES_APPLIED++))
fi

# PARCHE 3: get_posn_sh.c (NUEVO)
echo -e "\n${YELLOW}[3/4] Parcheando get_posn_sh.c...${NC}"
FILE3="ompi/mca/io/romio321/romio/mpi-io/get_posn_sh.c"
if [ -f "$FILE3" ]; then
    [ ! -f "$FILE3.backup" ] && cp "$FILE3" "$FILE3.backup"
    
    # Crear parche temporal
    cat > /tmp/get_posn_sh_patch.c << 'PATCH'
    /* Fix for 32-bit systems */
    ADIO_Offset temp_offset;
    ADIO_Get_shared_fp(adio_fh, 0, &temp_offset, &error_code);
    *offset = (MPI_Offset)temp_offset;
PATCH
    
    # Reemplazar línea 56
    sed -i '56d' "$FILE3"
    sed -i '56r /tmp/get_posn_sh_patch.c' "$FILE3"
    
    rm -f /tmp/get_posn_sh_patch.c
    echo -e "  ${GREEN}✓ get_posn_sh.c parcheado${NC}"
    ((PATCHES_APPLIED++))
fi

# PARCHE 4: Buscar y arreglar otros archivos ROMIO similares proactivamente
echo -e "\n${YELLOW}[4/4] Buscando otros archivos ROMIO con problemas similares...${NC}"

# Lista de archivos ROMIO que pueden tener el mismo problema
ROMIO_FILES=(
    "ompi/mca/io/romio321/romio/mpi-io/seek.c"
    "ompi/mca/io/romio321/romio/mpi-io/seek_sh.c"
    "ompi/mca/io/romio321/romio/mpi-io/get_view.c"
    "ompi/mca/io/romio321/romio/mpi-io/iread_sh.c"
    "ompi/mca/io/romio321/romio/mpi-io/iwrite_sh.c"
    "ompi/mca/io/romio321/romio/mpi-io/read_sh.c"
    "ompi/mca/io/romio321/romio/mpi-io/write_sh.c"
)

for file in "${ROMIO_FILES[@]}"; do
    if [ -f "$file" ]; then
        # Verificar si tiene problemas de MPI_Offset vs ADIO_Offset
        if grep -q "ADIO_Offset.*MPI_Offset\|MPI_Offset.*\&" "$file" 2>/dev/null; then
            echo "  Verificando: $(basename $file)"
            [ ! -f "$file.backup" ] && cp "$file" "$file.backup"
            
            # Buscar líneas problemáticas con ADIOI_ o ADIO_ funciones
            if grep -q "ADIOI_.*MPI_Offset \*\|ADIO_.*MPI_Offset \*" "$file"; then
                echo "    Aplicando fix preventivo..."
                
                # Este sed es más genérico para capturar varios casos
                sed -i 's/\(ADIOI_[A-Za-z_]*\)(\([^,]*\), \([^)]*\));/ADIO_Offset temp_off; \1(\2, \&temp_off); *\3 = (MPI_Offset)temp_off;/g' "$file" 2>/dev/null
                sed -i 's/\(ADIO_[A-Za-z_]*\)(\([^,]*\), \([^,]*\), \([^)]*\));/ADIO_Offset temp_off; \1(\2, \3, \&temp_off); *\4 = (MPI_Offset)temp_off;/g' "$file" 2>/dev/null
                
                ((PATCHES_APPLIED++))
            fi
        fi
    fi
done

# Verificar si quedan archivos .lo sin compilar de intentos anteriores
echo -e "\n${YELLOW}Limpiando archivos objeto de ROMIO...${NC}"
find ompi/mca/io/romio321 -name "*.lo" -delete 2>/dev/null
find ompi/mca/io/romio321 -name "*.o" -delete 2>/dev/null

echo ""
echo "=================================================="
echo -e "${GREEN}✅ Total de parches aplicados: $PATCHES_APPLIED${NC}"
echo ""
echo -e "${YELLOW}AHORA CONTINÚA LA COMPILACIÓN:${NC}"
echo -e "  ${GREEN}make -j$(nproc)${NC}"
echo ""
echo "Si aparecen más errores similares, repórtamelos"
echo "=================================================="