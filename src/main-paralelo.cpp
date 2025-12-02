/**
 * @file main-paralelo.cpp
 * @brief Programa de benchmarking para comparar implementaciones secuencial y paralelas (DNA)
 * 
 * Uso:
 *   ./main-paralelo -f archivo.fasta -p <match> <mismatch> <gap> [-r <repeticiones>] [-o salida.csv]
 * 
 * Ejemplo:
 *   ./main-paralelo -f data/test.fasta -p 2 -1 -2 -r 5 -o resultados.csv
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <cstdlib>
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
#include <omp.h>

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

/**
 * @brief Limpia la caché accediendo a una gran cantidad de memoria
 */
void limpiarCache() {
    const size_t TAMANIO_LIMPIEZA = 100 * 1024 * 1024; // 100 MB
    std::vector<int> buffer_limpieza(TAMANIO_LIMPIEZA / sizeof(int), 0);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, buffer_limpieza.size() - 1);
    
    for (size_t i = 0; i < buffer_limpieza.size() / 10; ++i) {
        size_t idx = dis(gen);
        buffer_limpieza[idx] = static_cast<int>(i);
    }
    
    volatile int suma = 0;
    for (size_t i = 0; i < buffer_limpieza.size(); i += 1000) {
        suma += buffer_limpieza[i];
    }
    (void)suma;
}

/**
 * @brief Guarda resultados en CSV
 */
void guardarResultadosCSV(const std::string& archivo_salida,
                         const std::string& archivo_fasta,
                         const std::string& metodo,
                         const ResultadoAlineamiento& resultado,
                         int match, int mismatch, int gap,
                         int repeticion = 0,
                         int num_threads = 1,
                         const std::string& schedule = "N/A") {
    
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
    csv << metodo << ",";
    csv << repeticion << ",";
    csv << num_threads << ",";
    csv << schedule << ",";
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
    
    limpiarCache();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    return funcion(secA, secB, config);
}

/**
 * @brief Muestra el uso del programa
 */
void mostrarUso(const char* nombre_programa) {
    std::cout << "Uso: " << nombre_programa << " [opciones]\n\n";
    std::cout << "Opciones:\n";
    std::cout << "  -f <archivo.fasta>    Archivo FASTA con las secuencias DNA (OBLIGATORIO)\n";
    std::cout << "  -p <match> <mismatch> <gap>   Parametros de puntuacion (OBLIGATORIO)\n";
    std::cout << "  -r <numero>           Numero de repeticiones por metodo [default: 1]\n";
    std::cout << "  -m <metodo>           Método específico a ejecutar [default: todos]\n";
    std::cout << "                        Opciones: secuencial, antidiagonal, bloques\n";
    std::cout << "  -o <archivo.csv>      Archivo de salida CSV [default: benchmark.csv]\n";
    std::cout << "  -h, --help           Mostrar esta ayuda\n\n";
    std::cout << "Ejemplos:\n";
    std::cout << "  " << nombre_programa << " -f data/test.fasta -p 2 -1 -2\n";
    std::cout << "  " << nombre_programa << " -f data/test.fasta -p 2 -1 -2 -r 5 -o resultados.csv\n\n";
    std::cout << "Metodos comparados:\n";
    std::cout << "  - secuencial\n";
    std::cout << "  - antidiagonal (schedule desde OMP_SCHEDULE)\n";
    std::cout << "  - bloques (schedule desde OMP_SCHEDULE)\n";
    std::cout << "\n";
    std::cout << "NOTA: Configure OMP_NUM_THREADS y OMP_SCHEDULE para controlar paralelización:\n";
    std::cout << "  export OMP_NUM_THREADS=8\n";
    std::cout << "  export OMP_SCHEDULE=\"dynamic,1\"\n";
}

/**
 * @brief Programa principal
 */
