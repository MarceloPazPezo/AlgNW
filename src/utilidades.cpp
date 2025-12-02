#include "utilidades.h"
#include "puntuacion.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <sstream>
#include <algorithm>

void imprimirAlineamiento(const std::string& secA, const std::string& secB, int puntuacion) {
    std::cout << "\n=== ALINEAMIENTO GLOBAL ===\n";
    std::cout << "Puntuación: " << puntuacion << "\n\n";
    
    std::cout << "Secuencia A: ";
    for (char c : secA) {
        std::cout << c << " ";
    }
    std::cout << "\n";
    
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
    
    std::cout << "Secuencia B: ";
    for (char c : secB) {
        std::cout << c << " ";
    }
    std::cout << "\n\n";
    
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

static std::vector<std::vector<int>> calcularMatrizPuntuacion(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config) {
    
    int m = secA.length();
    int n = secB.length();
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));
    int penalidadGap = obtenerPenalidadGapDNA(config.puntuacion);
    
    // Inicialización
    F[0][0] = 0;
    for (int i = 1; i <= m; ++i) {
        F[i][0] = F[i-1][0] + penalidadGap;
    }
    for (int j = 1; j <= n; ++j) {
        F[0][j] = F[0][j-1] + penalidadGap;
    }
    
    // Llenar la matriz
    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            int coincidencia = F[i-1][j-1] + obtenerPuntuacionDNA(secA[i-1], secB[j-1], config.puntuacion);
            int eliminacion = F[i-1][j] + penalidadGap;
            int insercion = F[i][j-1] + penalidadGap;
            F[i][j] = std::max({coincidencia, eliminacion, insercion});
        }
    }
    
    return F;
}

void compararResultadosDetallado(
    const ResultadoAlineamiento& resultado1, 
    const ResultadoAlineamiento& resultado2, 
    const std::string& metodo1, 
    const std::string& metodo2,
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config,
    bool comparar_matrices,
    int umbral_tamano_matriz) {
    
    std::cout << "\n=== COMPARACIÓN DETALLADA DE MÉTODOS ===\n";
    std::cout << "Método 1: " << metodo1 << "\n";
    std::cout << "Método 2: " << metodo2 << "\n\n";
    
    // Comparar puntuaciones
    std::cout << "--- PUNTUACIÓN FINAL (F[m][n]) ---\n";
    std::cout << "  " << metodo1 << ": " << resultado1.puntuacion << "\n";
    std::cout << "  " << metodo2 << ": " << resultado2.puntuacion << "\n";
    
    bool puntuaciones_iguales = (resultado1.puntuacion == resultado2.puntuacion);
    if (puntuaciones_iguales) {
        std::cout << "  ✓ Puntuaciones IDÉNTICAS\n\n";
    } else {
        std::cout << "  ✗ Puntuaciones DIFERENTES (diferencia: " 
                  << (resultado2.puntuacion - resultado1.puntuacion) << ")\n\n";
    }
    
    // Comparar secuencias alineadas
    std::cout << "--- SECUENCIAS ALINEADAS ---\n";
    bool secA_iguales = (resultado1.secA == resultado2.secA);
    bool secB_iguales = (resultado1.secB == resultado2.secB);
    
    if (secA_iguales && secB_iguales) {
        std::cout << "  ✓ Secuencias alineadas IDÉNTICAS\n";
        std::cout << "    Longitud: " << resultado1.secA.length() << " caracteres\n\n";
    } else {
        std::cout << "  ✗ Secuencias alineadas DIFERENTES\n";
        if (!secA_iguales) {
            std::cout << "    - Secuencia A difiere\n";
            if (resultado1.secA.length() != resultado2.secA.length()) {
                std::cout << "      Longitudes: " << resultado1.secA.length() 
                          << " vs " << resultado2.secA.length() << "\n";
            }
        }
        if (!secB_iguales) {
            std::cout << "    - Secuencia B difiere\n";
            if (resultado1.secB.length() != resultado2.secB.length()) {
                std::cout << "      Longitudes: " << resultado1.secB.length() 
                          << " vs " << resultado2.secB.length() << "\n";
            }
        }
        std::cout << "\n";
    }
    
    // Comparar matrices si se solicita y es factible
    int m = secA.length();
    int n = secB.length();
    bool matriz_pequena = (m <= umbral_tamano_matriz && n <= umbral_tamano_matriz);
    
    if (comparar_matrices && matriz_pequena) {
        std::cout << "--- MATRIZ DE PUNTUACIÓN ---\n";
        std::cout << "  Calculando matriz de referencia...\n";
        
        auto matriz_ref = calcularMatrizPuntuacion(secA, secB, config);
        
        // Verificar que la puntuación final coincide
        if (matriz_ref[m][n] == resultado1.puntuacion && matriz_ref[m][n] == resultado2.puntuacion) {
            std::cout << "  ✓ Puntuación final F[" << m << "][" << n << "] = " 
                      << matriz_ref[m][n] << " coincide con ambos métodos\n";
        } else {
            std::cout << "  ✗ Inconsistencia en puntuación final:\n";
            std::cout << "    Matriz referencia: " << matriz_ref[m][n] << "\n";
            std::cout << "    " << metodo1 << ": " << resultado1.puntuacion << "\n";
            std::cout << "    " << metodo2 << ": " << resultado2.puntuacion << "\n";
        }
        std::cout << "\n";
    } else if (comparar_matrices && !matriz_pequena) {
        std::cout << "--- MATRIZ DE PUNTUACIÓN ---\n";
        std::cout << "  ⚠ Matriz demasiado grande (" << m << "x" << n 
                  << ") para comparación completa\n";
        std::cout << "  Solo se compara la puntuación final F[" << m << "][" << n << "]\n";
        std::cout << "  (Umbral: " << umbral_tamano_matriz << "x" << umbral_tamano_matriz << ")\n\n";
    }
    
    // Resumen final
    std::cout << "--- RESUMEN ---\n";
    bool todo_igual = puntuaciones_iguales && secA_iguales && secB_iguales;
    if (todo_igual) {
        std::cout << "✓ AMBOS MÉTODOS PRODUCEN RESULTADOS IDÉNTICOS\n";
    } else {
        std::cout << "✗ LOS MÉTODOS PRODUCEN RESULTADOS DIFERENTES\n";
        if (!puntuaciones_iguales) {
            std::cout << "  - ERROR CRÍTICO: Puntuaciones diferentes\n";
        }
        if (!secA_iguales || !secB_iguales) {
            std::cout << "  - ADVERTENCIA: Secuencias alineadas diferentes\n";
            std::cout << "    (Esto puede ser normal si hay múltiples alineamientos óptimos)\n";
        }
    }
    std::cout << "==========================================\n\n";
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

