#!/bin/bash

# Script para generar casos de prueba DNA para benchmarking
# Genera longitudes en potencias de 2: 128, 256, 512, 1k, 2k, 4k, 8k, 16k
# Versión simplificada solo para DNA

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

# Longitudes para generar (potencias de 2)
declare -a LONGITUDES=(
    "128 128"
    "256 256"
    "512 512"
    "1k 1024"
    "2k 2048"
    "4k 4096"
    "8k 8192"
    "16k 16384"
)

# Función para mostrar ayuda
mostrar_ayuda() {
    echo "Uso: $0 [opciones]"
    echo ""
    echo "Genera archivos FASTA DNA para benchmarking."
    echo "Longitudes (potencias de 2): 128, 256, 512, 1k, 2k, 4k, 8k, 16k"
    echo ""
    echo "Opciones:"
    echo "  -s, --similitud <valor>   Similitud objetivo (0.0 - 1.0) [default: 0.9]"
    echo "  -o, --output <directorio> Directorio de salida [default: datos/]"
    echo "  -h, --help                Muestra esta ayuda"
    echo ""
    echo "Ejemplo:"
    echo "  $0 -s 0.85"
    echo "  $0 -o datos_benchmark/"
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
    
    # Intentar compilar con make
    if command -v make &> /dev/null && [ -f "Makefile" ]; then
        make "$GENERADOR" 2>/dev/null
    else
        # Compilar directamente
        g++ -O3 -Wall -std=c++11 -Isrcv2 -o "$GENERADOR" \
            main-gen-secuencia.cpp generador_secuencias.cpp 2>/dev/null
    fi
    
    if [ ! -f "$GENERADOR" ]; then
        echo -e "${RED}Error: No se pudo compilar el generador${NC}"
        echo "Ejecuta: make bin/main-gen-secuencia"
        exit 1
    fi
fi

# Crear directorio de salida
mkdir -p "$OUTPUT_DIR"

# Función para generar secuencias DNA
generar_secuencias_dna() {
    local CONTADOR=0
    
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}Generando secuencias DNA${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""
    
    for item in "${LONGITUDES[@]}"; do
        NOMBRE=$(echo $item | cut -d' ' -f1)
        LONGITUD=$(echo $item | cut -d' ' -f2)
        
        ARCHIVO_SIN_EXTENSION="$OUTPUT_DIR/dna_${NOMBRE}"
        ARCHIVO_FINAL="${ARCHIVO_SIN_EXTENSION}.fasta"
        
        echo -e "${YELLOW}Generando: dna_${NOMBRE}.fasta (${LONGITUD} caracteres)...${NC}"
        
        if "$GENERADOR" -l "$LONGITUD" -s "$SIMILITUD" -o "$ARCHIVO_SIN_EXTENSION" 2>/dev/null; then
            echo -e "${GREEN}✓ Generado: $ARCHIVO_FINAL${NC}"
            ((CONTADOR++))
        else
            echo -e "${RED}✗ Error al generar: $ARCHIVO_FINAL${NC}"
        fi
        echo ""
    done
    
    echo -e "${BLUE}✓ DNA: $CONTADOR/${#LONGITUDES[@]} archivos generados${NC}"
    echo ""
    
    return $CONTADOR
}

# Inicio
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Generador de Datos DNA${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Directorio de salida: $OUTPUT_DIR"
echo "Similitud objetivo: $SIMILITUD"
echo "Longitudes (potencias de 2): 128, 256, 512, 1k, 2k, 4k, 8k, 16k"
echo ""

TOTAL=0

# Generar DNA
generar_secuencias_dna
TOTAL=$?

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Total: $TOTAL archivos generados${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Archivos generados en: $OUTPUT_DIR/"
echo ""
echo "Para ejecutar benchmark:"
echo "  export OMP_NUM_THREADS=8"
echo "  export OMP_SCHEDULE=\"dynamic,1\""
echo "  ./bin/main-paralelo -f $OUTPUT_DIR/dna_1k.fasta -p 2 -1 -2 -r 5"

