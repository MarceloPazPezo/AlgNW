#!/bin/bash

# Script para ejecutar main-secuencial múltiples veces y calcular promedios
# Genera estadísticas de tiempos y puntuación

# Colores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Variables
BIN_DIR="bin"
EJECUTABLE="$BIN_DIR/main-secuencial"
NUM_EJECUCIONES=10
ARCHIVO_FASTA=""
MATCH=""
MISMATCH=""
GAP=""
OUTPUT_CSV="resultado_promedio.csv"
TEMP_CSV="temp_ejecuciones.csv"
VERBOSE=false

# Función para leer longitudes de secuencias desde archivo FASTA
leer_longitudes_fasta() {
    local archivo=$1
    local len1=0
    local len2=0
    local secuencia_actual=""
    local contador=0
    local dentro_secuencia=false
    
    while IFS= read -r linea || [ -n "$linea" ]; do
        # Eliminar espacios y retornos de carro
        linea=$(echo "$linea" | tr -d '\r' | tr -d ' ')
        
        if [ -z "$linea" ]; then
            continue
        fi
        
        if [[ "$linea" =~ ^\> ]]; then
            # Es una cabecera
            if [ "$dentro_secuencia" = true ]; then
                # Guardar la secuencia anterior
                if [ $contador -eq 1 ]; then
                    len1=${#secuencia_actual}
                elif [ $contador -eq 2 ]; then
                    len2=${#secuencia_actual}
                fi
                secuencia_actual=""
            fi
            ((contador++))
            dentro_secuencia=true
        else
            # Es una línea de secuencia (puede estar en múltiples líneas)
            secuencia_actual+="$linea"
        fi
    done < "$archivo"
    
    # Guardar la última secuencia
    if [ "$dentro_secuencia" = true ] && [ -n "$secuencia_actual" ]; then
        if [ $contador -eq 1 ]; then
            len1=${#secuencia_actual}
        elif [ $contador -eq 2 ]; then
            len2=${#secuencia_actual}
        fi
    fi
    
    echo "$len1 $len2"
}

# Función para calcular memoria necesaria (en bytes)
calcular_memoria_necesaria() {
    local len1=$1
    local len2=$2
    
    # Matriz de alineamiento: (len1+1) x (len2+1)
    # Cada celda necesita:
    # - Entero para puntuación: 4 bytes
    # - Dirección para traceback: 4 bytes
    # - Overhead adicional: ~2 bytes
    # Total aproximado: 10 bytes por celda (con margen de seguridad)
    
    local celdas=$(( (len1 + 1) * (len2 + 1) ))
    local bytes=$(( celdas * 10 ))
    
    # Agregar margen de seguridad del 50% para otras estructuras
    bytes=$(( bytes * 3 / 2 ))
    
    echo $bytes
}

# Función para obtener RAM disponible (en bytes)
obtener_ram_disponible() {
    # Intentar diferentes métodos según el sistema
    if command -v free &> /dev/null; then
        # Linux: obtener memoria disponible en bytes
        free -b | awk '/^Mem:/ {print $7}'
    elif [ -f /proc/meminfo ]; then
        # Linux: leer desde /proc/meminfo
        awk '/MemAvailable:/ {print $2 * 1024}' /proc/meminfo
    elif command -v vm_stat &> /dev/null; then
        # macOS: calcular memoria libre
        vm_stat | awk '/Pages free:/ {free=$3} /Pages inactive:/ {inactive=$3} END {
            gsub(/\./, "", free)
            gsub(/\./, "", inactive)
            print (free + inactive) * 4096
        }'
    else
        # Valor por defecto conservador si no se puede determinar
        echo "0"
    fi
}

# Función para formatear bytes a formato legible
formatear_bytes() {
    local bytes=$1
    
    # Usar awk si bc no está disponible
    if command -v bc &> /dev/null; then
        if [ $bytes -ge 1099511627776 ]; then
            printf "%.2f TB" $(echo "scale=2; $bytes / 1099511627776" | bc)
        elif [ $bytes -ge 1073741824 ]; then
            printf "%.2f GB" $(echo "scale=2; $bytes / 1073741824" | bc)
        elif [ $bytes -ge 1048576 ]; then
            printf "%.2f MB" $(echo "scale=2; $bytes / 1048576" | bc)
        elif [ $bytes -ge 1024 ]; then
            printf "%.2f KB" $(echo "scale=2; $bytes / 1024" | bc)
        else
            printf "%d bytes" $bytes
        fi
    else
        # Fallback usando awk
        awk -v bytes=$bytes '
        BEGIN {
            if (bytes >= 1099511627776) printf "%.2f TB", bytes / 1099511627776
            else if (bytes >= 1073741824) printf "%.2f GB", bytes / 1073741824
            else if (bytes >= 1048576) printf "%.2f MB", bytes / 1048576
            else if (bytes >= 1024) printf "%.2f KB", bytes / 1024
            else printf "%d bytes", bytes
        }'
    fi
}

# Función para verificar memoria
verificar_memoria() {
    local archivo_fasta=$1
    
    echo -e "${CYAN}Verificando requisitos de memoria...${NC}"
    echo ""
    
    # Leer longitudes
    local longitudes=$(leer_longitudes_fasta "$archivo_fasta")
    local len1=$(echo $longitudes | cut -d' ' -f1)
    local len2=$(echo $longitudes | cut -d' ' -f2)
    
    if [ "$len1" -eq 0 ] || [ "$len2" -eq 0 ]; then
        echo -e "${RED}Error: No se pudieron leer las longitudes de las secuencias${NC}"
        return 1
    fi
    
    echo -e "Longitud secuencia A: ${YELLOW}$len1${NC} caracteres"
    echo -e "Longitud secuencia B: ${YELLOW}$len2${NC} caracteres"
    echo ""
    
    # Calcular memoria necesaria
    local mem_necesaria=$(calcular_memoria_necesaria "$len1" "$len2")
    local mem_necesaria_gb=0
    if command -v bc &> /dev/null; then
        mem_necesaria_gb=$(echo "scale=2; $mem_necesaria / 1073741824" | bc)
    else
        mem_necesaria_gb=$(awk -v mem=$mem_necesaria 'BEGIN {printf "%.2f", mem / 1073741824}')
    fi
    
    echo -e "Memoria estimada necesaria: ${YELLOW}$(formatear_bytes $mem_necesaria)${NC}"
    
    # Obtener RAM disponible
    local ram_disponible=$(obtener_ram_disponible)
    
    if [ "$ram_disponible" -eq 0 ]; then
        echo -e "${YELLOW}Advertencia: No se pudo determinar la RAM disponible${NC}"
        echo -e "${YELLOW}Continuando con advertencia...${NC}"
        echo ""
        return 0
    fi
    
    local ram_disponible_gb=0
    if command -v bc &> /dev/null; then
        ram_disponible_gb=$(echo "scale=2; $ram_disponible / 1073741824" | bc)
    else
        ram_disponible_gb=$(awk -v ram=$ram_disponible 'BEGIN {printf "%.2f", ram / 1073741824}')
    fi
    echo -e "RAM disponible: ${GREEN}$(formatear_bytes $ram_disponible)${NC}"
    echo ""
    
    # Verificar si hay suficiente memoria (usar 80% de la disponible como límite)
    local umbral=$(( ram_disponible * 80 / 100 ))
    
    if [ "$mem_necesaria" -gt "$umbral" ]; then
        echo -e "${RED}========================================${NC}"
        echo -e "${RED}ERROR: Memoria insuficiente${NC}"
        echo -e "${RED}========================================${NC}"
        echo ""
        echo -e "La matriz de alineamiento requiere aproximadamente:"
        echo -e "  ${RED}$(formatear_bytes $mem_necesaria)${NC}"
        echo ""
        echo -e "RAM disponible:"
        echo -e "  ${GREEN}$(formatear_bytes $ram_disponible)${NC}"
        echo ""
        echo -e "Se requiere al menos ${YELLOW}80%${NC} de la RAM disponible para ejecutar de forma segura."
        echo ""
        echo -e "${YELLOW}Recomendaciones:${NC}"
        echo -e "  - Use secuencias más cortas (ej: 1k, 2k, 5k, 10k)"
        echo -e "  - Para 50k se recomienda al menos 20 GB de RAM"
        echo -e "  - Para 100k se recomienda al menos 80 GB de RAM"
        echo ""
        return 1
    fi
    
    # Advertencia para secuencias grandes
    if command -v bc &> /dev/null; then
        if [ "$mem_necesaria_gb" != "0" ] && (( $(echo "$mem_necesaria_gb > 5" | bc -l) )); then
            echo -e "${YELLOW}Advertencia:${NC} Esta ejecución requiere una cantidad significativa de memoria."
            echo -e "  Tiempo estimado de ejecución: ${YELLOW}puede ser largo${NC}"
            echo ""
        fi
    else
        # Fallback con awk
        local mem_gb_num=$(awk -v mem=$mem_necesaria 'BEGIN {printf "%.0f", mem / 1073741824}')
        if [ "$mem_gb_num" -gt 5 ]; then
            echo -e "${YELLOW}Advertencia:${NC} Esta ejecución requiere una cantidad significativa de memoria."
            echo -e "  Tiempo estimado de ejecución: ${YELLOW}puede ser largo${NC}"
            echo ""
        fi
    fi
    
    echo -e "${GREEN}✓ Memoria suficiente. Continuando...${NC}"
    echo ""
    
    return 0
}

# Función para mostrar ayuda
mostrar_ayuda() {
    echo "Uso: $0 -n <numero> -f <archivo.fasta> -p <match> <mismatch> <gap> [opciones]"
    echo ""
    echo "Ejecuta main-secuencial múltiples veces y calcula promedios de tiempos y puntuación."
    echo ""
    echo "Parámetros obligatorios:"
    echo "  -n, --numero <n>         Número de ejecuciones para promediar"
    echo "  -f, --fasta <archivo>     Archivo FASTA con las secuencias"
    echo "  -p <match> <mismatch> <gap>  Parámetros de puntuación"
    echo ""
    echo "Opciones:"
    echo "  -o, --output <archivo>    Archivo CSV de salida [default: resultado_promedio.csv]"
    echo "  -v, --verbose             Mostrar salida detallada de cada ejecución"
    echo "  -h, --help               Muestra esta ayuda"
    echo ""
    echo "Ejemplo:"
    echo "  $0 -n 5 -f datos/dna_1k.fasta -p 2 0 -2"
    echo "  $0 -n 10 -f datos/protein_5k.fasta -p 5 -4 -10 -o resultados.csv"
}

# Función para calcular estadísticas
calcular_estadisticas() {
    local archivo=$1
    local campo=$2
    
    # Usar awk para calcular estadísticas
    awk -v campo="$campo" -F',' '
    BEGIN {
        # Determinar número de columna según el campo
        if (campo == "tiempo_init_ms") col = 7
        else if (campo == "tiempo_llenado_ms") col = 8
        else if (campo == "tiempo_traceback_ms") col = 9
        else if (campo == "tiempo_total_ms") col = 10
        else if (campo == "puntuacion") col = 11
        else col = 10
        
        count = 0
        sum = 0
        min = 999999999
        max = 0
    }
    NR > 1 {  # Saltar encabezado
        if ($col != "" && $col != "0") {
            valor = $col
            sum += valor
            count++
            if (valor < min) min = valor
            if (valor > max) max = valor
            valores[count] = valor
        }
    }
    END {
        if (count > 0) {
            promedio = sum / count
            print promedio " " min " " max " " count
        } else {
            print "0 0 0 0"
        }
    }' "$archivo"
}

# Función para calcular desviación estándar
calcular_desviacion() {
    local archivo=$1
    local campo=$2
    local promedio=$3
    
    awk -v campo="$campo" -v prom="$promedio" -F',' '
    BEGIN {
        if (campo == "tiempo_init_ms") col = 7
        else if (campo == "tiempo_llenado_ms") col = 8
        else if (campo == "tiempo_traceback_ms") col = 9
        else if (campo == "tiempo_total_ms") col = 10
        else if (campo == "puntuacion") col = 11
        else col = 10
        
        count = 0
        sum_sq_diff = 0
    }
    NR > 1 {
        if ($col != "" && $col != "0") {
            diff = $col - prom
            sum_sq_diff += diff * diff
            count++
        }
    }
    END {
        if (count > 1) {
            varianza = sum_sq_diff / (count - 1)
            desv = sqrt(varianza)
            print desv
        } else {
            print "0"
        }
    }' "$archivo"
}

# Parsear argumentos
while [[ $# -gt 0 ]]; do
    case $1 in
        -n|--numero)
            NUM_EJECUCIONES="$2"
            shift 2
            ;;
        -f|--fasta)
            ARCHIVO_FASTA="$2"
            shift 2
            ;;
        -p)
            MATCH="$2"
            MISMATCH="$3"
            GAP="$4"
            shift 4
            ;;
        -o|--output)
            OUTPUT_CSV="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=true
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

# Validar argumentos obligatorios
if [ -z "$NUM_EJECUCIONES" ] || ! [[ "$NUM_EJECUCIONES" =~ ^[0-9]+$ ]] || [ "$NUM_EJECUCIONES" -lt 1 ]; then
    echo -e "${RED}Error: Debe especificar un número válido de ejecuciones con -n${NC}"
    mostrar_ayuda
    exit 1
fi

if [ -z "$ARCHIVO_FASTA" ] || [ ! -f "$ARCHIVO_FASTA" ]; then
    echo -e "${RED}Error: Debe especificar un archivo FASTA válido con -f${NC}"
    mostrar_ayuda
    exit 1
fi

if [ -z "$MATCH" ] || [ -z "$MISMATCH" ] || [ -z "$GAP" ]; then
    echo -e "${RED}Error: Debe especificar parámetros de puntuación con -p <match> <mismatch> <gap>${NC}"
    mostrar_ayuda
    exit 1
fi

# Verificar que el ejecutable existe
if [ ! -f "$EJECUTABLE" ]; then
    echo -e "${YELLOW}El ejecutable no está compilado. Compilando...${NC}"
    if ! make; then
        echo -e "${RED}Error: No se pudo compilar el programa${NC}"
        exit 1
    fi
fi

# Verificar memoria antes de ejecutar
if ! verificar_memoria "$ARCHIVO_FASTA"; then
    echo -e "${RED}La ejecución ha sido cancelada por falta de memoria.${NC}"
    exit 1
fi

# Limpiar archivo temporal
rm -f "$TEMP_CSV"

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Ejecución con Promedios${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Archivo FASTA: $ARCHIVO_FASTA"
echo "Parámetros de puntuación: match=$MATCH, mismatch=$MISMATCH, gap=$GAP"
echo "Número de ejecuciones: $NUM_EJECUCIONES"
echo "Archivo de salida: $OUTPUT_CSV"
echo ""

# Ejecutar múltiples veces
echo -e "${BLUE}Ejecutando $NUM_EJECUCIONES iteraciones...${NC}"
echo ""

for i in $(seq 1 $NUM_EJECUCIONES); do
    echo -ne "${YELLOW}[$i/$NUM_EJECUCIONES]${NC} "
    
    if [ "$VERBOSE" = true ]; then
        echo ""
        "$EJECUTABLE" -f "$ARCHIVO_FASTA" -p "$MATCH" "$MISMATCH" "$GAP" -o "$TEMP_CSV" 2>&1
    else
        "$EJECUTABLE" -f "$ARCHIVO_FASTA" -p "$MATCH" "$MISMATCH" "$GAP" -o "$TEMP_CSV" > /dev/null 2>&1
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}✓${NC}"
        else
            echo -e "${RED}✗ Error${NC}"
        fi
    fi
done

echo ""

# Verificar que se generaron datos
if [ ! -f "$TEMP_CSV" ] || [ ! -s "$TEMP_CSV" ]; then
    echo -e "${RED}Error: No se generaron datos en el archivo temporal${NC}"
    exit 1
fi

# Calcular estadísticas
echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}Estadísticas${NC}"
echo -e "${CYAN}========================================${NC}"
echo ""

