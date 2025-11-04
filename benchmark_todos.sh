#!/bin/bash

# Script para ejecutar benchmarks de todos los archivos FASTA en el directorio datos/
# Ejecuta promedios de 10 veces para cada archivo

# Colores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Variables
DATOS_DIR="datos"
EJECUTAR_PROMEDIO="./ejecutar_promedio.sh"
NUM_EJECUCIONES=10
VERBOSE=false
SKIP_ERRORS=false

# Parámetros de puntuación por defecto
# ADN: match=2, mismatch=0, gap=-2 (estándar para secuencias de ADN)
# Proteínas: match=5, mismatch=-4, gap=-10 (similar a BLOSUM62)
DNA_MATCH=2
DNA_MISMATCH=0
DNA_GAP=-2

PROTEIN_MATCH=5
PROTEIN_MISMATCH=-4
PROTEIN_GAP=-10

# Función para mostrar ayuda
mostrar_ayuda() {
    echo "Uso: $0 [opciones]"
    echo ""
    echo "Ejecuta benchmarks de todos los archivos FASTA en el directorio datos/."
    echo "Para cada archivo ejecuta $NUM_EJECUCIONES veces y calcula promedios."
    echo ""
    echo "Opciones:"
    echo "  -d, --datos <directorio>  Directorio con archivos FASTA [default: datos/]"
    echo "  -n, --numero <n>          Número de ejecuciones por archivo [default: 10]"
    echo "  -v, --verbose             Mostrar salida detallada"
    echo "  --skip-errors             Continuar automáticamente si hay error (sin preguntar)"
    echo "  --dna-params <m> <mm> <g> Parámetros para ADN: match mismatch gap [default: 2 0 -2]"
    echo "  --protein-params <m> <mm> <g> Parámetros para proteínas: match mismatch gap [default: 5 -4 -10]"
    echo "  -h, --help               Muestra esta ayuda"
    echo ""
    echo "Ejemplo:"
    echo "  $0"
    echo "  $0 -n 5"
    echo "  $0 -v --skip-errors"
}

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

# Función para verificar memoria (versión silenciosa que solo retorna código)
verificar_memoria_rapida() {
    local archivo_fasta=$1
    
    # Leer longitudes
    local longitudes=$(leer_longitudes_fasta "$archivo_fasta")
    local len1=$(echo $longitudes | cut -d' ' -f1)
    local len2=$(echo $longitudes | cut -d' ' -f2)
    
    if [ "$len1" -eq 0 ] || [ "$len2" -eq 0 ]; then
        return 2  # Error leyendo archivo
    fi
    
    # Calcular memoria necesaria
    local mem_necesaria=$(calcular_memoria_necesaria "$len1" "$len2")
    
    # Obtener RAM disponible
    local ram_disponible=$(obtener_ram_disponible)
    
    if [ "$ram_disponible" -eq 0 ]; then
        return 0  # No se puede determinar, permitir ejecución
    fi
    
    # Verificar si hay suficiente memoria (usar 80% de la disponible como límite)
    local umbral=$(( ram_disponible * 80 / 100 ))
    
    if [ "$mem_necesaria" -gt "$umbral" ]; then
        # Retornar memoria necesaria y disponible para reporte
        echo "$mem_necesaria $ram_disponible $len1 $len2"
        return 1  # Memoria insuficiente
    fi
    
    return 0  # Memoria suficiente
}

# Función para detectar tipo de secuencia basándose en el nombre del archivo
detectar_tipo() {
    local archivo=$1
    local nombre=$(basename "$archivo" .fasta)
    
    # Buscar patrones en el nombre
    if [[ "$nombre" =~ ^dna_ ]] || [[ "$nombre" =~ ^DNA_ ]] || [[ "$nombre" =~ _dna$ ]]; then
        echo "dna"
    elif [[ "$nombre" =~ ^protein_ ]] || [[ "$nombre" =~ ^PROTEIN_ ]] || [[ "$nombre" =~ _protein$ ]]; then
        echo "protein"
    else
        # Intentar detectar leyendo el archivo (verificar alfabeto)
        # Por ahora, asumir ADN si no se puede determinar
        echo "dna"
    fi
}

# Parsear argumentos
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--datos)
            DATOS_DIR="$2"
            shift 2
            ;;
        -n|--numero)
            NUM_EJECUCIONES="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        --skip-errors)
            SKIP_ERRORS=true
            shift
            ;;
        --dna-params)
            DNA_MATCH="$2"
            DNA_MISMATCH="$3"
            DNA_GAP="$4"
            shift 4
            ;;
        --protein-params)
            PROTEIN_MATCH="$2"
            PROTEIN_MISMATCH="$3"
            PROTEIN_GAP="$4"
            shift 4
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

# Validar directorio
if [ ! -d "$DATOS_DIR" ]; then
    echo -e "${RED}Error: El directorio '$DATOS_DIR' no existe${NC}"
    exit 1
fi

