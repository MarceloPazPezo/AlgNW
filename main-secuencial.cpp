/**
 * @file main-secuencial.cpp
 * @brief Programa independiente para ejecutar alineamiento Needleman-Wunsch
 * 
 * Uso:
 *   ./main-secuencial -f archivo.fasta -p <match> <mismatch> <gap> -o salida.csv
 * 
 * Ejemplo:
 *   ./main-secuencial -f data/test.fasta -p 2 0 -2 -o resultado-test.csv
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <cstdlib>
#include <cstring>

// Incluir headers del proyecto
#include "tipos.h"
#include "puntuacion.h"
#include "secuencial.h"
#include "utilidades.h"

// ============================================================================
// FUNCIONES AUXILIARES SIMPLES
// ============================================================================

// Nota: leerFASTA y otras funciones están en utilidades.h/utilidades.cpp

/**
 * @brief Guarda resultados en CSV usando ResultadoAlineamiento
 */
void guardarCSVMejorado(const std::string& archivo_salida, 
                        const std::string& archivo_fasta,
                        const ResultadoAlineamiento& resultado,
                        int match, int mismatch, int gap) {
    
    std::ofstream csv(archivo_salida, std::ios::app);
    
    if (!csv.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo " << archivo_salida << "\n";
        return;
    }
    
    // Escribir encabezado si el archivo está vacío
    csv.seekp(0, std::ios::end);
    bool archivo_vacio = csv.tellp() == 0;
    
    if (archivo_vacio) {
        csv << "archivo_fasta,longitud_A,longitud_B,match,mismatch,gap";
        csv << ",tiempo_init_ms,tiempo_llenado_ms,tiempo_traceback_ms,tiempo_total_ms,puntuacion\n";
    }
    
    // Escribir datos
    double tiempo_total = resultado.tiempo_fase1_ms + resultado.tiempo_fase2_ms + resultado.tiempo_fase3_ms;
    
    csv << archivo_fasta << ",";
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
    std::cout << "  -f <archivo.fasta>    Archivo FASTA con las secuencias (OBLIGATORIO)\n";
    std::cout << "  -p <match> <mismatch> <gap>   Parametros de puntuacion (OBLIGATORIO)\n";
    std::cout << "                          match:    puntuacion por coincidencia (ej: 2)\n";
    std::cout << "                          mismatch: puntuacion por sustitucion (ej: 0)\n";
    std::cout << "                          gap:      penalidad por gap (ej: -2)\n";
    std::cout << "  -o <archivo.csv>      Archivo de salida CSV (por defecto: resultado.csv)\n";
    std::cout << "  -h, --help           Mostrar esta ayuda\n\n";
    std::cout << "Ejemplos:\n";
    std::cout << "  " << nombre_programa << " -f data/test.fasta -p 2 0 -2\n";
    std::cout << "  " << nombre_programa << " -f data/test.fasta -p 2 0 -2 -o resultado-test.csv\n";
    std::cout << "  " << nombre_programa << " -f data/protein.fasta -p 5 -4 -10\n\n";
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
    
    // Crear configuración de alineamiento
    ConfiguracionAlineamiento config(match, mismatch, gap, false);
    
    // Ejecutar alineamiento usando alineamientoNWR
    std::cout << "Ejecutando alineamiento...\n";
    ResultadoAlineamiento resultado = alineamientoNWR(secA, secB, config);
    
    // Mostrar resultados
    double tiempo_total = resultado.tiempo_fase1_ms + resultado.tiempo_fase2_ms + resultado.tiempo_fase3_ms;
    
    std::cout << "\nResultados:\n";
    std::cout << "  Puntuacion: " << resultado.puntuacion << "\n";
    std::cout << "  Tiempos:\n";
    std::cout << "    Inicializacion: " << std::fixed << std::setprecision(4) << resultado.tiempo_fase1_ms << " ms\n";
    std::cout << "    Llenado:        " << resultado.tiempo_fase2_ms << " ms\n";
    std::cout << "    Traceback:      " << resultado.tiempo_fase3_ms << " ms\n";
    std::cout << "    Total:          " << tiempo_total << " ms\n";
    
    // Guardar en CSV
    std::cout << "\nGuardando resultados en " << archivo_salida << "...\n";
    guardarCSVMejorado(archivo_salida, archivo_fasta, resultado, match, mismatch, gap);
    
    std::cout << "Listo!\n";
    
    return 0;
}

