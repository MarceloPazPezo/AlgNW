#!/bin/bash

# ================= CONFIGURACIÓN =================
source /usr/local/etc/extrae.sh
export EXTRAE_ON=1
export EXTRAE_CONFIG_FILE=extrae.xml
# Definimos la librería pero NO la exportamos globalmente todavía
LIB_EXTRAE=${EXTRAE_HOME}/lib/libomptrace.so

# Configuración OpenMP Base
export OMP_NUM_THREADS=4
export OMP_PLACES="cores"
export OMP_PROC_BIND="spread"

# Datos y Salida
ARCHIVO_FASTA="datos/dna_512.fasta"
OUT_DIR="trazas_512"
mkdir -p "$OUT_DIR"

echo "=========================================================="
echo "Iniciando generación de trazas en carpeta: $OUT_DIR"
echo "=========================================================="

# ================= BUCLES DE EJECUCIÓN =================

declare -A estrategias=( 
    ["antidiagonal"]="-a" 
    ["bloques"]="-b" 
)

schedules=("static" "static,1" "dynamic" "guided")

for est_nombre in "${!estrategias[@]}"; do
    flag="${estrategias[$est_nombre]}"
    
    for sched in "${schedules[@]}"; do
        
        export OMP_SCHEDULE="$sched"
        
        # Limpiar nombre (ej: static,1 -> static1)
        sched_clean=$(echo $sched | tr -d ',')
        nombre_final="${est_nombre}_${sched_clean}"
        
        echo "----------------------------------------------------"
        echo "Ejecutando: $est_nombre ($flag) con $sched"
        
        # --- EJECUCIÓN CONTROLADA ---
        LD_PRELOAD=$LIB_EXTRAE ./bin/main-paralelo -f "$ARCHIVO_FASTA" -p 2 -1 -2 $flag
        
        # --- MOVER ARCHIVOS Y AÑADIR EVENTOS ---
        archivo_pcf=""
        
        # Detectar si se generó como main-paralelo o TRACE
        if [ -f "main-paralelo.prv" ]; then
            mv main-paralelo.prv "$OUT_DIR/${nombre_final}.prv"
            mv main-paralelo.pcf "$OUT_DIR/${nombre_final}.pcf"
            mv main-paralelo.row "$OUT_DIR/${nombre_final}.row"
            archivo_pcf="$OUT_DIR/${nombre_final}.pcf"
            echo "   -> OK: Guardado como ${nombre_final}.prv"
            
        elif [ -f "TRACE.prv" ]; then
            mv TRACE.prv "$OUT_DIR/${nombre_final}.prv"
            mv TRACE.pcf "$OUT_DIR/${nombre_final}.pcf"
            mv TRACE.row "$OUT_DIR/${nombre_final}.row"
            archivo_pcf="$OUT_DIR/${nombre_final}.pcf"
            echo "   -> OK: Guardado como ${nombre_final}.prv (desde TRACE)"
        else
            echo "   !! ERROR: No se encontró el archivo .prv generado"
        fi
        
        # --- AÑADIR EVENTOS PERSONALIZADOS AL PCF ---
        if [ ! -z "$archivo_pcf" ] && [ -f "$archivo_pcf" ]; then
            cat <<EOF >> "$archivo_pcf"

EVENT_TYPE
0    1000        fase_1
VALUES
0      End
1      Begin

EVENT_TYPE
0    2000        fase_2
VALUES
0      End
1      Begin

EVENT_TYPE
0    3000        fase_2_distribucion
VALUES
0      End
1      Begin

EVENT_TYPE
0    4000        fase_2_paralela
VALUES
0      End
1      Begin

EVENT_TYPE
0    5000        fase_3
VALUES
0      End
1      Begin
EOF
            echo "   -> Eventos personalizados añadidos al PCF."
        fi
        
        # Limpieza de temporales de Extrae
        rm -rf set-0 TRACE.mpits
        
    done
done

echo "===================================================="
echo "Proceso finalizado."
