# Herramientas de Análisis de Rendimiento para Aplicaciones Paralelas

Este documento compara diferentes herramientas para analizar el rendimiento de aplicaciones OpenMP, especialmente para el algoritmo Needleman-Wunsch.

## Tabla Comparativa Rápida

| Herramienta | Plataforma | OpenMP | Facilidad | Visualización | Licencia |
|------------|------------|--------|-----------|---------------|----------|
| **Extrae/Paraver** | Linux | ✅ Excelente | Media | Excelente | Open Source |
| **Intel VTune** | Linux/Windows | ✅ Excelente | Fácil | Excelente | Comercial (gratis limitado) |
| **Score-P/Vampir** | Linux | ✅ Excelente | Media | Excelente | Open Source |
| **HPCToolkit** | Linux | ✅ Buena | Media | Buena | Open Source |
| **TAU** | Linux | ✅ Buena | Difícil | Media | Open Source |
| **perf + FlameGraph** | Linux | ⚠️ Básico | Fácil | Buena | Open Source |
| **Visual Studio Profiler** | Windows | ✅ Buena | Fácil | Buena | Comercial |
| **AMD uProf** | Linux/Windows | ✅ Buena | Fácil | Buena | Gratis |

---

## 1. Intel VTune Profiler ⭐ (Recomendado para Windows)

### Ventajas
- ✅ Funciona en **Windows y Linux**
- ✅ Interfaz gráfica muy intuitiva
- ✅ Excelente soporte para OpenMP
- ✅ Análisis de hotspots, memory access, threading
- ✅ Versión gratuita disponible (con limitaciones)

### Desventajas
- ⚠️ Requiere registro en Intel
- ⚠️ Algunas funciones avanzadas son de pago

### Instalación

```bash
# Descargar desde: https://www.intel.com/content/www/us/en/developer/tools/oneapi/vtune-profiler.html
# O instalar vía oneAPI:
wget https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB
sudo apt-key add GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB
sudo sh -c 'echo deb https://apt.repos.intel.com/oneapi all main > /etc/apt/sources.list.d/oneAPI.list'
sudo apt-get update
sudo apt-get install intel-oneapi-vtune
```

### Uso

```bash
# Análisis básico
vtune -collect hotspots -result-dir ./vtune_results -- ./bin/main-benchmark -f datos/test_500.fasta -p 2 0 -2

# Análisis de threading (OpenMP)
vtune -collect threading -result-dir ./vtune_threading -- ./bin/main-benchmark -f datos/test_500.fasta -p 2 0 -2

# Visualizar resultados
vtune -report summary -result-dir ./vtune_results
```

### Interfaz Gráfica

```bash
vtune-gui ./vtune_results
```

---

## 2. Score-P + Vampir

### Ventajas
- ✅ Open Source
- ✅ Excelente para análisis detallado
- ✅ Soporte para OpenMP, MPI, CUDA
- ✅ Visualización muy potente con Vampir

### Desventajas
- ⚠️ Instalación más compleja
- ⚠️ Curva de aprendizaje media

### Instalación

```bash
# Ubuntu/Debian
sudo apt-get install scorep vampir

# O desde fuente
# https://www.vi-hps.org/projects/score-p/
```

### Uso

```bash
# Compilar con Score-P
scorep-gcc -O3 -fopenmp -Iinclude -o bin/main-benchmark main-benchmark.cpp src/*.cpp

# Ejecutar
export SCOREP_ENABLE_PROFILING=true
export SCOREP_ENABLE_TRACING=true
./bin/main-benchmark -f datos/test_500.fasta -p 2 0 -2

# Visualizar con Vampir
vampir scorep-*/profile.cubex
```

---

## 3. HPCToolkit

### Ventajas
- ✅ Open Source
- ✅ Análisis de muestreo (sampling)
- ✅ Visualización de call trees
- ✅ Bajo overhead

### Desventajas
- ⚠️ Principalmente para Linux
- ⚠️ Visualización menos rica que Paraver/Vampir

### Instalación

```bash
# Desde repositorios (si disponible)
sudo apt-get install hpctoolkit

# O desde fuente: http://hpctoolkit.org/
```

### Uso

```bash
# Ejecutar con HPCToolkit
hpcrun -o hpctoolkit-results ./bin/main-benchmark -f datos/test_500.fasta -p 2 0 -2

# Analizar
hpcviewer hpctoolkit-results
```

