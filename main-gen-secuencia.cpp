/**
 * @file generar_sec.cpp
 * @brief Generador de secuencias biológicas para benchmarking
 */

#include "generador_secuencias.h"
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

void mostrarUso() {
    std::cout << "\nGenerador de Secuencias Biológicas\n";
    std::cout << "===================================\n\n";
    std::cout << "Uso:\n";
    std::cout << "  generar_sec -t <tipo> -l <longitud> -s <similitud> -o <salida>\n";
    std::cout << "  generar_sec -b -o <directorio>  # Generar lote\n\n";
    std::cout << "Opciones:\n";
    std::cout << "  -t, --tipo       Tipo de secuencia: dna, rna, proteina (protein)\n";
    std::cout << "  -l, --longitud   Longitud de las secuencias\n";
    std::cout << "  -s, --similitud  Similitud objetivo (0.0 - 1.0)\n";
    std::cout << "  -o, --salida     Prefijo del archivo de salida o directorio\n";
    std::cout << "  -b, --batch      Generar lote de secuencias\n";
    std::cout << "  -h, --ayuda      Mostrar esta ayuda\n\n";
    std::cout << "Ejemplos:\n";
    std::cout << "  generar_sec -t dna -l 100 -s 0.9 -o datos/test\n";
    std::cout << "  generar_sec -b -o datos/\n\n";
}

int main(int argc, char* argv[]) {
    // Valores por defecto
    std::string tipo_str = "dna";
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
        else if ((arg == "-t" || arg == "--tipo") && i + 1 < argc) {
            tipo_str = argv[++i];
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

    // Determinar tipo de secuencia
    TipoSecuencia tipo = TipoSecuencia::DNA;
    if (tipo_str == "rna") {
        tipo = TipoSecuencia::RNA;
    } else if (tipo_str == "proteina" || tipo_str == "protein") {
        tipo = TipoSecuencia::PROTEINA;
    } else if (tipo_str != "dna") {
        std::cerr << "Error: Tipo inválido. Use: dna, rna, o proteina\n";
        return 1;
    }

    if (modo_lote) {
        // Generar lote de secuencias
        std::vector<int> longitudes = {50, 100, 200, 500, 1000};
        std::vector<double> similitudes = {0.5, 0.7, 0.85, 0.9, 0.95, 0.99};
        
        std::cout << "Generando lote de secuencias...\n";
        std::cout << "Directorio: " << salida << "\n";
        std::cout << "Tipo: " << tipo_str << "\n\n";
        
        int generados = generarLoteSecuencias(salida, longitudes, similitudes, tipo);
        
        std::cout << "\n✓ Generados " << generados << " archivos\n";
    } else {
        // Generar un par de secuencias
        std::cout << "Generando par de secuencias...\n";
        std::cout << "Tipo: " << tipo_str << "\n";
        std::cout << "Longitud: " << longitud << "\n";
        std::cout << "Similitud objetivo: " << similitud << "\n\n";
        
        ParSecuencias par = generarParSecuencias(longitud, similitud, tipo);
        
        std::string nombreArchivo = salida + ".fasta";
        if (guardarParSecuenciasFASTA(par, nombreArchivo)) {
            std::cout << "✓ Secuencias guardadas en: " << nombreArchivo << "\n";
            std::cout << "  Similitud real: " << par.similitud_real << "\n";
        } else {
            std::cerr << "✗ Error al guardar secuencias\n";
            return 1;
        }
    }

    return 0;
}
