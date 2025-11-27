# Guía de Uso de Extrae y Paraver

## ¿Qué son Extrae y Paraver?

**Extrae** y **Paraver** son herramientas del Barcelona Supercomputing Center (BSC) para análisis de rendimiento de aplicaciones paralelas:

- **Extrae**: Instrumenta la ejecución y genera trazas de rendimiento (archivos `.prv`)
- **Paraver**: Visualiza las trazas y permite crear gráficas de comportamiento

## Instalación

### En Linux

```bash
# Opción 1: Desde repositorios (si están disponibles)
sudo apt-get install extrae paraver  # Debian/Ubuntu
sudo yum install extrae paraver      # CentOS/RHEL

# Opción 2: Compilar desde fuente
# Descargar desde: https://tools.bsc.es/downloads
```

### En Windows

Extrae/Paraver están diseñados principalmente para Linux. En Windows puedes:
- Usar WSL (Windows Subsystem for Linux)
- Usar una máquina virtual con Linux
- Usar herramientas alternativas como Intel VTune

## Configuración para el Proyecto

### 1. Crear archivo de configuración de Extrae

Crea un archivo `extrae.xml` en la raíz del proyecto:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<trace>
  <enabled_tasks>
    <task enabled="yes" name="openmp"/>
    <task enabled="yes" name="pthread"/>
  </enabled_tasks>
  
  <openmp enabled="yes">
    <trace_io enabled="no"/>
    <trace_region enabled="yes"/>
    <trace_loop enabled="yes"/>
    <trace_wait_clause enabled="yes"/>
    <trace_taskloop enabled="yes"/>
    <trace_task enabled="yes"/>
    <trace_lock enabled="yes"/>
    <trace_deps enabled="yes"/>
  </openmp>
  
  <sampling enabled="no"/>
  
  <mpi enabled="no"/>
  
  <output_dir>./traces</output_dir>
  <output_prefix>nw_trace</output_prefix>
</trace>
```

### 2. Compilar con soporte de Extrae

Modifica el Makefile para incluir las librerías de Extrae:

```makefile
# En el Makefile, agregar:
EXTRAE_LIBS = -L$(EXTRAE_HOME)/lib -lseqtrace -lomptrace -lpttrace -lrt -lpthread -ldl
EXTRAE_INC = -I$(EXTRAE_HOME)/include

# Para el benchmark con Extrae:
CXXFLAGS_EXTRAE = $(CXXFLAGS_PARALELO) $(EXTRAE_INC)
LDFLAGS_EXTRAE = $(EXTRAE_LIBS)
```

### 3. Instrumentar el código

Extrae funciona automáticamente con OpenMP, pero puedes agregar etiquetas personalizadas:

```cpp
#include "extrae_user_events.h"

// Al inicio de una región importante
EXTRAE_USER_EVENT(1, 1);  // Etiqueta: Inicio de fase 2

// Tu código aquí...

EXTRAE_USER_EVENT(1, 0);  // Etiqueta: Fin de fase 2
```

## Uso Básico

### Ejecutar con Extrae

```bash
# Configurar variable de entorno
export EXTRAE_CONFIG_FILE=./extrae.xml

# Ejecutar el programa
./bin/main-benchmark -f datos/test_500.fasta -p 2 0 -2 -r 1
```

Esto generará archivos `.prv` en el directorio `traces/`.

### Visualizar con Paraver

```bash
# Abrir Paraver
paraver traces/nw_trace.prv

# O desde la línea de comandos
paraver traces/nw_trace.prv &
```

## Análisis con Paraver

### Vistas Útiles

1. **Timeline de Threads**: Ver cómo se distribuye el trabajo entre threads
   - Menú: `View > Timeline`
   - Muestra cada thread como una línea horizontal
   - Colores indican diferentes estados (ejecutando, esperando, etc.)

2. **Estadísticas de OMP**: Analizar comportamiento de OpenMP
   - Menú: `Statistics > OMP`
   - Muestra tiempo en regiones paralelas, overhead, etc.

3. **Comparación de Schedules**: Comparar diferentes schedules
   - Ejecutar cada método por separado
   - Cargar múltiples trazas en Paraver
   - Comparar visualmente la distribución de trabajo

### Etiquetas Personalizadas

Si agregaste etiquetas en el código, aparecerán en Paraver como eventos de usuario, permitiendo identificar:
- Inicio/fin de fases específicas
- Regiones de código importantes
- Puntos de sincronización

## Script de Automatización

Ver `ejecutar_extrae.sh` para un script que automatiza la ejecución con Extrae.

## Interpretación de Resultados

### Para Análisis de Schedules

1. **STATIC**: Deberías ver bloques uniformes de trabajo asignados a cada thread
2. **DYNAMIC**: Verás trabajo distribuido de forma más irregular, con threads tomando trabajo cuando terminan
3. **GUIDED**: Similar a dynamic pero con chunks que disminuyen exponencialmente
4. **AUTO**: Depende de la implementación del compilador

### Métricas Importantes

- **Load Balance**: ¿Todos los threads trabajan igual?
- **Overhead**: Tiempo gastado en sincronización
- **Idle Time**: Tiempo que los threads están esperando

## Alternativas en Windows

Si estás en Windows y no puedes usar Extrae/Paraver:

1. **Intel VTune Profiler**: Similar funcionalidad, funciona en Windows
2. **Visual Studio Profiler**: Herramienta integrada para análisis de OpenMP
3. **WSL**: Ejecutar en Windows Subsystem for Linux

## Referencias

- Documentación oficial: https://tools.bsc.es/doc/extrae
- Tutoriales: https://tools.bsc.es/doc/paraver
- Ejemplos: https://github.com/bsc-performance-tools

