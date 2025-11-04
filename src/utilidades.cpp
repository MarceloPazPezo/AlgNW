#include "utilidades.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <sstream>
#include <ctime>

void imprimirAlineamiento(const std::string& secA, const std::string& secB, int puntuacion) {
    std::cout << "\n=== ALINEAMIENTO GLOBAL ===\n";
    std::cout << "Puntuación: " << puntuacion << "\n\n";
    
    // Imprimir secuencia A
    std::cout << "Secuencia A: ";
    for (char c : secA) {
        std::cout << c << " ";
    }
    std::cout << "\n";
    
    // Imprimir línea de comparación
    std::cout << "             ";
    for (size_t i = 0; i < secA.length(); ++i) {
        if (secA[i] == secB[i]) {
            std::cout << "| ";
        } else if (secA[i] == '-' || secB[i] == '-') {
            std::cout << "  ";
        } else {
            std::cout << "· ";
        }
    }
    std::cout << "\n";
    
    // Imprimir secuencia B
    std::cout << "Secuencia B: ";
    for (char c : secB) {
        std::cout << c << " ";
    }
    std::cout << "\n\n";
    
    // Leyenda
    std::cout << "Leyenda: | = coincidencia, · = sustitución, espacio = gap\n";
    std::cout << "==========================================\n\n";
}

void imprimirResultadoAlineamiento(const ResultadoAlineamiento& resultado) {
    imprimirAlineamiento(resultado.secA, resultado.secB, resultado.puntuacion);
}

void compararResultados(const ResultadoAlineamiento& resultado1, const ResultadoAlineamiento& resultado2, 
                       const std::string& metodo1, const std::string& metodo2) {
    std::cout << "\n=== COMPARACIÓN DE MÉTODOS ===\n";
    std::cout << "Método 1 (" << metodo1 << "): Puntuación = " << resultado1.puntuacion << "\n";
    std::cout << "Método 2 (" << metodo2 << "): Puntuación = " << resultado2.puntuacion << "\n";
    
    if (resultado1.puntuacion == resultado2.puntuacion && 
        resultado1.secA == resultado2.secA && 
        resultado1.secB == resultado2.secB) {
        std::cout << "✓ AMBOS MÉTODOS PRODUCEN RESULTADOS IDÉNTICOS\n";
    } else {
        std::cout << "✗ LOS MÉTODOS PRODUCEN RESULTADOS DIFERENTES\n";
        if (resultado1.puntuacion != resultado2.puntuacion) {
            std::cout << "  - Puntuaciones diferentes: " << resultado1.puntuacion << " vs " << resultado2.puntuacion << "\n";
        }
        if (resultado1.secA != resultado2.secA) {
            std::cout << "  - Secuencias A diferentes\n";
        }
        if (resultado1.secB != resultado2.secB) {
            std::cout << "  - Secuencias B diferentes\n";
        }
    }
    std::cout << "================================\n\n";
}

double medirTiempo(std::function<void()> func) {
    auto inicio = std::chrono::high_resolution_clock::now();
    func();
    auto fin = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duracion = fin - inicio;
    return duracion.count();
}

std::vector<std::string> leerArchivoFasta(const std::string& nombreArchivo) {
    std::vector<std::string> secuencias;
    std::ifstream archivo(nombreArchivo);
    
    if (!archivo.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo " << nombreArchivo << "\n";
        return secuencias;
    }
    
    std::string linea;
    std::string secuencia_actual;
    
    while (std::getline(archivo, linea)) {
        // Eliminar espacios en blanco al final
        while (!linea.empty() && (linea.back() == '\r' || linea.back() == '\n')) {
            linea.pop_back();
        }
        
        if (linea.empty()) continue;
        
        if (linea[0] == '>') {
            // Cabecera de nueva secuencia
            if (!secuencia_actual.empty()) {
                secuencias.push_back(secuencia_actual);
                secuencia_actual.clear();
            }
        } else {
            // Línea de secuencia
            secuencia_actual += linea;
        }
    }
    
    // Agregar la última secuencia
    if (!secuencia_actual.empty()) {
        secuencias.push_back(secuencia_actual);
    }
    
    archivo.close();
    return secuencias;
}

bool exportarBenchmarkACSV(const std::vector<ResultadoBenchmark>& resultados, 
                          const std::string& nombreArchivo) {
    std::ofstream archivo(nombreArchivo);
    
    if (!archivo.is_open()) {
        std::cerr << "Error: No se pudo crear el archivo " << nombreArchivo << "\n";
        return false;
    }
    
    // Escribir cabecera
    archivo << "nombre_test,metodo,longitud_sec_a,longitud_sec_b,es_proteina,penalidad_gap,"
            << "tiempo_init_ms,tiempo_llenado_ms,tiempo_trace_ms,tiempo_total_ms,puntuacion\n";
    
    // Escribir datos
    for (const auto& r : resultados) {
        archivo << r.nombre_test << "," 
                << r.metodo << "," 
                << r.longitud_sec_a << "," 
                << r.longitud_sec_b << ","
                << (r.es_proteina ? "proteina" : "dna") << "," 
                << r.penalidad_gap << ","
                << std::fixed << std::setprecision(4) << r.tiempo_init_ms << ","
                << std::fixed << std::setprecision(4) << r.tiempo_llenado_ms << ","
                << std::fixed << std::setprecision(4) << r.tiempo_trace_ms << ","
                << std::fixed << std::setprecision(4) << r.tiempo_total_ms << "," 
                << r.puntuacion << "\n";
    }
    
    archivo.close();
    return true;
}

