# Makefile para compilar los programas principales del proyecto AlgNW
# Compilador
CXX = g++

# Flags de compilación
CXXFLAGS = -O3 -Wall -std=c++11 -Iinclude

# Directorios
SRC_DIR = src
INCLUDE_DIR = include
BIN_DIR = bin

# Archivos fuente
SECUENCIAL_SRCS = main-secuencial.cpp \
                  $(SRC_DIR)/secuencial.cpp \
                  $(SRC_DIR)/puntuacion.cpp \
                  $(SRC_DIR)/utilidades.cpp

GEN_SRCS = main-gen-secuencia.cpp \
           $(SRC_DIR)/generador_secuencias.cpp

# Objetivos principales
all: $(BIN_DIR)/main-secuencial $(BIN_DIR)/main-gen-secuencia

# Crear directorio bin si no existe
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Compilar main-secuencial
$(BIN_DIR)/main-secuencial: $(SECUENCIAL_SRCS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(SECUENCIAL_SRCS)

# Compilar main-gen-secuencia
$(BIN_DIR)/main-gen-secuencia: $(GEN_SRCS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(GEN_SRCS)

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
	@echo "  bin/main-gen-secuencia   - Generador de secuencias biológicas"

.PHONY: all clean rebuild help


