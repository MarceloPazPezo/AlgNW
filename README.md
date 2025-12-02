# AlgNW - Versión Simplificada (DNA)

Implementación simplificada del algoritmo Needleman-Wunsch para alineamiento global de secuencias DNA, con versiones secuencial y paralelas (antidiagonal y bloques) usando OpenMP.

## Descripción

Este proyecto implementa el algoritmo Needleman-Wunsch para alineamiento global de secuencias biológicas, enfocado únicamente en secuencias DNA. Incluye:

- **Algoritmo secuencial**: Implementación de referencia
- **Algoritmo paralelo antidiagonal**: Paralelización por antidiagonales
- **Algoritmo paralelo bloques**: Paralelización por bloques

## Compilación

```bash
cd src
make                    # Compila todos los programas
make clean              # Limpia archivos compilados
make rebuild            # Limpia y recompila todo
```

Los ejecutables se generan en `bin/`:
- `bin/main-secuencial` - Algoritmo secuencial
- `bin/main-paralelo` - Comparación de métodos (secuencial vs paralelo)
- `bin/main-gen-secuencia` - Generador de secuencias DNA

## Uso

### 1. Generar datos de prueba

```bash
# Generar secuencias DNA (potencias de 2: 128, 256, 512, 1k, 2k, 4k, 8k, 16k, 32k)
./generar-datos.sh

# Con opciones personalizadas
./generar-datos.sh -s 0.85 -o datos_custom/
```

### 2. Ejecutar algoritmo secuencial

```bash
./bin/main-secuencial -f datos/dna_1k.fasta -p 2 -1 -2 -o resultado.csv
```

### 3. Ejecutar comparación paralela

```bash
# Configurar variables de entorno OpenMP
export OMP_NUM_THREADS=8
export OMP_SCHEDULE="dynamic,1"

# Ejecutar todos los métodos
./bin/main-paralelo -f datos/dna_1k.fasta -p 2 -1 -2 -r 5 -o resultados.csv

# Ejecutar un método específico
./bin/main-paralelo -f datos/dna_1k.fasta -p 2 -1 -2 -r 5 -m antidiagonal -o resultados.csv
```

### 4. Ejecutar benchmark completo

```bash
# Ejecuta secuencial y paralelo con diferentes configuraciones
# (tamaños, threads, schedules)
./ejecutar-benchmark.sh
```

El script itera sobre:
- **Tamaños**: 128, 256, 512, 1k, 2k, 4k, 8k, 16k, 32k
- **Threads**: 2, 4, 6, 8 (para cada método paralelo)
- **Schedules**: static, static,1, dynamic,1, guided,1
- **Métodos**: antidiagonal, bloques

Los resultados se guardan en `resultados/resultados_completo.csv` con el siguiente formato:
- Primero ejecuta el secuencial (baseline)
- Luego ejecuta cada método paralelo con cada schedule y número de threads
- El CSV incluye columnas: archivo_fasta, metodo, repeticion, threads, schedule, longitud_A, longitud_B, match, mismatch, gap, tiempos, puntuacion

## Parámetros

- `-f <archivo.fasta>`: Archivo FASTA con las secuencias (obligatorio)
- `-p <match> <mismatch> <gap>`: Parámetros de puntuación (obligatorio)
- `-r <numero>`: Número de repeticiones [default: 1]
- `-m <metodo>`: Método específico (secuencial, antidiagonal, bloques) [default: todos]
- `-o <archivo.csv>`: Archivo de salida CSV [default: benchmark.csv o resultado.csv]
- `-h, --help`: Mostrar ayuda

## Variables de Entorno OpenMP

- `OMP_NUM_THREADS`: Número de threads a usar (ej: `export OMP_NUM_THREADS=8`)
- `OMP_SCHEDULE`: Planificador de OpenMP (ej: `export OMP_SCHEDULE="dynamic,1"`)

## Soporte para Extrae/Paraver

