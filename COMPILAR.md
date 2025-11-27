# Comandos de Compilación Directa

## Compilar Benchmark (con OpenMP)

```bash
mkdir -p bin

g++ -O3 -Wall -std=c++11 -Iinclude -fopenmp -o bin/main-benchmark \
    main-benchmark.cpp \
    src/secuencial.cpp \
    src/paralelo.cpp \
    src/puntuacion.cpp \
    src/utilidades.cpp
```

## Compilar Secuencial

```bash
mkdir -p bin

g++ -O3 -Wall -std=c++11 -Iinclude -o bin/main-secuencial \
    main-secuencial.cpp \
    src/secuencial.cpp \
    src/puntuacion.cpp \
    src/utilidades.cpp
```

## Compilar Generador de Secuencias

```bash
mkdir -p bin

g++ -O3 -Wall -std=c++11 -Iinclude -o bin/main-gen-secuencia \
    main-gen-secuencia.cpp \
    src/generador_secuencias.cpp
```

## Compilar Todo

```bash
# Crear directorio
mkdir -p bin

# Benchmark
g++ -O3 -Wall -std=c++11 -Iinclude -fopenmp -o bin/main-benchmark \
    main-benchmark.cpp src/secuencial.cpp src/paralelo.cpp \
    src/puntuacion.cpp src/utilidades.cpp

# Secuencial
g++ -O3 -Wall -std=c++11 -Iinclude -o bin/main-secuencial \
    main-secuencial.cpp src/secuencial.cpp \
    src/puntuacion.cpp src/utilidades.cpp

# Generador
g++ -O3 -Wall -std=c++11 -Iinclude -o bin/main-gen-secuencia \
    main-gen-secuencia.cpp src/generador_secuencias.cpp
```

## Explicación de Flags

- `-O3`: Optimización máxima
- `-Wall`: Mostrar todas las advertencias
- `-std=c++11`: Estándar C++11
- `-Iinclude`: Incluir directorio de headers
- `-fopenmp`: Habilitar OpenMP (solo para benchmark y paralelo)

## Verificar Compilación

```bash
# Verificar que se compilaron
ls -lh bin/

# Probar ejecución
./bin/main-secuencial -h
./bin/main-benchmark -h
```

