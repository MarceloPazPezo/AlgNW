/**
 * @file main-benchmark.cpp
 * @brief Programa de benchmarking para comparar todas las implementaciones
 * 
 * Uso:
 *   ./main-benchmark -f archivo.fasta -p <match> <mismatch> <gap> [-r <repeticiones>] [-o salida.csv]
 * 
 * Ejemplo:
 *   ./main-benchmark -f data/test.fasta -p 2 0 -2 -r 5 -o resultados.csv
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
#include <sstream>
#include <ctime>
#include <random>
#include <thread>
#include <functional>

// Incluir headers del proyecto
#include "tipos.h"
#include "puntuacion.h"
#include "secuencial.h"
#include "paralelo.h"
#include "utilidades.h"

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

/**
 * @brief Limpia la caché accediendo a una gran cantidad de memoria
 * 
 * Esta función intenta limpiar las cachés L1, L2, L3 accediendo a
 * una cantidad significativa de memoria que no está relacionada con
 * las matrices del algoritmo.
 */
void limpiarCache() {
    const size_t TAMANIO_LIMPIEZA = 100 * 1024 * 1024; // 100 MB
    std::vector<int> buffer_limpieza(TAMANIO_LIMPIEZA / sizeof(int), 0);
    
    // Acceder a la memoria de forma aleatoria para invalidar caché
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, buffer_limpieza.size() - 1);
    
    for (size_t i = 0; i < buffer_limpieza.size() / 10; ++i) {
        size_t idx = dis(gen);
        buffer_limpieza[idx] = static_cast<int>(i);
    }
    
    // Forzar escritura para asegurar que la caché se invalide
    volatile int suma = 0;
    for (size_t i = 0; i < buffer_limpieza.size(); i += 1000) {
        suma += buffer_limpieza[i];
    }
    (void)suma; // Evitar warning de variable no usada
}

/**
 * @brief Guarda resultados en CSV
 */
void guardarResultadosCSV(const std::string& archivo_salida,
                         const std::string& archivo_fasta,
                         const std::string& metodo,
                         const ResultadoAlineamiento& resultado,
                         int match, int mismatch, int gap,
                         int repeticion = 0) {
    
    std::ofstream csv(archivo_salida, std::ios::app);
    
    if (!csv.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo " << archivo_salida << "\n";
        return;
    }
    
    // Escribir encabezado si el archivo está vacío
    csv.seekp(0, std::ios::end);
    bool archivo_vacio = csv.tellp() == 0;
    
    if (archivo_vacio) {
        csv << "archivo_fasta,metodo,repeticion,longitud_A,longitud_B,match,mismatch,gap";
        csv << ",tiempo_init_ms,tiempo_llenado_ms,tiempo_traceback_ms,tiempo_total_ms,puntuacion\n";
    }
    
    // Escribir datos
    double tiempo_total = resultado.tiempo_fase1_ms + resultado.tiempo_fase2_ms + resultado.tiempo_fase3_ms;
    
    csv << archivo_fasta << ",";
    csv << metodo << ",";
    csv << repeticion << ",";
    csv << resultado.secA.length() << "," << resultado.secB.length() << ",";
    csv << match << "," << mismatch << "," << gap << ",";
    csv << std::fixed << std::setprecision(4);
    csv << resultado.tiempo_fase1_ms << "," << resultado.tiempo_fase2_ms << "," << resultado.tiempo_fase3_ms << ",";
    csv << tiempo_total << ",";
    csv << resultado.puntuacion << "\n";
    
    csv.close();
}

/**
 * @brief Ejecuta un método de alineamiento con limpieza de caché
 */
ResultadoAlineamiento ejecutarConLimpiezaCache(
    std::function<ResultadoAlineamiento(const std::string&, const std::string&, const ConfiguracionAlineamiento&)> funcion,
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config) {
    
    // Limpiar caché antes de ejecutar
    limpiarCache();
    
    // Pequeña pausa para asegurar que la limpieza se complete
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Ejecutar el método
    return funcion(secA, secB, config);
}

/**
 * @brief Muestra el uso del programa
 */
void mostrarUso(const char* nombre_programa) {
    std::cout << "Uso: " << nombre_programa << " [opciones]\n\n";
    std::cout << "Opciones:\n";
    std::cout << "  -f <archivo.fasta>    Archivo FASTA con las secuencias (OBLIGATORIO)\n";
    std::cout << "  -p <match> <mismatch> <gap>   Parametros de puntuacion (OBLIGATORIO)\n";
    std::cout << "  -r <numero>           Numero de repeticiones por metodo [default: 1]\n";
    std::cout << "  -o <archivo.csv>      Archivo de salida CSV [default: benchmark.csv]\n";
    std::cout << "  -h, --help           Mostrar esta ayuda\n\n";
    std::cout << "Ejemplos:\n";
    std::cout << "  " << nombre_programa << " -f data/test.fasta -p 2 0 -2\n";
    std::cout << "  " << nombre_programa << " -f data/test.fasta -p 2 0 -2 -r 5 -o resultados.csv\n\n";
    std::cout << "Metodos comparados:\n";
    std::cout << "  - secuencial\n";
    std::cout << "  - antidiagonal (schedule desde OMP_SCHEDULE)\n";
    std::cout << "  - bloques (schedule desde OMP_SCHEDULE)\n";
    std::cout << "\n";
    std::cout << "NOTA: Configure OMP_SCHEDULE para cambiar el schedule de OpenMP:\n";
    std::cout << "  export OMP_SCHEDULE=\"static\"        # static sin chunk\n";
    std::cout << "  export OMP_SCHEDULE=\"static,1\"      # static con chunk size 1\n";
    std::cout << "  export OMP_SCHEDULE=\"dynamic,1\"     # dynamic con chunk size 1\n";
    std::cout << "  export OMP_SCHEDULE=\"guided,1\"      # guided con chunk size mínimo 1\n";
    std::cout << "  export OMP_SCHEDULE=\"auto\"           # auto (OpenMP decide)\n";
}

