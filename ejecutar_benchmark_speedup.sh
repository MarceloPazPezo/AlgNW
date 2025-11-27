#!/bin/bash

# Script para ejecutar benchmark de Speedup (Escalabilidad Fuerte)
# Varía la cantidad de hilos OMP para calcular eficiencia

# Colores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configuración
DATOS_DIR="${1:-datos}"
REPETICIONES="${2:-3}" # Menos repeticiones para speedup suele ser suficiente
MATCH=2
MISMATCH=0
GAP=-2

# === CONFIGURACIÓN DE HILOS ===
# Tu CPU tiene 4 núcleos físicos y 8 lógicos.
# 1: Base secuencial (Speedup = 1)
# 2: Paralelismo básico
# 4: Uso completo de núcleos físicos (Suele dar la mejor eficiencia)
# 8: Uso de Hyperthreading (Puede mejorar o empeorar dependiendo de la memoria)
declare -a HILOS=(1 2 4 8)

# Casos a ejecutar (Corregí la coma que tenías en el array original)
declare -a CASOS=("500" "1k" "2k" "5k" "10k" "15k" "20k")

echo -e "${GREEN}=== Benchmark de Speedup / Escalabilidad ===${NC}"
echo "Directorio de datos: $DATOS_DIR"
echo "Hilos a probar: ${HILOS[*]}"
echo "Repeticiones por configuración: $REPETICIONES"
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

# Directorio específico para resultados de speedup
RESULTADOS_DIR="resultados_speedup"
mkdir -p "$RESULTADOS_DIR"

# Métodos y schedules a probar
# Formato: "metodo:schedule" donde schedule se configura en OMP_SCHEDULE
declare -a CONFIGURACIONES=(
    "antidiagonal:static"
    "antidiagonal:dynamic,1"
    "antidiagonal:guided,1"
    "bloques:static"
    "bloques:dynamic,1"
    "bloques:guided,1"
)

# Loop Principal
for caso in "${CASOS[@]}"; do
    for tipo in "dna" "protein"; do
        ARCHIVO="$DATOS_DIR/${tipo}_${caso}.fasta"
        
        if [ ! -f "$ARCHIVO" ]; then
            echo -e "${YELLOW}⚠ Archivo no encontrado: $ARCHIVO${NC}"
            continue
        fi

        echo -e "${BLUE}----------------------------------------${NC}"
        echo -e "${BLUE}Dataset: ${tipo}_${caso}${NC}"
        echo -e "${BLUE}----------------------------------------${NC}"

        for config in "${CONFIGURACIONES[@]}"; do
            # Separar metodo y schedule
            IFS=':' read -r metodo schedule <<< "$config"
            
            echo -e "${CYAN}Metodo: $metodo (schedule: $schedule)${NC}"
            
            # Configurar OMP_SCHEDULE
            export OMP_SCHEDULE="$schedule"
            
            # Loop de Hilos
            for t in "${HILOS[@]}"; do
                
                export OMP_NUM_THREADS=$t
                
                # Nombre del archivo incluye metodo, schedule y hilos
                # Convertir schedule a formato de nombre de archivo (reemplazar comas y espacios)
                schedule_clean=$(echo "$schedule" | tr ',' '_' | tr ' ' '_')
                RESULTADO_CSV="$RESULTADOS_DIR/speedup_${tipo}_${caso}_${metodo}_${schedule_clean}_th${t}.csv"
                
                echo -e "  Ejecutando con ${CYAN}$t Hilos${NC} (OMP_SCHEDULE=$schedule)..."
                
                # Ejecución con selección de método (-m)
                ./bin/main-benchmark -f "$ARCHIVO" -p $MATCH $MISMATCH $GAP -r $REPETICIONES -m "$metodo" -o "$RESULTADO_CSV" > /dev/null 2>&1
                
                if [ $? -eq 0 ]; then
                    echo -e "${GREEN}    ✓ OK${NC}"
                else
                    echo -e "${RED}    ✗ Falló${NC}"
                fi
            done
        done
        echo ""
    done
done

echo -e "${GREEN}=== Benchmark de Speedup Finalizado ===${NC}"
echo "Para calcular el speedup usa la fórmula: S = Tiempo(1 hilo) / Tiempo(N hilos)"
echo "Resultados guardados en: $RESULTADOS_DIR"