# Obtener estadísticas para cada métrica
STATS_INIT=$(calcular_estadisticas "$TEMP_CSV" "tiempo_init_ms")
STATS_LLENADO=$(calcular_estadisticas "$TEMP_CSV" "tiempo_llenado_ms")
STATS_TRACEBACK=$(calcular_estadisticas "$TEMP_CSV" "tiempo_traceback_ms")
STATS_TOTAL=$(calcular_estadisticas "$TEMP_CSV" "tiempo_total_ms")
STATS_PUNTUACION=$(calcular_estadisticas "$TEMP_CSV" "puntuacion")

# Extraer valores
read PROM_INIT MIN_INIT MAX_INIT COUNT_INIT <<< "$STATS_INIT"
read PROM_LLENADO MIN_LLENADO MAX_LLENADO COUNT_LLENADO <<< "$STATS_LLENADO"
read PROM_TRACEBACK MIN_TRACEBACK MAX_TRACEBACK COUNT_TRACEBACK <<< "$STATS_TRACEBACK"
read PROM_TOTAL MIN_TOTAL MAX_TOTAL COUNT_TOTAL <<< "$STATS_TOTAL"
read PROM_PUNT MIN_PUNT MAX_PUNT COUNT_PUNT <<< "$STATS_PUNTUACION"

