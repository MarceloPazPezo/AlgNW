# Guía de Instalación

## Requisitos del Sistema

### Compilador C++
- **g++** (GNU Compiler Collection) versión 4.8 o superior
- O **clang++** versión 3.4 o superior

### Librerías
- **OpenMP** (para las implementaciones paralelas)

## Instalación en Linux

### Ubuntu/Debian

```bash
# Instalar compilador y herramientas
sudo apt-get update
sudo apt-get install build-essential g++

# Instalar OpenMP (generalmente viene con g++)
sudo apt-get install libomp-dev

# Verificar instalación
g++ --version
```

### CentOS/RHEL/Fedora

```bash
# Instalar compilador y herramientas
sudo yum install gcc-c++ make

# O en versiones más recientes:
sudo dnf install gcc-c++ make

# Verificar instalación
g++ --version
```

### Arch Linux

```bash
# Instalar compilador
sudo pacman -S gcc make

# Verificar instalación
g++ --version
```

## Instalación de Make (si no está disponible)

### Ubuntu/Debian
```bash
sudo apt-get install build-essential
```

### CentOS/RHEL
```bash
sudo yum install make
```

### Sin Make (usar compilar.sh)

Si no puedes instalar `make`, el proyecto incluye `compilar.sh` que compila directamente:

```bash
# Dar permisos de ejecución
chmod +x compilar.sh

# Compilar todo
./compilar.sh

# O compilar solo lo necesario
./compilar.sh benchmark
./compilar.sh generador
./compilar.sh secuencial
```

## Compilación del Proyecto

### Opción 1: Usando Make (recomendado si está disponible)

```bash
# Compilar todo
make

# Compilar solo benchmark
make bin/main-benchmark

# Limpiar archivos compilados
make clean
```

### Opción 2: Usando compilar.sh (sin make)

```bash
# Dar permisos de ejecución
chmod +x compilar.sh

# Compilar todo
./compilar.sh

# Compilar solo benchmark
./compilar.sh benchmark

# Limpiar
./compilar.sh clean
```

### Opción 3: Compilación manual

```bash
# Crear directorio bin
mkdir -p bin

# Compilar benchmark
g++ -O3 -Wall -std=c++11 -Iinclude -fopenmp -o bin/main-benchmark \
    main-benchmark.cpp \
    src/secuencial.cpp \
    src/paralelo.cpp \
    src/puntuacion.cpp \
    src/utilidades.cpp

# Compilar secuencial
g++ -O3 -Wall -std=c++11 -Iinclude -o bin/main-secuencial \
    main-secuencial.cpp \
    src/secuencial.cpp \
    src/puntuacion.cpp \
    src/utilidades.cpp

# Compilar generador
g++ -O3 -Wall -std=c++11 -Iinclude -o bin/main-gen-secuencia \
    main-gen-secuencia.cpp \
    src/generador_secuencias.cpp
```

## Verificación de la Instalación

```bash
# Verificar compilador
g++ --version

# Verificar OpenMP
g++ -fopenmp --version

# Compilar y probar
./compilar.sh
./bin/main-secuencial -h
```

## Solución de Problemas

### Error: "g++: command not found"
- Instala el compilador: `sudo apt-get install g++` (Ubuntu) o `sudo yum install gcc-c++` (CentOS)

### Error: "fatal error: omp.h: No such file or directory"
- Instala OpenMP: `sudo apt-get install libomp-dev` (Ubuntu) o `sudo yum install libgomp` (CentOS)

### Error: "make: command not found"
- Usa `./compilar.sh` en su lugar, o instala make: `sudo apt-get install build-essential`

### Error de permisos en scripts
```bash
chmod +x compilar.sh
chmod +x ejecutar.sh
chmod +x generar_casos_*.sh
chmod +x ejecutar_benchmark_*.sh
```

## Herramientas Opcionales

### Para análisis de rendimiento

- **Extrae/Paraver**: `sudo apt-get install extrae paraver`
- **Intel VTune**: Descargar desde Intel
- **perf**: Ya incluido en Linux kernel

Ver `HERRAMIENTAS_ANALISIS.md` para más detalles.