bool anexarBenchmarkACSV(const ResultadoBenchmark& resultado, 
                        const std::string& nombreArchivo) {
    // Verificar si el archivo existe para decidir si escribir cabecera
    bool archivoExiste = false;
    {
        std::ifstream test(nombreArchivo);
        archivoExiste = test.good();
    }
    
    std::ofstream archivo(nombreArchivo, std::ios::app);
    
    if (!archivo.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo " << nombreArchivo << "\n";
        return false;
    }
    
    // Escribir cabecera si el archivo no existía
    if (!archivoExiste) {
        archivo << "nombre_test,metodo,longitud_sec_a,longitud_sec_b,es_proteina,penalidad_gap,"
                << "tiempo_init_ms,tiempo_llenado_ms,tiempo_trace_ms,tiempo_total_ms,puntuacion\n";
    }
    
    // Escribir datos
    archivo << resultado.nombre_test << "," 
            << resultado.metodo << "," 
            << resultado.longitud_sec_a << "," 
            << resultado.longitud_sec_b << ","
            << (resultado.es_proteina ? "proteina" : "dna") << "," 
            << resultado.penalidad_gap << ","
            << std::fixed << std::setprecision(4) << resultado.tiempo_init_ms << ","
            << std::fixed << std::setprecision(4) << resultado.tiempo_llenado_ms << ","
            << std::fixed << std::setprecision(4) << resultado.tiempo_trace_ms << ","
            << std::fixed << std::setprecision(4) << resultado.tiempo_total_ms << "," 
            << resultado.puntuacion << "\n";
    
    archivo.close();
    return true;
}

// ========== FUNCIONES DE GESTIÓN DE MEMORIA ==========

InfoMemoria calcularUsoMemoria(size_t lenA, size_t lenB, bool conPunteros) {
    InfoMemoria info;
    info.longitud_sec_a = lenA;
    info.longitud_sec_b = lenB;
    info.usa_matriz_traceback = conPunteros;
    
    // Matriz de puntuaciones: (m+1) x (n+1) x sizeof(int)
    size_t elementos_matriz = (lenA + 1) * (lenB + 1);
    size_t bytes_matriz = elementos_matriz * sizeof(int);
    info.tamaño_matriz_mb = bytes_matriz / (1024 * 1024);
    
    // Matriz de traceback (solo si conPunteros)
    if (conPunteros) {
        // Asumiendo 1 byte por dirección (enum)
        size_t bytes_traceback = elementos_matriz * sizeof(char);
        info.tamaño_traceback_mb = bytes_traceback / (1024 * 1024);
    } else {
        info.tamaño_traceback_mb = 0;
    }
    
    info.memoria_total_mb = info.tamaño_matriz_mb + info.tamaño_traceback_mb;
    
    return info;
}

bool verificarLimitesSecuencia(size_t lenA, size_t lenB, bool mostrarAdvertencias) {
    if (lenA > LONGITUD_MAXIMA_SECUENCIA || lenB > LONGITUD_MAXIMA_SECUENCIA) {
        if (mostrarAdvertencias) {
            std::cerr << "⚠️  ERROR: Secuencia demasiado grande!\n";
            std::cerr << "   Longitud máxima permitida: " << LONGITUD_MAXIMA_SECUENCIA << " caracteres\n";
            std::cerr << "   Longitud recibida: A=" << lenA << ", B=" << lenB << "\n";
        }
        return false;
    }
    
    if ((lenA >= LONGITUD_ADVERTENCIA_SECUENCIA || lenB >= LONGITUD_ADVERTENCIA_SECUENCIA) && mostrarAdvertencias) {
        std::cerr << "⚠️  ADVERTENCIA: Secuencias grandes detectadas\n";
        std::cerr << "   Esto puede requerir mucha memoria y tiempo de procesamiento\n";
        std::cerr << "   Longitudes: A=" << lenA << ", B=" << lenB << "\n";
    }
    
    return true;
}

void imprimirInfoMemoria(const InfoMemoria& info) {
    std::cout << "\n=== INFORMACIÓN DE MEMORIA ===\n";
    std::cout << "Longitudes: A=" << info.longitud_sec_a << ", B=" << info.longitud_sec_b << "\n";
    std::cout << "Matriz de puntuaciones: " << info.tamaño_matriz_mb << " MB\n";
    if (info.usa_matriz_traceback) {
        std::cout << "Matriz de traceback: " << info.tamaño_traceback_mb << " MB\n";
    } else {
        std::cout << "Matriz de traceback: NO USADA (traceback recalculado)\n";
    }
    std::cout << "Memoria total estimada: " << info.memoria_total_mb << " MB\n";
    std::cout << "==============================\n\n";
}

std::string formatearBytes(size_t bytes) {
    const char* unidades[] = {"B", "KB", "MB", "GB", "TB"};
    int unidad = 0;
    double tamaño = static_cast<double>(bytes);
    
    while (tamaño >= 1024.0 && unidad < 4) {
        tamaño /= 1024.0;
        unidad++;
    }
    
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << tamaño << " " << unidades[unidad];
    return ss.str();
}