int main(int argc, char* argv[]) {
    
    std::string archivo_fasta = "";
    std::string archivo_salida = "benchmark.csv";
    std::string metodo_seleccionado = "";  // Para seleccionar un método específico
    int match = 0, mismatch = 0, gap = 0;
    int repeticiones = 1;
    bool parametros_validos = false;
    
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
            if (repeticiones < 1) {
                std::cerr << "Error: El número de repeticiones debe ser >= 1\n";
                return 1;
            }
        }
        else if (arg == "-m" && i + 1 < argc) {
            metodo_seleccionado = argv[++i];
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
    
    std::cout << "\n=== CONFIGURACIÓN OPENMP ===\n";
    const char* omp_num_threads_env = std::getenv("OMP_NUM_THREADS");
    if (omp_num_threads_env != nullptr) {
        std::cout << "OMP_NUM_THREADS: " << omp_num_threads_env << "\n";
    } else {
        std::cout << "OMP_NUM_THREADS: default (" << omp_get_max_threads() << " threads)\n";
    }
    const char* schedule = std::getenv("OMP_SCHEDULE");
    if (schedule != nullptr) {
        std::cout << "OMP_SCHEDULE: " << schedule << "\n";
    } else {
        std::cout << "OMP_SCHEDULE: default (static)\n";
    }
    std::cout << "Threads máximos disponibles: " << omp_get_max_threads() << "\n";
    std::cout << "============================\n\n";
    
    ConfiguracionAlineamiento config(match, mismatch, gap, false);
    
    struct MetodoPrueba {
        std::string nombre;
        std::function<ResultadoAlineamiento(const std::string&, const std::string&, const ConfiguracionAlineamiento&)> funcion;
    };
    
    std::vector<MetodoPrueba> todos_metodos = {
        {"secuencial", AlgNW},
        
        {"antidiagonal", alineamientoNWParaleloAntidiagonal},
        
        {"bloques", alineamientoNWParaleloBloques}
    };
    
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
            std::cerr << "Error: Método '" << metodo_seleccionado << "' no encontrado.\n";
            std::cerr << "Métodos disponibles:\n";
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
    
    int num_threads = omp_get_max_threads();
    std::string schedule_str = "N/A";
    const char* schedule_env = std::getenv("OMP_SCHEDULE");
    if (schedule_env != nullptr) {
        schedule_str = std::string(schedule_env);
    }
    
    for (const auto& metodo : metodos) {
        std::cout << "--- Metodo: " << metodo.nombre << " ---\n";
        
        for (int r = 1; r <= repeticiones; ++r) {
            std::cout << "  Repeticion " << r << "/" << repeticiones << "... ";
            std::cout.flush();
            
            ResultadoAlineamiento resultado = ejecutarConLimpiezaCache(
                metodo.funcion, secA, secB, config);
            
            guardarResultadosCSV(archivo_salida, archivo_fasta, metodo.nombre, 
                               resultado, match, mismatch, gap, r, num_threads, schedule_str);
            
            double tiempo_total = resultado.tiempo_fase1_ms + resultado.tiempo_fase2_ms + resultado.tiempo_fase3_ms;
            std::cout << "Tiempo: " << std::fixed << std::setprecision(2) 
                      << tiempo_total << " ms, Puntuacion: " << resultado.puntuacion << "\n";
        }
        std::cout << "\n";
    }
    
    std::cout << "=== BENCHMARK COMPLETADO ===\n";
    std::cout << "Resultados guardados en: " << archivo_salida << "\n";
    
    if (metodos.size() > 1 && repeticiones > 0) {
        std::cout << "\n=== COMPARACIÓN DE RESULTADOS ===\n";
        
        ResultadoAlineamiento resultado_secuencial = ejecutarConLimpiezaCache(
            AlgNW, secA, secB, config);
        ResultadoAlineamiento resultado_antidiagonal = ejecutarConLimpiezaCache(
            alineamientoNWParaleloAntidiagonal, secA, secB, config);
        ResultadoAlineamiento resultado_bloques = ejecutarConLimpiezaCache(
            alineamientoNWParaleloBloques, secA, secB, config);
        
        compararResultados(resultado_secuencial, resultado_antidiagonal, 
                          "secuencial", "antidiagonal");
        
        compararResultados(resultado_secuencial, resultado_bloques, 
                          "secuencial", "bloques");
    }
    
    return 0;
}

