#!/bin/bash

# Script para ejecutar el benchmark con Extrae y generar trazas
# Uso: ./ejecutar_extrae.sh [archivo.fasta] [metodo] [repeticiones]

# Colores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Verificar si Extrae está instalado
if ! command -v extrae &> /dev/null; then
    echo -e "${RED}Error: Extrae no está instalado o no está en el PATH${NC}"
    echo "Instala Extrae o configura EXTRAE_HOME"
    exit 1
fi

# Configuración por defecto
ARCHIVO_FASTA="${1:-datos/test_500.fasta}"
METODO="${2:-antidiagonal_static}"
REPETICIONES="${3:-1}"
MATCH=2
MISMATCH=0
GAP=-2

# Directorio de trazas
TRACES_DIR="traces"
mkdir -p "$TRACES_DIR"

# Archivo de configuración de Extrae
EXTRAE_CONFIG="extrae.xml"

# Verificar si existe el archivo de configuración
if [ ! -f "$EXTRAE_CONFIG" ]; then
    echo -e "${YELLOW}Advertencia: No se encontró $EXTRAE_CONFIG${NC}"
    echo "Creando configuración básica..."
    
    cat > "$EXTRAE_CONFIG" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<trace>
  <enabled_tasks>
    <task enabled="yes" name="openmp"/>
  </enabled_tasks>
  
  <openmp enabled="yes">
    <trace_region enabled="yes"/>
    <trace_loop enabled="yes"/>
    <trace_wait_clause enabled="yes"/>
  </openmp>
  
  <sampling enabled="no"/>
  <mpi enabled="no"/>
  
  <output_dir>./traces</output_dir>
  <output_prefix>nw_trace</output_prefix>
</trace>
EOF
    echo -e "${GREEN}Configuración creada en $EXTRAE_CONFIG${NC}"
fi

echo -e "${GREEN}=== Compilando proyecto ===${NC}"
make clean
make bin/main-benchmark

if [ $? -ne 0 ]; then
    echo -e "${RED}Error al compilar. Abortando.${NC}"
    exit 1
fi

echo -e "\n${GREEN}=== Configurando Extrae ===${NC}"
export EXTRAE_CONFIG_FILE="./$EXTRAE_CONFIG"
export EXTRAE_HOME="${EXTRAE_HOME:-/usr}"

echo "Archivo FASTA: $ARCHIVO_FASTA"
echo "Método: $METODO"
echo "Repeticiones: $REPETICIONES"
echo "Directorio de trazas: $TRACES_DIR"
echo ""

# Nombre del archivo de traza
TRACE_NAME="${METODO}_$(date +%Y%m%d_%H%M%S)"
export EXTRAE_TRACE_NAME="$TRACE_NAME"

echo -e "${GREEN}=== Ejecutando con Extrae ===${NC}"
echo "Nota: Esto puede tomar más tiempo debido a la instrumentación"
echo ""

# Ejecutar con Extrae
extrae -- ./bin/main-benchmark -f "$ARCHIVO_FASTA" -p $MATCH $MISMATCH $GAP -r $REPETICIONES -o /dev/null

if [ $? -eq 0 ]; then
    echo -e "\n${GREEN}=== Traza generada exitosamente ===${NC}"
    
    # Buscar el archivo .prv generado
    PRV_FILE=$(find "$TRACES_DIR" -name "${TRACE_NAME}*.prv" | head -1)
    
    if [ -n "$PRV_FILE" ]; then
        echo "Archivo de traza: $PRV_FILE"
        echo ""
        echo "Para visualizar con Paraver:"
        echo "  paraver $PRV_FILE"
        echo ""
        echo "O abre Paraver y carga el archivo desde el menú File > Open"
    else
        echo -e "${YELLOW}No se encontró el archivo .prv generado${NC}"
        echo "Revisa el directorio $TRACES_DIR"
    fi
else
    echo -e "\n${RED}Error durante la ejecución con Extrae${NC}"
    exit 1
fi

