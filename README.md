# Algoritmo Needleman-Wunsch (AlgNW)

Proyecto de implementación del algoritmo de alineamiento global Needleman-Wunsch para secuencias biológicas.

## Compilación

### Requisitos
- Compilador C++ compatible con C++11 (g++ o clang++)
- Sistema operativo Linux/Unix

### Compilar los programas

Para compilar todos los programas:
```bash
make
```

Esto generará los ejecutables en el directorio `bin/`:
- `bin/main-secuencial` - Programa de alineamiento secuencial
- `bin/main-gen-secuencia` - Generador de secuencias

### Opciones del Makefile

- `make` o `make all` - Compila todos los programas
- `make clean` - Elimina los archivos compilados
- `make rebuild` - Limpia y recompila todo desde cero
- `make help` - Muestra ayuda sobre los comandos disponibles

## Uso de los Programas

### 1. Generador de Secuencias (`main-gen-secuencia`)

Genera pares de secuencias biológicas para pruebas y benchmarking.

#### Uso básico
```bash
./bin/main-gen-secuencia -t <tipo> -l <longitud> -s <similitud> -o <salida>
```

#### Parámetros
- `-t, --tipo`: Tipo de secuencia (`dna`, `rna`, `proteina` o `protein`)
- `-l, --longitud`: Longitud de las secuencias
- `-s, --similitud`: Similitud objetivo entre 0.0 y 1.0
- `-o, --salida`: Prefijo del archivo de salida (se añadirá `.fasta`)
- `-b, --batch`: Modo lote (genera múltiples archivos)
- `-h, --ayuda`: Muestra ayuda

#### Ejemplos

Generar un par de secuencias de DNA:
```bash
./bin/main-gen-secuencia -t dna -l 100 -s 0.9 -o datos/test
```
Esto creará el archivo `datos/test.fasta`

Generar un lote de secuencias para benchmarking:
```bash
./bin/main-gen-secuencia -b -o datos/
```
Esto generará múltiples archivos con diferentes longitudes y similitudes.

### Script de Generación de Datos (`generar_datos.sh`)

Script de conveniencia para generar automáticamente archivos FASTA de ADN y proteínas con las longitudes comúnmente usadas para benchmarking.

#### Uso básico
```bash
./generar_datos.sh
```

Por defecto, esto generará archivos tanto de **ADN** como de **proteínas** con las siguientes longitudes:
- 500 (500 caracteres)
- 1k (1000 caracteres)
- 2k (2000 caracteres)
- 5k (5000 caracteres)
- 10k (10000 caracteres)
- 20k (20000 caracteres)
- 30k (30000 caracteres)
- 50k (50000 caracteres)
- 75k (75000 caracteres)
- 100k (100000 caracteres)

Los archivos se guardarán en el directorio `datos/` con nombres:
- ADN: `dna_1k.fasta`, `dna_2k.fasta`, etc.
- Proteínas: `protein_1k.fasta`, `protein_2k.fasta`, etc.

#### Opciones
- `-s, --similitud <valor>`: Similitud objetivo (0.0 - 1.0) [default: 0.9]
- `-o, --output <directorio>`: Directorio de salida [default: datos/]
- `--solo-dna`: Generar solo archivos de ADN
- `--solo-proteina`: Generar solo archivos de proteínas
- `-h, --help`: Muestra ayuda

#### Ejemplos

Generar datos con similitud personalizada (ADN y proteínas):
```bash
./generar_datos.sh -s 0.85
```

Generar solo datos de ADN:
```bash
./generar_datos.sh --solo-dna
```

Generar solo datos de proteínas:
```bash
./generar_datos.sh --solo-proteina
```

Generar datos en un directorio específico:
```bash
./generar_datos.sh -o data/ -s 0.95
```

**Nota:** El script automáticamente compilará el generador si no está disponible.

### 2. Alineamiento Secuencial (`main-secuencial`)

