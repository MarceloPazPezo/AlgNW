#!/bin/bash

# Script para generar casos de prueba para servidor (mucha RAM)
# Genera: 20k, 30k, 50k, 75k, 100k, 125k, 150k
# Diseñado para análisis completo en servidor con mucha memoria

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

# Longitudes para servidor (casos grandes)
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
    "125k 125000"
    "150k 150000"
)

# Función para calcular memoria estimada
calcular_memoria() {
    local longitud=$1
    # Memoria aproximada: (n+1) * (m+1) * sizeof(int) * 2 (matriz + overhead)
    # Asumiendo secuencias de igual longitud
    local memoria_bytes=$(( (longitud + 1) * (longitud + 1) * 4 * 2 ))
    local memoria_mb=$(( memoria_bytes / 1024 / 1024 ))
    local memoria_gb=$(awk "BEGIN {printf \"%.2f\", $memoria_bytes / 1024 / 1024 / 1024}")
    echo "$memoria_mb $memoria_gb"
}

# Función para mostrar ayuda
mostrar_ayuda() {
    echo "Uso: $0 [opciones]"
    echo ""
    echo "Genera archivos FASTA para servidor (casos grandes con mucha RAM)."
    echo "Longitudes: 20k, 30k, 50k, 75k, 100k, 125k, 150k"
    echo ""
    echo "⚠️  ADVERTENCIA: Estos casos requieren MUCHA memoria RAM"
    echo ""
    echo "Opciones:"
    echo "  -s, --similitud <valor>   Similitud objetivo (0.0 - 1.0) [default: 0.9]"
    echo "  -o, --output <directorio> Directorio de salida [default: datos/]"
    echo "  --solo-dna                Generar solo archivos de ADN"
    echo "  --solo-proteina           Generar solo archivos de proteínas"
    echo "  --skip-warning            Omitir advertencia de memoria"
    echo "  -h, --help                Muestra esta ayuda"
    echo ""
    echo "Ejemplo:"
    echo "  $0 -s 0.85"
    echo "  $0 -o datos_servidor/ --solo-dna"
}

# Parsear argumentos
GENERAR_DNA=true
GENERAR_PROTEINA=true
SKIP_WARNING=false

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
        --skip-warning)
            SKIP_WARNING=true
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

# Advertencia de memoria
if [ "$SKIP_WARNING" = false ]; then
    echo -e "${RED}========================================${NC}"
    echo -e "${RED}⚠️  ADVERTENCIA DE MEMORIA${NC}"
    echo -e "${RED}========================================${NC}"
    echo ""
    echo "Estos casos requieren MUCHA memoria RAM:"
    echo ""
    
    for item in "${LONGITUDES[@]}"; do
        LONGITUD=$(echo $item | cut -d' ' -f2)
        read memoria_mb memoria_gb <<< $(calcular_memoria $LONGITUD)
        if (( memoria_gb > 1 )); then
            echo "  ${LONGITUD}: ~${memoria_gb} GB"
        else
            echo "  ${LONGITUD}: ~${memoria_mb} MB"
        fi
    done
    
    echo ""
    echo -e "${YELLOW}El caso más grande (150k) requiere aproximadamente 36 GB de RAM${NC}"
    echo ""
    read -p "¿Continuar? (s/n): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[SsYy]$ ]]; then
        echo "Cancelado."
        exit 0
    fi
    echo ""
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
    echo -e "${BLUE}Generando ${TIPO_NOMBRE} (servidor)${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""
    
    for item in "${LONGITUDES[@]}"; do
        NOMBRE=$(echo $item | cut -d' ' -f1)
        LONGITUD=$(echo $item | cut -d' ' -f2)
        
        ARCHIVO_SIN_EXTENSION="$OUTPUT_DIR/${PREFIJO}_${NOMBRE}"
        ARCHIVO_FINAL="${ARCHIVO_SIN_EXTENSION}.fasta"
        
        # Calcular memoria estimada
        read memoria_mb memoria_gb <<< $(calcular_memoria $LONGITUD)
        if (( $(echo "$memoria_gb > 1" | bc -l 2>/dev/null || echo 0) )); then
            MEM_INFO="${memoria_gb} GB"
        else
            MEM_INFO="${memoria_mb} MB"
        fi
        
        echo -e "${YELLOW}Generando: ${PREFIJO}_${NOMBRE}.fasta (${LONGITUD} caracteres, ~${MEM_INFO} RAM)...${NC}"
        
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
echo -e "${GREEN}Generador de Casos para Servidor${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Directorio de salida: $OUTPUT_DIR"
echo "Similitud objetivo: $SIMILITUD"
echo "Longitudes: 20k, 30k, 50k, 75k, 100k, 125k, 150k"
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
echo "Para ejecutar benchmark completo:"
echo "  ./ejecutar.sh $OUTPUT_DIR/dna_20k.fasta 5"
echo "  ./bin/main-benchmark -f $OUTPUT_DIR/dna_20k.fasta -p 2 0 -2 -r 5 -o resultados_servidor.csv"