# Validar que el script ejecutar_promedio.sh existe
if [ ! -f "$EJECUTAR_PROMEDIO" ]; then
    echo -e "${RED}Error: No se encontró el script $EJECUTAR_PROMEDIO${NC}"
    exit 1
fi

# Buscar archivos FASTA
archivos_fasta=$(find "$DATOS_DIR" -maxdepth 1 -name "*.fasta" -type f | sort)

if [ -z "$archivos_fasta" ]; then
    echo -e "${YELLOW}No se encontraron archivos FASTA en $DATOS_DIR${NC}"
    exit 0
fi

# Contar archivos
total_archivos=$(echo "$archivos_fasta" | wc -l)
archivos_procesados=0
archivos_exitosos=0
archivos_fallidos=0
archivos_descartados=0

# Arrays para almacenar información de descartados
declare -a descartados_archivo
declare -a descartados_razon
declare -a descartados_mem_necesaria
declare -a descartados_mem_disponible

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Benchmark de Todos los Archivos${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Directorio: $DATOS_DIR"
echo "Archivos encontrados: $total_archivos"
echo "Ejecuciones por archivo: $NUM_EJECUCIONES"
echo "Parámetros ADN: match=$DNA_MATCH, mismatch=$DNA_MISMATCH, gap=$DNA_GAP"
echo "Parámetros Proteínas: match=$PROTEIN_MATCH, mismatch=$PROTEIN_MISMATCH, gap=$PROTEIN_GAP"
echo ""

# Crear directorio para resultados
RESULTADOS_DIR="resultados_benchmark"
mkdir -p "$RESULTADOS_DIR"

# Procesar cada archivo
while IFS= read -r archivo; do
    ((archivos_procesados++))
    
    nombre_archivo=$(basename "$archivo")
    nombre_sin_ext=$(basename "$archivo" .fasta)
    
    echo -e "${CYAN}========================================${NC}"
    echo -e "${CYAN}[$archivos_procesados/$total_archivos] Procesando: $nombre_archivo${NC}"
    echo -e "${CYAN}========================================${NC}"
    echo ""
    
    # Verificar memoria primero (versión rápida sin output)
    info_memoria=$(verificar_memoria_rapida "$archivo" 2>/dev/null)
    resultado_verificacion=$?
    
    if [ $resultado_verificacion -eq 1 ]; then
        # Memoria insuficiente - descartar archivo
        mem_info=($info_memoria)
        mem_necesaria=${mem_info[0]}
        mem_disponible=${mem_info[1]}
        len1=${mem_info[2]}
        len2=${mem_info[3]}
        
        echo -e "${RED}⚠ Archivo descartado por memoria insuficiente${NC}"
        echo -e "  Longitudes: ${len1} x ${len2} caracteres"
        echo -e "  Memoria necesaria: ${RED}$(formatear_bytes $mem_necesaria)${NC}"
        echo -e "  RAM disponible: ${GREEN}$(formatear_bytes $mem_disponible)${NC}"
        echo -e "  ${YELLOW}Se requiere al menos 80% de la RAM disponible${NC}"
        echo ""
        
        # Guardar información del descartado
        descartados_archivo+=("$nombre_archivo")
        descartados_razon+=("Memoria insuficiente")
        descartados_mem_necesaria+=("$mem_necesaria")
        descartados_mem_disponible+=("$mem_disponible")
        ((archivos_descartados++))
        
        continue
    elif [ $resultado_verificacion -eq 2 ]; then
        # Error leyendo el archivo
        echo -e "${RED}⚠ Error al leer el archivo: $nombre_archivo${NC}"
        echo -e "  ${YELLOW}Archivo descartado (no se pudieron leer las longitudes)${NC}"
        echo ""
        
        descartados_archivo+=("$nombre_archivo")
        descartados_razon+=("Error leyendo archivo")
        descartados_mem_necesaria+=("0")
        descartados_mem_disponible+=("0")
        ((archivos_descartados++))
        
        continue
    fi
    
    # Memoria suficiente, continuar
    echo -e "${GREEN}✓ Memoria verificada - suficiente${NC}"
    echo ""
    
    # Detectar tipo
    tipo=$(detectar_tipo "$archivo")
    
    # Seleccionar parámetros según el tipo
    if [ "$tipo" = "dna" ]; then
        MATCH=$DNA_MATCH
        MISMATCH=$DNA_MISMATCH
        GAP=$DNA_GAP
        echo -e "${BLUE}Tipo detectado: ADN${NC}"
    else
        MATCH=$PROTEIN_MATCH
        MISMATCH=$PROTEIN_MISMATCH
        GAP=$PROTEIN_GAP
        echo -e "${BLUE}Tipo detectado: Proteína${NC}"
    fi
    
    echo -e "Parámetros: match=$MATCH, mismatch=$MISMATCH, gap=$GAP"
    echo ""
    
    # Crear nombre de archivo de salida
    archivo_salida="$RESULTADOS_DIR/${nombre_sin_ext}_promedio.csv"
    
    # Construir comando
    cmd="$EJECUTAR_PROMEDIO -n $NUM_EJECUCIONES -f \"$archivo\" -p $MATCH $MISMATCH $GAP -o \"$archivo_salida\""
    
    if [ "$VERBOSE" = false ]; then
        cmd="$cmd 2>/dev/null"
    fi
    
    # Ejecutar
    if eval "$cmd" 2>/dev/null; then
        echo -e "${GREEN}✓ Completado: $nombre_archivo${NC}"
        echo -e "${GREEN}  Resultados en: $archivo_salida${NC}"
        ((archivos_exitosos++))
    else
        echo -e "${RED}✗ Error al procesar: $nombre_archivo${NC}"
        ((archivos_fallidos++))
        
        if [ "$SKIP_ERRORS" = false ]; then
            echo ""
            echo -e "${YELLOW}¿Desea continuar con los demás archivos? (s/n)${NC}"
            read -r respuesta
            if [[ ! "$respuesta" =~ ^[sS]$ ]]; then
                echo "Proceso cancelado por el usuario"
                break
            fi
        fi
    fi
    
    echo ""
    echo ""
    
done <<< "$archivos_fasta"

# Resumen final
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Resumen Final${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo -e "Archivos procesados: ${CYAN}$archivos_procesados${NC}"
echo -e "Archivos exitosos:   ${GREEN}$archivos_exitosos${NC}"
echo -e "Archivos fallidos:   ${RED}$archivos_fallidos${NC}"
echo -e "Archivos descartados: ${YELLOW}$archivos_descartados${NC}"
echo ""
echo -e "Resultados guardados en: ${BLUE}$RESULTADOS_DIR/${NC}"
echo ""

# Mostrar archivos descartados si los hay
if [ $archivos_descartados -gt 0 ]; then
    echo -e "${YELLOW}========================================${NC}"
    echo -e "${YELLOW}Archivos Descartados${NC}"
    echo -e "${YELLOW}========================================${NC}"
    echo ""
    
    for i in $(seq 0 $(( ${#descartados_archivo[@]} - 1 )) ); do
        echo -e "${YELLOW}${descartados_archivo[$i]}${NC}"
        echo -e "  Razón: ${descartados_razon[$i]}"
        if [ "${descartados_mem_necesaria[$i]}" != "0" ]; then
            echo -e "  Memoria necesaria: $(formatear_bytes ${descartados_mem_necesaria[$i]})"
            echo -e "  RAM disponible: $(formatear_bytes ${descartados_mem_disponible[$i]})"
        fi
        echo ""
    done
    
    echo -e "${YELLOW}Nota:${NC} Estos archivos fueron descartados automáticamente"
    echo -e "      para evitar que el proceso falle por falta de memoria."
    echo ""
fi

# Crear archivo de resumen consolidado si hay resultados
if [ $archivos_exitosos -gt 0 ]; then
    resumen_consolidado="$RESULTADOS_DIR/resumen_consolidado.csv"
    echo -e "${BLUE}Creando resumen consolidado...${NC}"
    
    # Encabezado
    echo "archivo,tipo,ejecuciones,promedio_tiempo_total_ms,min_tiempo_total_ms,max_tiempo_total_ms,desv_tiempo_total_ms,promedio_puntuacion" > "$resumen_consolidado"
    
    # Consolidar datos de cada archivo
    for archivo_resultado in "$RESULTADOS_DIR"/*_promedio.csv; do
        if [ -f "$archivo_resultado" ]; then
            nombre_base=$(basename "$archivo_resultado" _promedio.csv)
            tipo_detectado=$(detectar_tipo "$DATOS_DIR/${nombre_base}.fasta")
            
            # Extraer estadísticas de tiempo_total_ms (campo 4 en la línea que contiene tiempo_total_ms)
            if [ -f "$archivo_resultado" ]; then
                tiempo_linea=$(grep "tiempo_total_ms" "$archivo_resultado")
                if [ -n "$tiempo_linea" ]; then
                    promedio=$(echo "$tiempo_linea" | cut -d',' -f3)
                    minimo=$(echo "$tiempo_linea" | cut -d',' -f4)
                    maximo=$(echo "$tiempo_linea" | cut -d',' -f5)
                    desviacion=$(echo "$tiempo_linea" | cut -d',' -f6)
                    num_ejec=$(echo "$tiempo_linea" | cut -d',' -f7)
                    
                    # Extraer puntuación promedio
                    punt_linea=$(grep "puntuacion" "$archivo_resultado")
                    punt_promedio=""
                    if [ -n "$punt_linea" ]; then
                        punt_promedio=$(echo "$punt_linea" | cut -d',' -f3)
                    fi
                    
                    echo "${nombre_base}.fasta,$tipo_detectado,$num_ejec,$promedio,$minimo,$maximo,$desviacion,$punt_promedio" >> "$resumen_consolidado"
                fi
            fi
        fi
    done
    
    echo -e "${GREEN}✓ Resumen consolidado creado: $resumen_consolidado${NC}"
fi

echo -e "${GREEN}========================================${NC}"
