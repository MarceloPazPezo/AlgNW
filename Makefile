# Makefile para compilar los programas del proyecto AlgNW (versión simplificada DNA)
# Compilador
CXX = g++

# Flags de compilación (mismos que el Makefile principal)
CXXFLAGS = -O3 -Wall -std=c++11 -Iinclude -Isrc
CXXFLAGS_PARALELO = $(CXXFLAGS) -fopenmp

# Soporte opcional para Extrae (solo si está instalado)
# Para habilitar: make EXTRAE=1 o export EXTRAE_HOME=/ruta/a/extrae
ifdef EXTRAE_HOME
  EXTRAE_INC = -I$(EXTRAE_HOME)/include
  EXTRAE_LIBS = -L$(EXTRAE_HOME)/lib -lseqtrace -lomptrace -lpttrace -lrt -lpthread -ldl
  CXXFLAGS += -DHAVE_EXTRAE $(EXTRAE_INC)
  CXXFLAGS_PARALELO += -DHAVE_EXTRAE $(EXTRAE_INC)
  LDFLAGS_EXTRAE = $(EXTRAE_LIBS)
else
  ifdef EXTRAE
    # Intentar detectar Extrae automáticamente
    EXTRAE_HOME ?= /usr
    EXTRAE_INC = -I$(EXTRAE_HOME)/include
    # Librerías de Extrae necesarias para eventos de usuario
    EXTRAE_LIBS = -L$(EXTRAE_HOME)/lib -lseqtrace -lomptrace -lpttrace -lrt -lpthread -ldl
    CXXFLAGS += -DHAVE_EXTRAE $(EXTRAE_INC)
    CXXFLAGS_PARALELO += -DHAVE_EXTRAE $(EXTRAE_INC)
    LDFLAGS_EXTRAE = $(EXTRAE_LIBS)
  endif
endif

# Directorios
SRC_DIR = src
BIN_DIR = bin

# Archivos fuente (con ruta desde src/)
SECUENCIAL_SRCS = $(SRC_DIR)/main-secuencial.cpp \
                  $(SRC_DIR)/secuencial.cpp \
                  $(SRC_DIR)/puntuacion.cpp \
                  $(SRC_DIR)/utilidades.cpp

PARALELO_SRCS = $(SRC_DIR)/main-paralelo.cpp \
                $(SRC_DIR)/secuencial.cpp \
                $(SRC_DIR)/paralelo.cpp \
                $(SRC_DIR)/puntuacion.cpp \
                $(SRC_DIR)/utilidades.cpp

GENERADOR_SRCS = $(SRC_DIR)/main-gen-secuencia.cpp \
                 $(SRC_DIR)/generador_secuencias.cpp

# Objetivos principales
all: $(BIN_DIR)/main-secuencial $(BIN_DIR)/main-paralelo $(BIN_DIR)/main-gen-secuencia

# Crear directorio bin si no existe
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Compilar main-secuencial
$(BIN_DIR)/main-secuencial: $(SECUENCIAL_SRCS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(SECUENCIAL_SRCS)

# Compilar main-paralelo (requiere OpenMP)
$(BIN_DIR)/main-paralelo: $(PARALELO_SRCS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS_PARALELO) -o $@ $(PARALELO_SRCS) $(LDFLAGS_EXTRAE)

# Compilar main-gen-secuencia
$(BIN_DIR)/main-gen-secuencia: $(GENERADOR_SRCS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(GENERADOR_SRCS)

# Limpiar archivos compilados
clean:
	rm -rf $(BIN_DIR)

# Recompilar todo desde cero
rebuild: clean all

# Ayuda
help:
	@echo "Makefile para compilar programas del proyecto AlgNW (srcv2 - DNA)"
	@echo ""
	@echo "Objetivos disponibles:"
	@echo "  make                    - Compila todos los programas"
	@echo "  make all                - Compila todos los programas"
	@echo "  make clean              - Elimina los archivos compilados"
	@echo "  make rebuild            - Limpia y recompila todo"
	@echo "  make help               - Muestra esta ayuda"
	@echo ""
	@echo "Programas generados:"
	@echo "  bin/main-secuencial     - Alineamiento Needleman-Wunsch secuencial"
	@echo "  bin/main-paralelo       - Benchmark comparativo (secuencial vs paralelo)"
	@echo "  bin/main-gen-secuencia  - Generador de secuencias DNA"
	@echo ""
	@echo "Variables de entorno OpenMP:"
	@echo "  export OMP_NUM_THREADS=8        # Número de threads"
	@echo "  export OMP_SCHEDULE=\"dynamic,1\"  # Planificador"
	@echo ""
	@echo "Compilación con Extrae (opcional):"
	@echo "  make EXTRAE=1                  # Compilar con soporte Extrae"
	@echo "  make EXTRAE_HOME=/ruta/extrae   # Especificar ruta de Extrae"

.PHONY: all clean rebuild help