Ejecuta el algoritmo de alineamiento Needleman-Wunsch de forma secuencial.

#### Uso básico
```bash
./bin/main-secuencial -f <archivo.fasta> -p <match> <mismatch> <gap> [-o <salida.csv>]
```

#### Parámetros
- `-f <archivo.fasta>`: Archivo FASTA con las secuencias (OBLIGATORIO)
  - Debe contener al menos 2 secuencias
  - El programa usará las primeras 2 secuencias encontradas
- `-p <match> <mismatch> <gap>`: Parámetros de puntuación (OBLIGATORIO)
  - `match`: Puntuación por coincidencia (ej: 2)
  - `mismatch`: Puntuación por sustitución (ej: 0)
  - `gap`: Penalidad por gap (ej: -2)
- `-o <archivo.csv>`: Archivo de salida CSV (opcional, por defecto: `resultado.csv`)
- `-h, --help`: Muestra ayuda

#### Ejemplos

Ejecutar alineamiento con parámetros básicos:
```bash
./bin/main-secuencial -f datos/test.fasta -p 2 0 -2
```

Especificar archivo de salida:
```bash
./bin/main-secuencial -f datos/test.fasta -p 2 0 -2 -o resultado-test.csv
```

Alineamiento para proteínas con matriz BLOSUM:
```bash
./bin/main-secuencial -f datos/protein.fasta -p 5 -4 -10
```

### Script de Ejecución con Promedios (`ejecutar_promedio.sh`)

Script para ejecutar `main-secuencial` múltiples veces y calcular promedios y estadísticas de los tiempos y puntuación.

#### Uso básico
```bash
./ejecutar_promedio.sh -n <numero> -f <archivo.fasta> -p <match> <mismatch> <gap>
```

#### Parámetros obligatorios
- `-n, --numero <n>`: Número de ejecuciones para promediar
- `-f, --fasta <archivo>`: Archivo FASTA con las secuencias
- `-p <match> <mismatch> <gap>`: Parámetros de puntuación

#### Opciones
- `-o, --output <archivo>`: Archivo CSV de salida [default: `resultado_promedio.csv`]
- `-v, --verbose`: Mostrar salida detallada de cada ejecución
- `-h, --help`: Muestra ayuda

#### Ejemplos

Ejecutar 5 veces y calcular promedios:
```bash
./ejecutar_promedio.sh -n 5 -f datos/dna_1k.fasta -p 2 0 -2
```

Ejecutar 10 veces con salida personalizada:
```bash
./ejecutar_promedio.sh -n 10 -f datos/protein_5k.fasta -p 5 -4 -10 -o resultados.csv
```

Ejecutar con salida detallada:
```bash
./ejecutar_promedio.sh -n 3 -f datos/dna_2k.fasta -p 2 0 -2 -v
```

#### Verificación de Memoria

**NUEVO:** El script ahora verifica automáticamente la RAM disponible antes de ejecutar y calcula si la matriz de alineamiento cabrá en memoria.

- **Verificación previa**: Lee las longitudes de las secuencias del archivo FASTA
- **Cálculo de memoria**: Estima la memoria necesaria para la matriz (N+1) × (M+1)
- **Comparación**: Verifica si hay suficiente RAM disponible (usa 80% como umbral de seguridad)
- **Protección**: **Detiene la ejecución** si no hay suficiente memoria, evitando:
  - Ejecuciones de 50k sin suficiente RAM (requiere ~20 GB)
  - Ejecuciones de 100k sin suficiente RAM (requiere ~80 GB)
  - Crashes del sistema por falta de memoria

El script mostrará:
- Longitudes de las secuencias
- Memoria estimada necesaria
- RAM disponible en el sistema
- Advertencias para secuencias grandes (>5 GB)

#### Estadísticas generadas

El script calcula y muestra:
- **Promedio**: Tiempo/puntuación promedio de todas las ejecuciones
- **Mínimo**: Valor mínimo registrado
- **Máximo**: Valor máximo registrado
- **Desviación estándar**: Variabilidad de los resultados

