# Makefile para compilar los programas principales del proyecto AlgNW
# Compilador
CXX = g++

# Flags de compilación
CXXFLAGS = -O3 -Wall -std=c++11 -Iinclude
CXXFLAGS_PARALELO = $(CXXFLAGS) -fopenmp

# Directorios
SRC_DIR = src
INCLUDE_DIR = include
BIN_DIR = bin

# Archivos fuente
SECUENCIAL_SRCS = main-secuencial.cpp \
                  $(SRC_DIR)/secuencial.cpp \
                  $(SRC_DIR)/puntuacion.cpp \
                  $(SRC_DIR)/utilidades.cpp

BENCHMARK_SRCS = main-benchmark.cpp \
                 $(SRC_DIR)/secuencial.cpp \
                 $(SRC_DIR)/paralelo.cpp \
                 $(SRC_DIR)/puntuacion.cpp \
                 $(SRC_DIR)/utilidades.cpp

# Objetivos principales
all: $(BIN_DIR)/main-secuencial $(BIN_DIR)/main-benchmark

# Crear directorio bin si no existe
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Compilar main-secuencial
$(BIN_DIR)/main-secuencial: $(SECUENCIAL_SRCS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(SECUENCIAL_SRCS)

# Compilar main-benchmark
$(BIN_DIR)/main-benchmark: $(BENCHMARK_SRCS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS_PARALELO) -o $@ $(BENCHMARK_SRCS)

# Limpiar archivos compilados
clean:
	rm -rf $(BIN_DIR)

# Recompilar todo desde cero
rebuild: clean all

# Ayuda
help:
	@echo "Makefile para compilar programas del proyecto AlgNW"
	@echo ""
	@echo "Objetivos disponibles:"
	@echo "  make              - Compila todos los programas"
	@echo "  make all          - Compila todos los programas"
	@echo "  make clean        - Elimina los archivos compilados"
	@echo "  make rebuild      - Limpia y recompila todo"
	@echo "  make help         - Muestra esta ayuda"
	@echo ""
	@echo "Programas generados:"
	@echo "  bin/main-secuencial      - Alineamiento Needleman-Wunsch secuencial"
	@echo "  bin/main-benchmark       - Benchmark comparativo de todas las implementaciones"

.PHONY: all clean rebuild help


