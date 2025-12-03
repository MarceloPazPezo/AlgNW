#!/bin/bash
# Script bash para benchmarks con tamaños grandes (32k)
# Equivalente a ejecutar-benchmark-grandes-ps.ps1 pero para Linux

# Colores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configuración por defecto
METODO="${1:-bloques}"
THREADS="${2:-4}"
ARCHIVOS="${3:-dna_32k}"
REPETICIONES="${4:-5}"
OUTPUT_FILE="${5:-resultados_grandes.csv}"

# Schedules más prometedores basados en análisis anterior
SCHEDULES=(
    "dynamic"
    "dynamic,1"
    "dynamic,2"
    "guided,1"
    "guided,2"
    "static,4"
    "static,8"
)

# Configuración
DATOS_DIR="datos"
RESULTADOS_DIR="resultados"
ARCHIVO_SALIDA="$RESULTADOS_DIR/$OUTPUT_FILE"

# Parámetros de puntuación
MATCH=2
MISMATCH=-1
GAP=-2

# Crear directorios
mkdir -p "$RESULTADOS_DIR"

# Verificar que los programas existen
if [ ! -f "bin/main-paralelo" ]; then
    echo -e "${RED}Error: bin/main-paralelo no encontrado${NC}"
    echo -e "${YELLOW}Ejecuta: make${NC}"
    exit 1
fi

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}BENCHMARK TAMAÑOS GRANDES${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo -e "${CYAN}Método: $METODO${NC}"
echo -e "${CYAN}Threads: $THREADS${NC}"
echo -e "${CYAN}Schedules: ${#SCHEDULES[@]} configuraciones${NC}"
echo -e "${CYAN}Archivos: $ARCHIVOS${NC}"
echo -e "${CYAN}Repeticiones: $REPETICIONES${NC}"
echo -e "${CYAN}Archivo de salida: $ARCHIVO_SALIDA${NC}"
echo ""

# Primero ejecutar secuencial para comparación
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Ejecutando SECUENCIAL (baseline)${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

for archivo_base in $ARCHIVOS; do
    ARCHIVO="$DATOS_DIR/${archivo_base}.fasta"
    
    if [ ! -f "$ARCHIVO" ]; then
        echo -e "${YELLOW}Saltando $archivo_base (archivo no encontrado: $ARCHIVO)${NC}"
        continue
    fi
    
    echo -ne "  Ejecutando secuencial para $archivo_base... "
    
    export OMP_NUM_THREADS=1
    ./bin/main-paralelo -f "$ARCHIVO" -p $MATCH $MISMATCH $GAP -r $REPETICIONES -m "secuencial" -o "$ARCHIVO_SALIDA" > /dev/null 2>&1
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}OK${NC}"
    else
        echo -e "${RED}FAIL${NC}"
    fi
done

echo ""

# Ahora ejecutar paralelo
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Ejecutando PARALELO${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

total_tests=$((${#SCHEDULES[@]} * 1))  # 1 porque THREADS es un solo valor
current_test=0

for archivo_base in $ARCHIVOS; do
    ARCHIVO="$DATOS_DIR/${archivo_base}.fasta"
    
    if [ ! -f "$ARCHIVO" ]; then
        echo -e "${YELLOW}Saltando $archivo_base (archivo no encontrado: $ARCHIVO)${NC}"
        continue
    fi
    
    echo -e "${BLUE}----------------------------------------${NC}"
    echo -e "${BLUE}Archivo: ${archivo_base}.fasta${NC}"
    echo -e "${BLUE}----------------------------------------${NC}"
    echo ""
    
    for schedule in "${SCHEDULES[@]}"; do
        current_test=$((current_test + 1))
        porcentaje=$(echo "scale=1; $current_test * 100 / $total_tests" | bc)
        
        # Configurar schedule para OpenMP
        export OMP_SCHEDULE="$schedule"
        export OMP_NUM_THREADS="$THREADS"
        
        echo -ne "  [$porcentaje%] $schedule (T=$THREADS)... "
        
        # Ejecutar main-paralelo con método específico
        ./bin/main-paralelo -f "$ARCHIVO" -p $MATCH $MISMATCH $GAP -r $REPETICIONES -m "$METODO" -o "$ARCHIVO_SALIDA" > /dev/null 2>&1
        
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}OK${NC}"
        else
            echo -e "${RED}FAIL${NC}"
        fi
    done
    echo ""
done

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Benchmark finalizado${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo -e "${CYAN}Resultados guardados en: $ARCHIVO_SALIDA${NC}"
echo ""
echo -e "${YELLOW}Para comparar secuencial vs paralelo:${NC}"
echo "  python comparar_secuencial_paralelo.py $ARCHIVO_SALIDA"
echo ""