Se generan estadísticas para:
- Tiempo de inicialización
- Tiempo de llenado de matriz
- Tiempo de traceback
- Tiempo total
- Puntuación

El script también genera dos archivos CSV:
- Archivo de estadísticas: Contiene los promedios y estadísticas calculadas
- Archivo individual: Contiene los datos de cada ejecución individual (sufijo `_individual.csv`)

### Script de Benchmark para Todos los Archivos (`benchmark_todos.sh`)

Script automatizado para ejecutar benchmarks de todos los archivos FASTA en un directorio, ejecutando promedios de múltiples veces para cada uno.

#### Uso básico
```bash
./benchmark_todos.sh
```

Este script:
- Busca todos los archivos `.fasta` en el directorio `datos/`
- Detecta automáticamente si son ADN o proteínas
- Ejecuta `ejecutar_promedio.sh` con 10 ejecuciones por defecto
- Crea un archivo `resumen_consolidado.csv` con todos los resultados

#### Opciones
- `-d, --datos <directorio>`: Directorio con archivos FASTA [default: `datos/`]
- `-n, --numero <n>`: Número de ejecuciones por archivo [default: 10]
- `-v, --verbose`: Mostrar salida detallada
- `--skip-errors`: Continuar automáticamente si hay error (sin preguntar)
- `--dna-params <m> <mm> <g>`: Parámetros para ADN: match mismatch gap [default: 2 0 -2]
- `--protein-params <m> <mm> <g>`: Parámetros para proteínas [default: 5 -4 -10]
- `-h, --help`: Muestra ayuda

#### Ejemplos

Ejecutar benchmarks de todos los archivos:
```bash
./benchmark_todos.sh
```

Ejecutar con 5 iteraciones por archivo:
```bash
./benchmark_todos.sh -n 5
```

Ejecutar con salida detallada y continuar aunque haya errores:
```bash
./benchmark_todos.sh -v --skip-errors
```

### Script de Análisis y Visualización (`analizar_benchmark.py`)

Script en Python para analizar los resultados de los benchmarks y generar gráficos que muestran:
- **Porcentajes de tiempo por fase**: Inicialización, llenado de matriz, traceback
- **Distribución de tiempos**: Gráficos comparativos entre fases
- **Análisis de cuello de botella**: Identifica qué fase consume más tiempo

#### Requisitos

Instalar las dependencias de Python:
```bash
pip install -r requirements.txt
```

O instalar manualmente:
```bash
pip install pandas matplotlib seaborn numpy
```

#### Uso básico
```bash
python3 analizar_benchmark.py
```

#### Opciones
- `-d, --directorio <dir>`: Directorio con archivos CSV de resultados [default: `resultados_benchmark/`]
- `-o, --output <dir>`: Directorio para guardar gráficos [default: `graficos_benchmark/`]
- `-a, --archivo-torta <archivo>`: Archivo específico para gráfico de torta (opcional)
- `-h, --help`: Muestra ayuda

#### Ejemplos

Analizar resultados con configuración por defecto:
```bash
python3 analizar_benchmark.py
```

Especificar directorios personalizados:
```bash
python3 analizar_benchmark.py -d mis_resultados/ -o mis_graficos/
```

Generar gráfico de torta para un archivo específico:
```bash
python3 analizar_benchmark.py -a dna_10k
```

#### Gráficos generados

El script genera los siguientes gráficos:

1. **`porcentajes_por_fase_apilado.png`**: Gráfico de barras apiladas mostrando el porcentaje del tiempo total que consume cada fase (inicialización, llenado, traceback) para cada archivo.

2. **`tiempos_absolutos_por_fase.png`**: Gráfico de líneas con escala logarítmica mostrando los tiempos absolutos (ms) de cada fase vs la longitud de las secuencias.