El código incluye eventos de usuario de Extrae para marcar las diferentes fases del algoritmo y analizar la distribución del trabajo en los loops paralelos:

### Eventos de Fases
- **Evento 1000**: Fase de inicialización (1=inicio, 0=fin)
- **Evento 2000**: Fase de llenado de matriz (1=inicio, 0=fin)
- **Evento 3000**: Fase de traceback (1=inicio, 0=fin)

### Eventos de Distribución de Trabajo (dentro de loops paralelos)
Para analizar cómo los diferentes planificadores (static, dynamic, guided) distribuyen el trabajo:

- **Eventos 4000-4999**: Inicio de iteración/bloque por thread
  - `4000 + thread_id`: Indica que el thread `thread_id` está empezando a procesar
  - Valor del evento: número de iteración/bloque que está procesando
- **Eventos 5000-5999**: Fin de iteración/bloque por thread
  - `5000 + thread_id`: Indica que el thread `thread_id` terminó de procesar
  - Valor del evento: número de iteración/bloque que terminó

**Ejemplo de interpretación:**
- Con `schedule(static)`: Verás bloques uniformes de trabajo asignados secuencialmente a cada thread
- Con `schedule(dynamic)`: Verás trabajo distribuido de forma irregular, threads tomando trabajo cuando terminan
- Con `schedule(guided)`: Similar a dynamic pero con chunks que disminuyen exponencialmente

Los eventos se insertan usando `Extrae_event(tipo, valor)` donde:
- `tipo` es el identificador del evento
- `valor` contiene información sobre la iteración/bloque procesado

### Compilar con soporte de Extrae

```bash
# Opción 1: Si Extrae está en /usr
make EXTRAE=1

# Opción 2: Especificar ruta de Extrae
make EXTRAE_HOME=/ruta/a/extrae
```

### Ejecutar con Extrae

```bash
# Configurar Extrae
export EXTRAE_CONFIG_FILE=./extrae.xml
export EXTRAE_TRACE_NAME=nw_trace

# Ejecutar el programa
./bin/main-paralelo -f datos/dna_1k.fasta -p 2 -1 -2 -r 1
```

Los eventos aparecerán en Paraver como eventos de usuario, permitiendo identificar visualmente cada fase del algoritmo en las trazas de rendimiento.

Para más información, consulta `EXTRAE_GUIA.md` en el directorio raíz del proyecto.

## Estructura del Proyecto

```
srcv2/
├── secuencial.h / secuencial.cpp # Algoritmo secuencial
├── paralelo.h / paralelo.cpp     # Algoritmos paralelos
├── puntuacion.h / puntuacion.cpp # Sistema de puntuación DNA
├── generador_secuencias.h / .cpp # Generador de secuencias
├── tipos.h                       # Estructuras de datos
├── utilidades.h / utilidades.cpp # Funciones auxiliares
├── main-secuencial.cpp           # Programa secuencial
├── main-paralelo.cpp             # Programa paralelo
├── main-gen-secuencia.cpp        # Generador de secuencias
├── generar-datos.sh              # Script para generar datos
├── ejecutar-benchmark.sh         # Script para benchmark completo
└── Makefile                      # Sistema de compilación
```

## Ejemplo Completo

```bash
# 1. Compilar
make

# 2. Generar datos
./generar-datos.sh

# 3. Ejecutar benchmark
export OMP_NUM_THREADS=8
./ejecutar-benchmark.sh

# 4. Ver resultados
cat resultados/resultados_completo.csv
```

## Notas

- El proyecto está optimizado para secuencias DNA únicamente
- Los algoritmos paralelos usan `schedule(runtime)` para leer `OMP_SCHEDULE`
- El benchmark completo puede tardar considerablemente según el hardware
- Los resultados incluyen tiempos de inicialización, llenado de matriz y traceback
- Los eventos de Extrae están implementados y se activan automáticamente si se compila con soporte de Extrae

