#!/bin/bash

# Script para ejecutar benchmark secuencial y generar análisis de fases
# Genera gráficos mostrando qué fase ocupa más tiempo

# Colores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configuración
DATOS_DIR="${1:-datos}"
REPETICIONES="${2:-10}"
MATCH=2
MISMATCH=0
GAP=-2

echo -e "${GREEN}=== Benchmark Secuencial - Análisis de Fases ===${NC}"
echo "Directorio de datos: $DATOS_DIR"
echo "Repeticiones por archivo: $REPETICIONES"
echo ""

# Verificar compilación
if [ ! -f "bin/main-benchmark" ]; then
    echo -e "${YELLOW}Compilando benchmark...${NC}"
    make bin/main-benchmark
    if [ $? -ne 0 ]; then
        echo -e "${RED}Error al compilar${NC}"
        exit 1
    fi
fi

# Directorio para resultados
RESULTADOS_DIR="resultados_benchmark"
mkdir -p "$RESULTADOS_DIR"

# Archivos a procesar (puedes ajustar según tus datos)
declare -a CASOS=("50" "100" "250" "500" "1k" "2k" "5k" "10k" "15k" "20k")

echo -e "${BLUE}Ejecutando benchmark secuencial para análisis de fases...${NC}"
echo ""

# Ejecutar benchmark secuencial para cada archivo
for caso in "${CASOS[@]}"; do
    for tipo in "dna" "protein"; do
        ARCHIVO="$DATOS_DIR/${tipo}_${caso}.fasta"
        
        if [ ! -f "$ARCHIVO" ]; then
            echo -e "${YELLOW}⚠ Archivo no encontrado: $ARCHIVO${NC}"
            continue
        fi

        echo -e "${CYAN}Procesando: ${tipo}_${caso}${NC}"
        
        RESULTADO_CSV="$RESULTADOS_DIR/${tipo}_${caso}_secuencial.csv"
        
        # Ejecutar método secuencial
        ./bin/main-benchmark -f "$ARCHIVO" -p $MATCH $MISMATCH $GAP -r $REPETICIONES -m "secuencial" -o "$RESULTADO_CSV" > /dev/null 2>&1
        
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}  ✓ OK${NC}"
        else
            echo -e "${RED}  ✗ Falló${NC}"
        fi
    done
done

echo ""
echo -e "${GREEN}=== Calculando promedios ===${NC}"

# Verificar si existe el script de promedios
if [ -f "calcular_promedios_benchmark.py" ]; then
    echo "Calculando promedios de ejecuciones..."
    python3 calcular_promedios_benchmark.py -d "$RESULTADOS_DIR"
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Promedios calculados${NC}"
    else
        echo -e "${RED}✗ Error al calcular promedios${NC}"
    fi
else
    echo -e "${YELLOW}⚠ Script calcular_promedios_benchmark.py no encontrado${NC}"
    echo "Los archivos CSV individuales están en: $RESULTADOS_DIR"
fi

echo ""
echo -e "${GREEN}=== Generando análisis y gráficos ===${NC}"

# Verificar si existe el script de análisis
if [ -f "analizar_benchmark.py" ]; then
    echo "Ejecutando análisis de benchmark..."
    python3 analizar_benchmark.py -d "$RESULTADOS_DIR" -o "graficos_benchmark"
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Análisis completado${NC}"
        echo "Gráficos guardados en: graficos_benchmark/"
        echo ""
        echo "Gráficos generados:"
        echo "  - porcentajes_por_fase_apilado.png"
        echo "  - tiempos_absolutos_por_fase.png"
        echo "  - analisis_comparativo_fases.png"
        echo "  - porcentaje_llenado_por_archivo_promedio.png"
        echo "  - tiempo_total_vs_longitud.png"
    else
        echo -e "${RED}✗ Error al generar análisis${NC}"
    fi
else
    echo -e "${YELLOW}⚠ Script analizar_benchmark.py no encontrado${NC}"
    echo "Ejecuta manualmente: python3 analizar_benchmark.py -d $RESULTADOS_DIR"
fi

echo ""
echo -e "${GREEN}=== Benchmark Secuencial Finalizado ===${NC}"
echo "Resultados guardados en: $RESULTADOS_DIR"
echo "Gráficos guardados en: graficos_benchmark/"

