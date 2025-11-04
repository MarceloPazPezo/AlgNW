#!/bin/bash

# Script para generar datos de ADN y proteínas para benchmarking
# Genera secuencias biológicas con diferentes longitudes

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

# Similitud por defecto (puede cambiarse con -s)
SIMILITUD=0.9

# Tipos de secuencias a generar (dna, proteina)
GENERAR_DNA=true
GENERAR_PROTEINA=true

# Función para mostrar ayuda
mostrar_ayuda() {
    echo "Uso: $0 [opciones]"
    echo ""
    echo "Genera archivos FASTA de ADN y proteínas con diferentes longitudes."
    echo ""
    echo "Opciones:"
    echo "  -s, --similitud <valor>   Similitud objetivo (0.0 - 1.0) [default: 0.9]"
    echo "  -o, --output <directorio> Directorio de salida [default: datos/]"
    echo "  --solo-dna                Generar solo archivos de ADN"
    echo "  --solo-proteina           Generar solo archivos de proteínas"
    echo "  -h, --help                Muestra esta ayuda"
    echo ""
    echo "Longitudes generadas:"
    echo "  500, 1k (1000), 2k (2000), 5k (5000), 10k (10000), 20k (20000), 30k (30000), 50k (50000), 75k (75000), 100k (100000)"
    echo ""
    echo "Ejemplo:"
    echo "  $0 -s 0.85"
    echo "  $0 -o data/ -s 0.95"
    echo "  $0 --solo-dna"
    echo "  $0 --solo-proteina"
}

# Parsear argumentos
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

# Validar similitud (usando awk para comparación de floats)
if ! awk -v sim="$SIMILITUD" 'BEGIN { exit !(sim >= 0.0 && sim <= 1.0) }'; then
    echo -e "${RED}Error: La similitud debe estar entre 0.0 y 1.0${NC}"
    exit 1
fi

# Verificar que el generador existe
if [ ! -f "$GENERADOR" ]; then
    echo -e "${YELLOW}El generador no está compilado. Compilando...${NC}"
    if ! make; then
        echo -e "${RED}Error: No se pudo compilar el generador${NC}"
        exit 1
    fi
fi

# Crear directorio de salida si no existe
mkdir -p "$OUTPUT_DIR"

# Array con las longitudes (en formato: nombre longitud)
declare -a LONGITUDES=(
    "500 500"
    "1k 1000"
    "2k 2000"
    "5k 5000"
    "10k 10000"
    "20k 20000"
    "30k 30000"
    "50k 50000"
    "75k 75000"
    "100k 100000"
)

# Función para generar secuencias de un tipo específico
generar_secuencias() {
    local TIPO=$1
    local TIPO_NOMBRE=$2
    local PREFIJO=$3
    local CONTADOR_TIPO=0
    
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}Generando datos de ${TIPO_NOMBRE}${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""
    
    # Generar cada archivo
    for item in "${LONGITUDES[@]}"; do
        # Separar nombre y longitud
        NOMBRE=$(echo $item | cut -d' ' -f1)
        LONGITUD=$(echo $item | cut -d' ' -f2)
        
        # No incluir .fasta porque el generador lo agrega automáticamente
        ARCHIVO_SIN_EXTENSION="$OUTPUT_DIR/${PREFIJO}_${NOMBRE}"
        ARCHIVO_FINAL="${ARCHIVO_SIN_EXTENSION}.fasta"
        
        echo -e "${YELLOW}Generando: ${PREFIJO}_${NOMBRE}.fasta (${LONGITUD} caracteres)...${NC}"
        
        # Ejecutar el generador (sin extensión .fasta)
        if "$GENERADOR" -t "$TIPO" -l "$LONGITUD" -s "$SIMILITUD" -o "$ARCHIVO_SIN_EXTENSION" 2>/dev/null; then
            echo -e "${GREEN}✓ Generado: $ARCHIVO_FINAL${NC}"
            ((CONTADOR_TIPO++))
        else
            echo -e "${RED}✗ Error al generar: $ARCHIVO_FINAL${NC}"
        fi
        echo ""
    done
    
    echo -e "${BLUE}✓ ${TIPO_NOMBRE}: $CONTADOR_TIPO/${#LONGITUDES[@]} archivos generados${NC}"
    echo ""
    
    return $CONTADOR_TIPO
}

# Contador total de archivos generados
TOTAL_CONTADOR=0

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Generador de Datos Biológicos${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Directorio de salida: $OUTPUT_DIR"
echo "Similitud objetivo: $SIMILITUD"
echo "Tipos a generar:"
if [ "$GENERAR_DNA" = true ]; then
    echo "  - ADN"
fi
if [ "$GENERAR_PROTEINA" = true ]; then
    echo "  - Proteínas"
fi
echo ""

# Generar ADN
if [ "$GENERAR_DNA" = true ]; then
    generar_secuencias "dna" "ADN" "dna"
    RESULTADO=$?
    TOTAL_CONTADOR=$((TOTAL_CONTADOR + RESULTADO))
fi

# Generar proteínas
if [ "$GENERAR_PROTEINA" = true ]; then
    generar_secuencias "proteina" "Proteínas" "protein"
    RESULTADO=$?
    TOTAL_CONTADOR=$((TOTAL_CONTADOR + RESULTADO))
fi

# Calcular total esperado
TOTAL_ESPERADO=0
if [ "$GENERAR_DNA" = true ]; then
    TOTAL_ESPERADO=$((TOTAL_ESPERADO + ${#LONGITUDES[@]}))
fi
if [ "$GENERAR_PROTEINA" = true ]; then
    TOTAL_ESPERADO=$((TOTAL_ESPERADO + ${#LONGITUDES[@]}))
fi

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}✓ Proceso completado${NC}"
echo -e "${GREEN}Archivos generados: $TOTAL_CONTADOR/$TOTAL_ESPERADO${NC}"
echo -e "${GREEN}Ubicación: $OUTPUT_DIR/${NC}"
echo -e "${GREEN}========================================${NC}"