3. **`analisis_comparativo_fases.png`**: Panel de 4 gráficos comparativos:
   - Tiempos absolutos por fase
   - Porcentajes por fase
   - Porcentaje de tiempo en llenado de matriz (cuello de botella)
   - Tiempo total vs longitud (coloreado por % de llenado)

4. **`torta_<archivo>.png`**: Gráfico de torta para un archivo específico mostrando la distribución de tiempo por fase.

5. **`resumen_analisis.txt`**: Resumen en texto con estadísticas importantes, incluyendo:
   - Promedios de porcentaje por fase
   - Archivo con mayor % en llenado (cuello de botella principal)
   - Porcentajes detallados por archivo

#### Interpretación de resultados

El script identifica automáticamente que **el llenado de matriz (Fase 2)** es típicamente el cuello de botella, consumiendo normalmente entre 80-95% del tiempo total. Esto es esperado ya que:
- La fase de llenado tiene complejidad O(N×M) y requiere accesos a memoria
- La inicialización es O(N+M) y generalmente muy rápida
- El traceback es O(N+M) pero puede ser más lento que la inicialización debido a construcción de strings

## Formato de Salida

### Archivo CSV (`main-secuencial`)

El programa genera un archivo CSV con los siguientes campos:
- `archivo_fasta`: Nombre del archivo de entrada
- `longitud_A`: Longitud de la primera secuencia
- `longitud_B`: Longitud de la segunda secuencia
- `match`: Parámetro de puntuación usado
- `mismatch`: Parámetro de puntuación usado
- `gap`: Parámetro de puntuación usado
- `tiempo_init_ms`: Tiempo de inicialización (ms)
- `tiempo_llenado_ms`: Tiempo de llenado de la matriz DP (ms)
- `tiempo_traceback_ms`: Tiempo de traceback (ms)
- `tiempo_total_ms`: Tiempo total de ejecución (ms)
- `puntuacion`: Puntuación final del alineamiento

### Archivo FASTA (`main-gen-secuencia`)

El generador crea archivos FASTA estándar con dos secuencias:
```
>sec1
ATGCGATCGATCG...
>sec2
ATGCGATCGATCG...
```

## Optimización

Los programas se compilan con optimización `-O3` para máximo rendimiento. Esto es especialmente útil para:
- Secuencias largas
- Ejecuciones repetidas para benchmarking
- Comparación de rendimiento

## Estructura del Proyecto

```
AlgNW/
├── include/              # Archivos de cabecera (.h)
│   ├── tipos.h
│   ├── puntuacion.h
│   ├── secuencial.h
│   ├── paralelo.h
│   ├── generador_secuencias.h
│   └── utilidades.h
├── src/                  # Archivos fuente (.cpp)
│   ├── secuencial.cpp
│   ├── paralelo.cpp
│   ├── puntuacion.cpp
│   ├── generador_secuencias.cpp
│   └── utilidades.cpp
├── main-secuencial.cpp   # Programa principal secuencial
├── main-gen-secuencia.cpp # Programa generador
├── Makefile              # Archivo de compilación
├── generar_datos.sh      # Script para generar datos de benchmarking
├── ejecutar_promedio.sh  # Script para ejecutar con promedios
├── benchmark_todos.sh    # Script para ejecutar benchmarks de todos los archivos
├── analizar_benchmark.py # Script de análisis y visualización en Python
├── requirements.txt      # Dependencias de Python para el análisis
├── README.md             # Este archivo
├── bin/                  # Directorio de ejecutables (generado)
├── datos/                # Directorio de datos generados (generado)
├── resultados_benchmark/ # Directorio de resultados CSV (generado)
└── graficos_benchmark/   # Directorio de gráficos generados (generado)
```

## Notas

- Asegúrate de que el archivo FASTA tenga al menos 2 secuencias antes de ejecutar `main-secuencial`
- Los tiempos se miden en milisegundos (ms)
- El programa secuencial muestra información detallada en la consola además de guardar en CSV
- Para grandes volúmenes de datos, considera usar el modo batch del generador


