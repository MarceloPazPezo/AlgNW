/**
 * @file main-gen-secuencia.cpp
 * @brief Generador de secuencias DNA para benchmarking
 * 
 * Uso:
 *   ./main-gen-secuencia -l <longitud> -s <similitud> -o <salida>
 *   ./main-gen-secuencia -b -o <directorio>  # Generar lote
 */

#include "generador_secuencias.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstdlib>

void mostrarUso() {
    std::cout << "\nGenerador de Secuencias DNA\n";
    std::cout << "===========================\n\n";
    std::cout << "Uso:\n";
    std::cout << "  main-gen-secuencia -l <longitud> -s <similitud> -o <salida>\n";
    std::cout << "  main-gen-secuencia -b -o <directorio>  # Generar lote\n\n";
    std::cout << "Opciones:\n";
    std::cout << "  -l, --longitud   Longitud de las secuencias DNA\n";
    std::cout << "  -s, --similitud  Similitud objetivo (0.0 - 1.0)\n";
    std::cout << "  -o, --salida     Prefijo del archivo de salida o directorio\n";
    std::cout << "  -b, --batch      Generar lote de secuencias\n";
    std::cout << "  -h, --ayuda      Mostrar esta ayuda\n\n";
    std::cout << "Ejemplos:\n";
    std::cout << "  main-gen-secuencia -l 100 -s 0.9 -o datos/test\n";
    std::cout << "  main-gen-secuencia -b -o datos/\n\n";
}

int main(int argc, char* argv[]) {
    // Valores por defecto
    int longitud = 100;
    double similitud = 0.9;
    std::string salida = "secuencias";
    bool modo_lote = false;

    // Parsear argumentos
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--ayuda") {
            mostrarUso();
            return 0;
        }
        else if ((arg == "-l" || arg == "--longitud") && i + 1 < argc) {
            longitud = std::atoi(argv[++i]);
            if (longitud <= 0) {
                std::cerr << "Error: La longitud debe ser positiva\n";
                return 1;
            }
        }
        else if ((arg == "-s" || arg == "--similitud") && i + 1 < argc) {
            similitud = std::atof(argv[++i]);
            if (similitud < 0.0 || similitud > 1.0) {
                std::cerr << "Error: La similitud debe estar entre 0.0 y 1.0\n";
                return 1;
            }
        }
        else if ((arg == "-o" || arg == "--salida") && i + 1 < argc) {
            salida = argv[++i];
        }
        else if (arg == "-b" || arg == "--batch") {
            modo_lote = true;
        }
    }

    if (modo_lote) {
        // Generar lote de secuencias DNA
        std::vector<int> longitudes = {50, 100, 200, 500, 1000, 2000, 5000};
        std::vector<double> similitudes = {0.5, 0.7, 0.85, 0.9, 0.95, 0.99};
        
        std::cout << "Generando lote de secuencias DNA...\n";
        std::cout << "Directorio: " << salida << "\n\n";
        
        int generados = generarLoteSecuenciasDNA(salida, longitudes, similitudes);
        
        std::cout << "\n✓ Generados " << generados << " archivos\n";
    } else {
        // Generar un par de secuencias DNA
        std::cout << "Generando par de secuencias DNA...\n";
        std::cout << "Longitud: " << longitud << "\n";
        std::cout << "Similitud objetivo: " << similitud << "\n\n";
        
        ParSecuenciasDNA par = generarParSecuenciasDNA(longitud, similitud);
        
        std::string nombreArchivo = salida + ".fasta";
        if (guardarParSecuenciasDNAFASTA(par, nombreArchivo)) {
            std::cout << "✓ Secuencias guardadas en: " << nombreArchivo << "\n";
            std::cout << "  Similitud real: " << std::fixed << std::setprecision(4) 
                      << par.similitud_real << "\n";
        } else {
            std::cerr << "✗ Error al guardar secuencias\n";
            return 1;
        }
    }

    return 0;
}