# Calcular desviaciones estándar
DESV_INIT=$(calcular_desviacion "$TEMP_CSV" "tiempo_init_ms" "$PROM_INIT")
DESV_LLENADO=$(calcular_desviacion "$TEMP_CSV" "tiempo_llenado_ms" "$PROM_LLENADO")
DESV_TRACEBACK=$(calcular_desviacion "$TEMP_CSV" "tiempo_traceback_ms" "$PROM_TRACEBACK")
DESV_TOTAL=$(calcular_desviacion "$TEMP_CSV" "tiempo_total_ms" "$PROM_TOTAL")

# Mostrar resultados
printf "${CYAN}Tiempo de Inicialización:${NC}\n"
printf "  Promedio: %.4f ms\n" "$PROM_INIT"
printf "  Mínimo:   %.4f ms\n" "$MIN_INIT"
printf "  Máximo:   %.4f ms\n" "$MAX_INIT"
printf "  Desv. Est: %.4f ms\n" "$DESV_INIT"
echo ""

printf "${CYAN}Tiempo de Llenado:${NC}\n"
printf "  Promedio: %.4f ms\n" "$PROM_LLENADO"
printf "  Mínimo:   %.4f ms\n" "$MIN_LLENADO"
printf "  Máximo:   %.4f ms\n" "$MAX_LLENADO"
printf "  Desv. Est: %.4f ms\n" "$DESV_LLENADO"
echo ""

