#!/bin/bash

# Script para ejecutar benchmarks en servidor con casos grandes
# Ejecuta todos los casos: 20k, 30k, 50k, 75k, 100k, 125k, 150k
# ⚠️ Requiere MUCHA memoria RAM

# Colores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuración
DATOS_DIR="${1:-datos}"
REPETICIONES="${2:-5}"
MATCH=2
MISMATCH=0
GAP=-2

# Casos a ejecutar
declare -a CASOS=("20k" "30k" "50k" "75k" "100k" "125k" "150k")

# Función para calcular memoria
calcular_memoria() {
    local caso=$1
    local longitud=${caso%k}
    longitud=$((longitud * 1000))
    local memoria_bytes=$(( (longitud + 1) * (longitud + 1) * 4 * 2 ))
    local memoria_gb=$(awk "BEGIN {printf \"%.2f\", $memoria_bytes / 1024 / 1024 / 1024}")
    echo "$memoria_gb"
}

echo -e "${GREEN}=== Benchmark Servidor ===${NC}"
echo "Directorio de datos: $DATOS_DIR"
echo "Repeticiones: $REPETICIONES"
echo ""
echo -e "${RED}⚠️  ADVERTENCIA: Estos casos requieren MUCHA memoria RAM${NC}"
echo ""

# Mostrar memoria estimada
echo "Memoria estimada por caso:"
for caso in "${CASOS[@]}"; do
    memoria=$(calcular_memoria $caso)
    echo "  ${caso}: ~${memoria} GB"
done
echo ""

read -p "¿Continuar? (s/n): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[SsYy]$ ]]; then
    echo "Cancelado."
    exit 0
fi
echo ""

# Verificar que el benchmark está compilado
if [ ! -f "bin/main-benchmark" ]; then
    echo -e "${YELLOW}Compilando benchmark...${NC}"
    make bin/main-benchmark
    if [ $? -ne 0 ]; then
        echo -e "${RED}Error al compilar${NC}"
        exit 1
    fi
fi

# Crear directorio de resultados
RESULTADOS_DIR="resultados_servidor"
mkdir -p "$RESULTADOS_DIR"

# Ejecutar cada caso
for caso in "${CASOS[@]}"; do
    for tipo in "dna" "protein"; do
        ARCHIVO="$DATOS_DIR/${tipo}_${caso}.fasta"
        
        if [ ! -f "$ARCHIVO" ]; then
            echo -e "${YELLOW}⚠ Archivo no encontrado: $ARCHIVO${NC}"
            continue
        fi
        
        memoria=$(calcular_memoria $caso)
        
        echo -e "${BLUE}========================================${NC}"
        echo -e "${BLUE}Ejecutando: ${tipo}_${caso} (~${memoria} GB RAM)${NC}"
        echo -e "${BLUE}========================================${NC}"
        
        RESULTADO_CSV="$RESULTADOS_DIR/benchmark_${tipo}_${caso}_$(date +%Y%m%d_%H%M%S).csv"
        
        ./bin/main-benchmark -f "$ARCHIVO" -p $MATCH $MISMATCH $GAP -r $REPETICIONES -o "$RESULTADO_CSV"
        
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}✓ Completado: $RESULTADO_CSV${NC}"
        else
            echo -e "${RED}✗ Error en: $ARCHIVO${NC}"
            echo -e "${YELLOW}Posible causa: Memoria insuficiente${NC}"
        fi
        echo ""
    done
done

echo -e "${GREEN}=== Benchmark completado ===${NC}"
echo "Resultados en: $RESULTADOS_DIR"