---

## 4. TAU (Tuning and Analysis Utilities)

### Ventajas
- ✅ Muy completo y flexible
- ✅ Soporte para múltiples paradigmas
- ✅ Open Source

### Desventajas
- ⚠️ Configuración compleja
- ⚠️ Curva de aprendizaje alta

### Instalación

```bash
# Desde repositorios
sudo apt-get install tau

# O desde fuente: http://www.cs.uoregon.edu/research/tau/
```

### Uso

```bash
# Compilar con TAU
tau_cxx.sh -O3 -fopenmp -Iinclude -o bin/main-benchmark main-benchmark.cpp src/*.cpp

# Ejecutar
./bin/main-benchmark -f datos/test_500.fasta -p 2 0 -2

# Visualizar
paraprof
```

---

## 5. perf + FlameGraph (Linux)

### Ventajas
- ✅ Ya incluido en Linux
- ✅ Muy fácil de usar
- ✅ Visualización con FlameGraph
- ✅ Bajo overhead

### Desventajas
- ⚠️ Análisis de OpenMP limitado
- ⚠️ No muestra timeline de threads

### Instalación

```bash
# perf ya viene con Linux
# Instalar FlameGraph
git clone https://github.com/brendangregg/FlameGraph.git
```

### Uso

```bash
# Perfilado básico
perf record -g ./bin/main-benchmark -f datos/test_500.fasta -p 2 0 -2
perf report

# Con FlameGraph
perf record -g ./bin/main-benchmark -f datos/test_500.fasta -p 2 0 -2
perf script | ./FlameGraph/stackcollapse-perf.pl | ./FlameGraph/flamegraph.pl > flamegraph.svg
```

---

## 6. Visual Studio Profiler (Windows)

### Ventajas
- ✅ Integrado en Visual Studio
- ✅ Muy fácil de usar
- ✅ Excelente para desarrollo en Windows

### Desventajas
- ⚠️ Solo Windows
- ⚠️ Requiere Visual Studio

### Uso

1. Abrir proyecto en Visual Studio
2. Menú: `Debug > Performance Profiler`
3. Seleccionar "CPU Usage" o "Concurrency"
4. Ejecutar

---

## 7. AMD uProf (AMD Processors)

### Ventajas
- ✅ Gratis
- ✅ Funciona en Windows y Linux
- ✅ Específico para procesadores AMD

### Desventajas
- ⚠️ Optimizado para AMD (puede funcionar en Intel pero menos preciso)

### Instalación

```bash
# Descargar desde: https://developer.amd.com/amd-uprof/
```

---

## Recomendaciones por Plataforma

### Windows
1. **Intel VTune Profiler** (mejor opción general)
2. **Visual Studio Profiler** (si usas VS)
3. **AMD uProf** (si tienes procesador AMD)

### Linux
1. **Extrae/Paraver** (si ya lo tienes instalado)
2. **Intel VTune Profiler** (más fácil de usar)
3. **Score-P/Vampir** (más potente, más complejo)
4. **perf + FlameGraph** (rápido y simple)

### Para tu Proyecto Específico

Dado que estás comparando diferentes schedules de OpenMP:

**Mejor opción**: **Intel VTune Profiler**
- Muestra claramente la distribución de trabajo entre threads
- Fácil comparación entre diferentes schedules
- Funciona en Windows (tu plataforma actual)

**Alternativa simple**: **perf + FlameGraph**
- Si solo necesitas identificar hotspots
- Muy rápido de usar
- No requiere instalación compleja

---

## Scripts de Ejemplo

Ver los siguientes scripts en el proyecto:
- `ejecutar_vtune.sh` - Para Intel VTune
- `ejecutar_perf.sh` - Para perf + FlameGraph
- `ejecutar_extrae.sh` - Para Extrae/Paraver (ya creado)

---

## Referencias

- Intel VTune: https://www.intel.com/content/www/us/en/developer/tools/oneapi/vtune-profiler.html
- Score-P/Vampir: https://www.vi-hps.org/
- HPCToolkit: http://hpctoolkit.org/
- TAU: http://www.cs.uoregon.edu/research/tau/
- FlameGraph: https://github.com/brendangregg/FlameGraph