printf "${CYAN}Tiempo de Traceback:${NC}\n"
printf "  Promedio: %.4f ms\n" "$PROM_TRACEBACK"
printf "  Mínimo:   %.4f ms\n" "$MIN_TRACEBACK"
printf "  Máximo:   %.4f ms\n" "$MAX_TRACEBACK"
printf "  Desv. Est: %.4f ms\n" "$DESV_TRACEBACK"
echo ""

printf "${CYAN}Tiempo Total:${NC}\n"
printf "  Promedio: %.4f ms\n" "$PROM_TOTAL"
printf "  Mínimo:   %.4f ms\n" "$MIN_TOTAL"
printf "  Máximo:   %.4f ms\n" "$MAX_TOTAL"
printf "  Desv. Est: %.4f ms\n" "$DESV_TOTAL"
echo ""

printf "${CYAN}Puntuación:${NC}\n"
printf "  Promedio: %.2f\n" "$PROM_PUNT"
printf "  Mínimo:   %.2f\n" "$MIN_PUNT"
printf "  Máximo:   %.2f\n" "$MAX_PUNT"
echo ""

# Guardar resultados en archivo de salida
echo -e "${BLUE}Guardando resultados en $OUTPUT_CSV...${NC}"
{
    echo "tipo_metric,valor,promedio,minimo,maximo,desviacion_estandar,num_ejecuciones"
    printf "tiempo_init_ms,%.4f,%.4f,%.4f,%.4f,%.4f,%d\n" "$PROM_INIT" "$PROM_INIT" "$MIN_INIT" "$MAX_INIT" "$DESV_INIT" "$NUM_EJECUCIONES"
    printf "tiempo_llenado_ms,%.4f,%.4f,%.4f,%.4f,%.4f,%d\n" "$PROM_LLENADO" "$PROM_LLENADO" "$MIN_LLENADO" "$MAX_LLENADO" "$DESV_LLENADO" "$NUM_EJECUCIONES"
    printf "tiempo_traceback_ms,%.4f,%.4f,%.4f,%.4f,%.4f,%d\n" "$PROM_TRACEBACK" "$PROM_TRACEBACK" "$MIN_TRACEBACK" "$MAX_TRACEBACK" "$DESV_TRACEBACK" "$NUM_EJECUCIONES"
    printf "tiempo_total_ms,%.4f,%.4f,%.4f,%.4f,%.4f,%d\n" "$PROM_TOTAL" "$PROM_TOTAL" "$MIN_TOTAL" "$MAX_TOTAL" "$DESV_TOTAL" "$NUM_EJECUCIONES"
    printf "puntuacion,%.2f,%.2f,%.2f,%.2f,0.00,%d\n" "$PROM_PUNT" "$PROM_PUNT" "$MIN_PUNT" "$MAX_PUNT" "$NUM_EJECUCIONES"
} > "$OUTPUT_CSV"

# Copiar datos individuales si se solicita
if [ "$OUTPUT_CSV" != "$TEMP_CSV" ]; then
    # Crear archivo con todas las ejecuciones individuales
    INDIVIDUAL_CSV="${OUTPUT_CSV%.csv}_individual.csv"
    cp "$TEMP_CSV" "$INDIVIDUAL_CSV"
    echo -e "${BLUE}Datos individuales guardados en $INDIVIDUAL_CSV${NC}"
fi

# Limpiar archivo temporal
rm -f "$TEMP_CSV"

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}✓ Proceso completado${NC}"
echo -e "${GREEN}Resultados guardados en: $OUTPUT_CSV${NC}"
echo -e "${GREEN}========================================${NC}"

