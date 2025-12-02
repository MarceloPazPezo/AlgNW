#!/bin/bash

# Script para ejecutar benchmark completo: secuencial y paralelo
# Itera sobre diferentes tamaños, threads y schedules

# Colores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuración
DATOS_DIR="datos"
RESULTADOS_DIR="resultados"
ARCHIVO_SALIDA="$RESULTADOS_DIR/resultados_completo.csv"

# Tamaños en potencias de 2
declare -a CASOS=("128" "256" "512" "1k" "2k" "4k" "8k" "16k" "32k")

# Hilos a probar (NO empezar desde 1, no tiene sentido para paralelo)
declare -a HILOS=(2 4 6 8)

# Configuraciones "METODO:SCHEDULE"
declare -a CONFIGURACIONES=(
    "antidiagonal:static"
    "antidiagonal:static,1"
    "antidiagonal:dynamic,1"
    "antidiagonal:guided,1"
    "bloques:static"
    "bloques:static,1"
    "bloques:dynamic,1"
    "bloques:guided,1"
)

# Parámetros de puntuación
MATCH=2
MISMATCH=-1
GAP=-2

# Crear directorios
mkdir -p "$RESULTADOS_DIR"

# Limpiar archivo de salida anterior
rm -f "$ARCHIVO_SALIDA"

# Verificar que los programas existen
if [ ! -f "bin/main-secuencial" ]; then
    echo -e "${RED}Error: bin/main-secuencial no encontrado${NC}"
    echo "Ejecuta: make bin/main-secuencial"
    exit 1
fi

if [ ! -f "bin/main-paralelo" ]; then
    echo -e "${RED}Error: bin/main-paralelo no encontrado${NC}"
    echo "Ejecuta: make bin/main-paralelo"
    exit 1
fi

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}BENCHMARK COMPLETO${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Directorio de datos: $DATOS_DIR"
echo "Directorio de resultados: $RESULTADOS_DIR"
echo "Archivo de salida: $ARCHIVO_SALIDA"
echo "Tamaños: ${CASOS[*]}"
echo "Threads: ${HILOS[*]}"
echo "Parámetros: match=$MATCH, mismatch=$MISMATCH, gap=$GAP"
echo ""

# Función para obtener número de repeticiones según tamaño
obtener_repeticiones() {
    local caso=$1
    case "$caso" in
        "128"|"256"|"512")
            echo 10
            ;;
        "1k"|"2k"|"4k")
            echo 5
            ;;
        "8k"|"16k")
            echo 3
            ;;
        "32k")
            echo 2
            ;;
        *)
            echo 3
            ;;
    esac
}

# Función para convertir nombre de caso a longitud numérica
caso_a_longitud() {
    local caso=$1
    case "$caso" in
        "128") echo 128 ;;
        "256") echo 256 ;;
        "512") echo 512 ;;
        "1k") echo 1024 ;;
        "2k") echo 2048 ;;
        "4k") echo 4096 ;;
        "8k") echo 8192 ;;
        "16k") echo 16384 ;;
        "32k") echo 32768 ;;
        *) echo "$caso" ;;
    esac
}

# Iterar sobre todos los casos
for caso in "${CASOS[@]}"; do
    ARCHIVO="$DATOS_DIR/dna_${caso}.fasta"
    
    if [ ! -f "$ARCHIVO" ]; then
        echo -e "${YELLOW}Saltando $caso (archivo no encontrado: $ARCHIVO)${NC}"
        continue
    fi
    
    REPS=$(obtener_repeticiones "$caso")
    
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}Dataset: dna_${caso}.fasta (Reps: $REPS)${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    # 1. Ejecutar SECUENCIAL (baseline)
    echo -e "${YELLOW}>>> Ejecutando SECUENCIAL...${NC}"
    ./bin/main-secuencial -f "$ARCHIVO" -p $MATCH $MISMATCH $GAP -o "$ARCHIVO_SALIDA" > /dev/null 2>&1
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Secuencial completado${NC}"
    else
        echo -e "${RED}✗ Error en secuencial${NC}"
    fi
    
    # 2. Ejecutar PARALELOS con diferentes configuraciones
    for config_str in "${CONFIGURACIONES[@]}"; do
        IFS=':' read -r metodo schedule <<< "$config_str"
        
        # Configurar schedule para OpenMP
        export OMP_SCHEDULE="$schedule"
        
        echo -e "${YELLOW}>>> Método: $metodo, Schedule: $schedule${NC}"
        
        # Iterar sobre número de threads
        for t in "${HILOS[@]}"; do
            export OMP_NUM_THREADS=$t
            
            echo -ne "   Threads: $t... "
            
            # Ejecutar main-paralelo con método específico
            ./bin/main-paralelo -f "$ARCHIVO" -p $MATCH $MISMATCH $GAP -r "$REPS" -m "$metodo" -o "$ARCHIVO_SALIDA" > /dev/null 2>&1
            
            if [ $? -eq 0 ]; then
                echo -e "${GREEN}OK${NC}"
            else
                echo -e "${RED}FAIL${NC}"
            fi
        done
        echo ""
    done
    
    echo ""
done

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Benchmark finalizado${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Resultados guardados en: $ARCHIVO_SALIDA"
echo ""
echo "Para analizar resultados:"
echo "  cat $ARCHIVO_SALIDA | grep secuencial"
echo "  cat $ARCHIVO_SALIDA | grep antidiagonal"
echo "  cat $ARCHIVO_SALIDA | grep bloques"

