/**
 * @file main-secuencial.cpp
 * @brief Programa para ejecutar el algoritmo Needleman-Wunsch de forma secuencial (DNA)
 * 
 * Uso:
 *   ./main-secuencial -f archivo.fasta -p <match> <mismatch> <gap> [-o salida.csv]
 * 
 * Ejemplo:
 *   ./main-secuencial -f datos/test.fasta -p 2 -1 -2 -o resultado.csv
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#include <vector>
#include <chrono>
#include "tipos.h"
#include "puntuacion.h"
#include "secuencial.h"
#include "utilidades.h"

/**
 * @brief Guarda resultados en CSV
 */
void guardarCSV(const std::string& archivo_salida,
                const std::string& archivo_fasta,
                const ResultadoAlineamiento& resultado,
                int match, int mismatch, int gap) {
    
    std::ofstream csv(archivo_salida, std::ios::app);
    
    if (!csv.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo " << archivo_salida << "\n";
        return;
    }
    
    csv.seekp(0, std::ios::end);
    bool archivo_vacio = csv.tellp() == 0;
    
    if (archivo_vacio) {
        csv << "archivo_fasta,metodo,repeticion,threads,schedule,longitud_A,longitud_B,match,mismatch,gap";
        csv << ",tiempo_init_ms,tiempo_llenado_ms,tiempo_traceback_ms,tiempo_total_ms,puntuacion\n";
    }
    
    double tiempo_total = resultado.tiempo_fase1_ms + resultado.tiempo_fase2_ms + resultado.tiempo_fase3_ms;
    
    csv << archivo_fasta << ",";
    csv << "secuencial,";  // método
    csv << "1,";  // repeticion (siempre 1 para secuencial)
    csv << "1,";  // threads (siempre 1 para secuencial)
    csv << "N/A,";  // schedule (no aplica para secuencial)
    csv << resultado.secA.length() << "," << resultado.secB.length() << ",";
    csv << match << "," << mismatch << "," << gap << ",";
    csv << std::fixed << std::setprecision(4);
    csv << resultado.tiempo_fase1_ms << "," << resultado.tiempo_fase2_ms << "," << resultado.tiempo_fase3_ms << ",";
    csv << tiempo_total << ",";
    csv << resultado.puntuacion << "\n";
    
    csv.close();
}

/**
 * @brief Muestra el uso del programa
 */
void mostrarUso(const char* nombre_programa) {
    std::cout << "Uso: " << nombre_programa << " [opciones]\n\n";
    std::cout << "Opciones:\n";
    std::cout << "  -f <archivo.fasta>    Archivo FASTA con las secuencias DNA (OBLIGATORIO)\n";
    std::cout << "  -p <match> <mismatch> <gap>   Parametros de puntuacion (OBLIGATORIO)\n";
    std::cout << "  -o <archivo.csv>      Archivo de salida CSV [default: resultado.csv]\n";
    std::cout << "  -h, --help           Mostrar esta ayuda\n\n";
    std::cout << "Ejemplos:\n";
    std::cout << "  " << nombre_programa << " -f data/test.fasta -p 2 -1 -2\n";
    std::cout << "  " << nombre_programa << " -f data/test.fasta -p 2 -1 -2 -o resultado.csv\n";
}

/**
 * @brief Programa principal
 */
int main(int argc, char* argv[]) {
    
    // Variables para parámetros
    std::string archivo_fasta = "";
    std::string archivo_salida = "resultado.csv";
    int match = 0, mismatch = 0, gap = 0;
    bool parametros_validos = false;
    
    // Parsear argumentos
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-f" && i + 1 < argc) {
            archivo_fasta = argv[++i];
        }
        else if (arg == "-o" && i + 1 < argc) {
            archivo_salida = argv[++i];
        }
        else if (arg == "-p" && i + 3 < argc) {
            match = std::atoi(argv[++i]);
            mismatch = std::atoi(argv[++i]);
            gap = std::atoi(argv[++i]);
            parametros_validos = true;
        }
        else if (arg == "-h" || arg == "--help") {
            mostrarUso(argv[0]);
            return 0;
        }
    }
    
    // Validar argumentos
    if (archivo_fasta.empty()) {
        std::cerr << "Error: Debe especificar un archivo FASTA con -f\n\n";
        mostrarUso(argv[0]);
        return 1;
    }
    
    if (!parametros_validos) {
        std::cerr << "Error: Debe especificar los parametros de puntuacion con -p\n\n";
        mostrarUso(argv[0]);
        return 1;
    }
    
    // Leer secuencias del archivo FASTA
    std::cout << "Leyendo secuencias de " << archivo_fasta << "...\n";
    std::vector<std::string> secuencias = leerArchivoFasta(archivo_fasta);
    
    if (secuencias.size() < 2) {
        std::cerr << "Error: El archivo FASTA debe contener al menos 2 secuencias\n";
        return 1;
    }
    
    std::string secA = secuencias[0];
    std::string secB = secuencias[1];
    
    std::cout << "Secuencia A: " << secA.length() << " caracteres\n";
    std::cout << "Secuencia B: " << secB.length() << " caracteres\n";
    std::cout << "Parametros: match=" << match << ", mismatch=" << mismatch << ", gap=" << gap << "\n\n";
    
    ConfiguracionAlineamiento config(match, mismatch, gap, false);
    
    std::cout << "Ejecutando alineamiento secuencial...\n";
    auto inicio = std::chrono::high_resolution_clock::now();
    ResultadoAlineamiento resultado = AlgNW(secA, secB, config);
    auto fin = std::chrono::high_resolution_clock::now();
    
    double tiempo_total = std::chrono::duration<double, std::milli>(fin - inicio).count();
    
    // Mostrar resultados
    std::cout << "\n=== RESULTADOS ===\n";
    std::cout << "Puntuacion: " << resultado.puntuacion << "\n";
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "Tiempo de inicializacion: " << resultado.tiempo_fase1_ms << " ms\n";
    std::cout << "Tiempo de llenado de matriz: " << resultado.tiempo_fase2_ms << " ms\n";
    std::cout << "Tiempo de traceback: " << resultado.tiempo_fase3_ms << " ms\n";
    std::cout << "Tiempo total: " << tiempo_total << " ms\n";
    
    // Guardar resultados en CSV
    guardarCSV(archivo_salida, archivo_fasta, resultado, match, mismatch, gap);
    std::cout << "\nResultados guardados en: " << archivo_salida << "\n";
    
    return 0;
}