/**
 * @brief Programa principal
 */
int main(int argc, char* argv[]) {
    
    // Variables para parámetros
    std::string archivo_fasta = "";
    std::string archivo_salida = "benchmark.csv";
    std::string metodo_seleccionado = ""; // [NEW] Variable para seleccionar método
    int match = 0, mismatch = 0, gap = 0;
    int repeticiones = 1;
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
        else if (arg == "-r" && i + 1 < argc) {
            repeticiones = std::atoi(argv[++i]);
            if (repeticiones < 1) repeticiones = 1;
        }
        else if (arg == "-p" && i + 3 < argc) {
            match = std::atoi(argv[++i]);
            mismatch = std::atoi(argv[++i]);
            gap = std::atoi(argv[++i]);
            parametros_validos = true;
        }
        else if ((arg == "-m" || arg == "--metodo") && i + 1 < argc) { // [NEW] Argumento -m
            metodo_seleccionado = argv[++i];
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
    std::cout << "Parametros: match=" << match << ", mismatch=" << mismatch << ", gap=" << gap << "\n";
    std::cout << "Repeticiones por metodo: " << repeticiones << "\n";
    if (!metodo_seleccionado.empty()) {
        std::cout << "Metodo seleccionado: " << metodo_seleccionado << "\n";
    }
    std::cout << "\n";
    
    // Crear configuración de alineamiento
    ConfiguracionAlineamiento config(match, mismatch, gap, false);
    
    // Definir métodos a probar
    struct MetodoPrueba {
        std::string nombre;
        std::function<ResultadoAlineamiento(const std::string&, const std::string&, const ConfiguracionAlineamiento&)> funcion;
    };
    
    std::vector<MetodoPrueba> todos_metodos = {
        // Secuencial
        {"secuencial", alineamientoNWR},
        
        // Paralelo: antidiagonal (schedule desde OMP_SCHEDULE)
        {"antidiagonal", alineamientoNWParaleloAntidiagonal},
        
        // Paralelo: bloques (schedule desde OMP_SCHEDULE)
        {"bloques", alineamientoNWParaleloBloques}
    };

    // Filtrar métodos si se seleccionó uno
    std::vector<MetodoPrueba> metodos;
    if (!metodo_seleccionado.empty()) {
        bool encontrado = false;
        for (const auto& m : todos_metodos) {
            if (m.nombre == metodo_seleccionado) {
                metodos.push_back(m);
                encontrado = true;
                break;
            }
        }
        if (!encontrado) {
            std::cerr << "Error: Metodo '" << metodo_seleccionado << "' no encontrado.\n";
            std::cerr << "Metodos disponibles:\n";
            for (const auto& m : todos_metodos) {
                std::cerr << "  - " << m.nombre << "\n";
            }
            return 1;
        }
    } else {
        metodos = todos_metodos;
    }
    
    std::cout << "=== EJECUTANDO BENCHMARK ===\n";
    std::cout << "Metodos: " << metodos.size() << "\n";
    std::cout << "Total de ejecuciones: " << (metodos.size() * repeticiones) << "\n\n";
    
    // Ejecutar cada método
    for (const auto& metodo : metodos) {
        std::cout << "--- Metodo: " << metodo.nombre << " ---\n";
        
        for (int r = 1; r <= repeticiones; ++r) {
            std::cout << "  Repeticion " << r << "/" << repeticiones << "... ";
            std::cout.flush();
            
            // Ejecutar con limpieza de caché
            auto inicio = std::chrono::high_resolution_clock::now();
            ResultadoAlineamiento resultado = ejecutarConLimpiezaCache(
                metodo.funcion, secA, secB, config);
            auto fin = std::chrono::high_resolution_clock::now();
            
            double tiempo_total = std::chrono::duration<double, std::milli>(fin - inicio).count();
            
            // Guardar resultado
            guardarResultadosCSV(archivo_salida, archivo_fasta, metodo.nombre, 
                               resultado, match, mismatch, gap, r);
            
            std::cout << "Completado (total: " << std::fixed << std::setprecision(2) 
                      << tiempo_total << " ms)\n";
        }
        std::cout << "\n";
    }
    
    std::cout << "=== BENCHMARK COMPLETADO ===\n";
    std::cout << "Resultados guardados en: " << archivo_salida << "\n";
    
    return 0;
}

