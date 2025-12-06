#!/bin/bash

# Script para ejecutar benchmark completo: secuencial y paralelo
# Itera sobre diferentes tamaños, threads y schedules
#
# Uso:
#   ./ejecutar-benchmark.sh [opciones]
#
# Opciones:
#   -d, --datos DIR         Directorio con archivos FASTA [default: datos]
#   -o, --output DIR        Directorio de salida para resultados [default: resultados]
#   -f, --file FILE         Nombre del archivo CSV de salida [default: resultados_completo.csv]
#   -r, --repeticiones N    Número de repeticiones para todos los tamaños [default: según tamaño]
#   -h, --help             Mostrar esta ayuda
#
# Ejemplos:
#   ./ejecutar-benchmark.sh
#   ./ejecutar-benchmark.sh -o mis_resultados -f benchmark.csv
#   ./ejecutar-benchmark.sh -d datos -o resultados -r 10

# Colores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Valores por defecto
DATOS_DIR="datos"
RESULTADOS_DIR="resultados"
ARCHIVO_SALIDA_CSV="resultados_completo.csv"
REPETICIONES_FORZADAS=""  # Vacío significa usar la función obtener_repeticiones()

# Parsear argumentos de línea de comandos
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--datos)
            DATOS_DIR="$2"
            shift 2
            ;;
        -o|--output)
            RESULTADOS_DIR="$2"
            shift 2
            ;;
        -f|--file)
            ARCHIVO_SALIDA_CSV="$2"
            shift 2
            ;;
        -r|--repeticiones)
            REPETICIONES_FORZADAS="$2"
            shift 2
            ;;
        -h|--help)
            echo "Uso: $0 [opciones]"
            echo ""
            echo "Opciones:"
            echo "  -d, --datos DIR         Directorio con archivos FASTA [default: datos]"
            echo "  -o, --output DIR        Directorio de salida para resultados [default: resultados]"
            echo "  -f, --file FILE         Nombre del archivo CSV de salida [default: resultados_completo.csv]"
            echo "  -r, --repeticiones N    Número de repeticiones para todos los tamaños [default: según tamaño]"
            echo "  -h, --help             Mostrar esta ayuda"
            echo ""
            echo "Ejemplos:"
            echo "  $0"
            echo "  $0 -o mis_resultados -f benchmark.csv"
            echo "  $0 -d datos -o resultados -r 10"
            exit 0
            ;;
        *)
            echo -e "${RED}Error: Opción desconocida: $1${NC}"
            echo "Usa -h o --help para ver la ayuda"
            exit 1
            ;;
    esac
done

# Construir ruta completa del archivo de salida
ARCHIVO_SALIDA="$RESULTADOS_DIR/$ARCHIVO_SALIDA_CSV"

# Tamaños en potencias de 2
declare -a CASOS=("128" "256" "512" "1k" "2k" "4k" "8k" "16k")

# Hilos a probar (NO empezar desde 1, no tiene sentido para paralelo)
declare -a HILOS=(2 4 6 8)

# Configuraciones "METODO:SCHEDULE"
declare -a CONFIGURACIONES=(
    "antidiagonal:static"
    "antidiagonal:static,1"
    "antidiagonal:dynamic"
    "antidiagonal:guided"
    "bloques:static"
    "bloques:static,1"
    "bloques:dynamic"
    "bloques:guided"
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
echo -e "${CYAN}Configuración:${NC}"
echo "  Directorio de datos: $DATOS_DIR"
echo "  Directorio de resultados: $RESULTADOS_DIR"
echo "  Archivo de salida: $ARCHIVO_SALIDA"
if [ -n "$REPETICIONES_FORZADAS" ]; then
    echo -e "  ${YELLOW}Repeticiones: $REPETICIONES_FORZADAS (forzado para todos los tamaños)${NC}"
else
    echo "  Repeticiones: según tamaño (128-512: 10, 1k-4k: 5, 8k-16k: 3)"
fi
echo "  Tamaños: ${CASOS[*]}"
echo "  Threads: ${HILOS[*]}"
echo "  Parámetros: match=$MATCH, mismatch=$MISMATCH, gap=$GAP"
echo ""

# Función para obtener número de repeticiones según tamaño
obtener_repeticiones() {
    local caso=$1
    
    # Si se especificó un número forzado, usarlo para todos
    if [ -n "$REPETICIONES_FORZADAS" ]; then
        echo "$REPETICIONES_FORZADAS"
        return
    fi
    
    # Valores por defecto según tamaño
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
    
    # 1. Ejecutar SECUENCIAL (baseline) - N repeticiones
    # NOTA: main-secuencial no acepta flags, solo se ejecuta directamente
    echo -e "${YELLOW}>>> Ejecutando SECUENCIAL ($REPS repeticiones)...${NC}"
    for ((r=1; r<=$REPS; r++)); do
        echo -ne "   Repetición $r/$REPS... "
        ./bin/main-secuencial -f "$ARCHIVO" -p $MATCH $MISMATCH $GAP -o "$ARCHIVO_SALIDA" > /dev/null 2>&1
        
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}OK${NC}"
        else
            echo -e "${RED}FAIL${NC}"
        fi
    done
    
    # 2. Ejecutar PARALELOS con diferentes configuraciones - N repeticiones
    for config_str in "${CONFIGURACIONES[@]}"; do
        IFS=':' read -r metodo schedule <<< "$config_str"
        
        # Configurar schedule para OpenMP
        export OMP_SCHEDULE="$schedule"
        
        echo -e "${YELLOW}>>> Método: $metodo, Schedule: $schedule${NC}"
        
        # Iterar sobre número de threads
        for t in "${HILOS[@]}"; do
            export OMP_NUM_THREADS=$t
            
            echo -e "   Threads: $t (${REPS} repeticiones)..."
            
            # Ejecutar main-paralelo con método específico - N repeticiones
            # Convertir nombre del método a flag: antidiagonal -> -a, bloques -> -b
            case "$metodo" in
                "antidiagonal")
                    METODO_FLAG="-a"
                    ;;
                "bloques")
                    METODO_FLAG="-b"
                    ;;
                *)
                    METODO_FLAG="-m $metodo"  # Fallback (no debería ocurrir)
                    ;;
            esac
            
            for ((r=1; r<=$REPS; r++)); do
                echo -ne "      Rep $r/$REPS... "
                ./bin/main-paralelo -f "$ARCHIVO" -p $MATCH $MISMATCH $GAP $METODO_FLAG -o "$ARCHIVO_SALIDA" > /dev/null 2>&1
            
                if [ $? -eq 0 ]; then
                    echo -e "${GREEN}OK${NC}"
                else
                    echo -e "${RED}FAIL${NC}"
                fi
            done
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

