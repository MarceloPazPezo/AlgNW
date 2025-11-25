#!/bin/bash

# Script para generar casos de prueba para notebook (poca RAM)
# Genera: 500, 1k, 2k, 5k, 10k
# Diseñado para ejecutar con Extrae/Paraver

# Colores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Directorio de salida
OUTPUT_DIR="datos"
BIN_DIR="bin"
GENERADOR="$BIN_DIR/main-gen-secuencia"

# Similitud por defecto
SIMILITUD=0.9

# Longitudes para notebook (casos pequeños)
declare -a LONGITUDES=(
    "500 500"
    "1k 1000"
    "2k 2000"
    "5k 5000"
    "10k 10000"
)

# Función para mostrar ayuda
mostrar_ayuda() {
    echo "Uso: $0 [opciones]"
    echo ""
    echo "Genera archivos FASTA para notebook (casos pequeños con poca RAM)."
    echo "Longitudes: 500, 1k, 2k, 5k, 10k"
    echo ""
    echo "Opciones:"
    echo "  -s, --similitud <valor>   Similitud objetivo (0.0 - 1.0) [default: 0.9]"
    echo "  -o, --output <directorio> Directorio de salida [default: datos/]"
    echo "  --solo-dna                Generar solo archivos de ADN"
    echo "  --solo-proteina           Generar solo archivos de proteínas"
    echo "  -h, --help                Muestra esta ayuda"
    echo ""
    echo "Ejemplo:"
    echo "  $0 -s 0.85"
    echo "  $0 -o datos_notebook/ --solo-dna"
}

# Parsear argumentos
GENERAR_DNA=true
GENERAR_PROTEINA=true

while [[ $# -gt 0 ]]; do
    case $1 in
        -s|--similitud)
            SIMILITUD="$2"
            shift 2
            ;;
        -o|--output)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --solo-dna)
            GENERAR_DNA=true
            GENERAR_PROTEINA=false
            shift
            ;;
        --solo-proteina)
            GENERAR_DNA=false
            GENERAR_PROTEINA=true
            shift
            ;;
        -h|--help)
            mostrar_ayuda
            exit 0
            ;;
        *)
            echo -e "${RED}Error: Opción desconocida '$1'${NC}"
            mostrar_ayuda
            exit 1
            ;;
    esac
done

# Validar similitud
if ! awk -v sim="$SIMILITUD" 'BEGIN { exit !(sim >= 0.0 && sim <= 1.0) }'; then
    echo -e "${RED}Error: La similitud debe estar entre 0.0 y 1.0${NC}"
    exit 1
fi

# Verificar que el generador existe
if [ ! -f "$GENERADOR" ]; then
    echo -e "${YELLOW}El generador no está compilado. Compilando...${NC}"
    if ! make bin/main-gen-secuencia 2>/dev/null; then
        echo -e "${RED}Error: No se pudo compilar el generador${NC}"
        exit 1
    fi
fi

# Crear directorio de salida
mkdir -p "$OUTPUT_DIR"

# Función para generar secuencias
generar_secuencias() {
    local TIPO=$1
    local TIPO_NOMBRE=$2
    local PREFIJO=$3
    local CONTADOR=0
    
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}Generando ${TIPO_NOMBRE} (notebook)${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""
    
    for item in "${LONGITUDES[@]}"; do
        NOMBRE=$(echo $item | cut -d' ' -f1)
        LONGITUD=$(echo $item | cut -d' ' -f2)
        
        ARCHIVO_SIN_EXTENSION="$OUTPUT_DIR/${PREFIJO}_${NOMBRE}"
        ARCHIVO_FINAL="${ARCHIVO_SIN_EXTENSION}.fasta"
        
        echo -e "${YELLOW}Generando: ${PREFIJO}_${NOMBRE}.fasta (${LONGITUD} caracteres)...${NC}"
        
        if "$GENERADOR" -t "$TIPO" -l "$LONGITUD" -s "$SIMILITUD" -o "$ARCHIVO_SIN_EXTENSION" 2>/dev/null; then
            echo -e "${GREEN}✓ Generado: $ARCHIVO_FINAL${NC}"
            ((CONTADOR++))
        else
            echo -e "${RED}✗ Error al generar: $ARCHIVO_FINAL${NC}"
        fi
        echo ""
    done
    
    echo -e "${BLUE}✓ ${TIPO_NOMBRE}: $CONTADOR/${#LONGITUDES[@]} archivos generados${NC}"
    echo ""
    
    return $CONTADOR
}

# Inicio
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Generador de Casos para Notebook${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Directorio de salida: $OUTPUT_DIR"
echo "Similitud objetivo: $SIMILITUD"
echo "Longitudes: 500, 1k, 2k, 5k, 10k"
echo ""
echo -e "${YELLOW}Nota: Estos casos están diseñados para notebooks con poca RAM${NC}"
echo -e "${YELLOW}       y para ejecutar con Extrae/Paraver${NC}"
echo ""

TOTAL=0

# Generar ADN
if [ "$GENERAR_DNA" = true ]; then
    generar_secuencias "dna" "ADN" "dna"
    TOTAL=$((TOTAL + $?))
fi

# Generar proteínas
if [ "$GENERAR_PROTEINA" = true ]; then
    generar_secuencias "proteina" "Proteínas" "protein"
    TOTAL=$((TOTAL + $?))
fi

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Total: $TOTAL archivos generados${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Para ejecutar con Extrae/Paraver:"
echo "  ./ejecutar_extrae.sh $OUTPUT_DIR/dna_500.fasta antidiagonal_static 1"
echo "  paraver traces/nw_trace_*.prv"

