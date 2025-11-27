#!/bin/bash

# Script para ejecutar benchmarks en notebook con casos pequeños
# Ejecuta todos los casos: 500, 1k, 2k, 5k, 10k
# Opcionalmente con Extrae/Paraver

# Colores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuración
DATOS_DIR="${1:-datos}"
REPETICIONES="${2:-3}"
MATCH=2
MISMATCH=0
GAP=-2
USAR_EXTRAE="${3:-no}"  # "si" o "no"

# Casos a ejecutar
declare -a CASOS=("500" "1k" "2k" "5k" "10k")

echo -e "${GREEN}=== Benchmark Notebook ===${NC}"
echo "Directorio de datos: $DATOS_DIR"
echo "Repeticiones: $REPETICIONES"
echo "Usar Extrae: $USAR_EXTRAE"
echo ""

# Verificar que el benchmark está compilado
if [ ! -f "bin/main-benchmark" ]; then
    echo -e "${YELLOW}Compilando benchmark...${NC}"
    # Intentar con compilar.sh primero, luego con make si existe
    if [ -f "./compilar.sh" ]; then
        ./compilar.sh benchmark
    elif command -v make &> /dev/null && [ -f "Makefile" ]; then
        make bin/main-benchmark
    else
        echo -e "${RED}Error: No se encontró make ni compilar.sh${NC}"
        echo "Ejecuta: ./compilar.sh benchmark"
        exit 1
    fi
    
    if [ ! -f "bin/main-benchmark" ]; then
        echo -e "${RED}Error al compilar${NC}"
        exit 1
    fi
fi

# Crear directorio de resultados
RESULTADOS_DIR="resultados_notebook"
mkdir -p "$RESULTADOS_DIR"

# Verificar Extrae si se solicita
if [ "$USAR_EXTRAE" = "si" ]; then
    if ! command -v extrae &> /dev/null; then
        echo -e "${RED}Error: Extrae no está instalado${NC}"
        echo "Ejecutando sin Extrae..."
        USAR_EXTRAE="no"
    else
        export EXTRAE_CONFIG_FILE="./extrae.xml"
        mkdir -p traces
    fi
fi

# Ejecutar cada caso
for caso in "${CASOS[@]}"; do
    for tipo in "dna" "protein"; do
        ARCHIVO="$DATOS_DIR/${tipo}_${caso}.fasta"
        
        if [ ! -f "$ARCHIVO" ]; then
            echo -e "${YELLOW}⚠ Archivo no encontrado: $ARCHIVO${NC}"
            continue
        fi
        
        echo -e "${BLUE}========================================${NC}"
        echo -e "${BLUE}Ejecutando: ${tipo}_${caso}${NC}"
        echo -e "${BLUE}========================================${NC}"
        
        RESULTADO_CSV="$RESULTADOS_DIR/benchmark_${tipo}_${caso}_$(date +%Y%m%d_%H%M%S).csv"
        
        if [ "$USAR_EXTRAE" = "si" ]; then
            TRACE_NAME="${tipo}_${caso}_$(date +%Y%m%d_%H%M%S)"
            export EXTRAE_TRACE_NAME="$TRACE_NAME"
            
            echo "Ejecutando con Extrae..."
            extrae -- ./bin/main-benchmark -f "$ARCHIVO" -p $MATCH $MISMATCH $GAP -r $REPETICIONES -o "$RESULTADO_CSV"
        else
            ./bin/main-benchmark -f "$ARCHIVO" -p $MATCH $MISMATCH $GAP -r $REPETICIONES -o "$RESULTADO_CSV"
        fi
        
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}✓ Completado: $RESULTADO_CSV${NC}"
        else
            echo -e "${RED}✗ Error en: $ARCHIVO${NC}"
        fi
        echo ""
    done
done

echo -e "${GREEN}=== Benchmark completado ===${NC}"
echo "Resultados en: $RESULTADOS_DIR"
if [ "$USAR_EXTRAE" = "si" ]; then
    echo "Trazas en: traces/"
    echo "Para visualizar: paraver traces/*.prv"
fi

