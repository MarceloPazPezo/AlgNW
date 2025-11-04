#include "secuencial.h"
#include "puntuacion.h"
#include "utilidades.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include <chrono>

/**
 * @brief Ejecuta Needleman–Wunsch usando una matriz de traceback (punteros).
 *
 * Implementación clásica que almacena direcciones en una matriz auxiliar y
 * luego realiza el traceback leyendo esa matriz.
 */
ResultadoAlineamiento alineamientoNWP(const std::string& secA, const std::string& secB, 
                                                const ConfiguracionAlineamiento& config) {
    int m = secA.length();
    int n = secB.length();
    
    // Verificar límites de memoria
    if (!verificarLimitesSecuencia(m, n, true)) {
        std::cerr << "Alineamiento cancelado por límites de memoria.\n";
        return ResultadoAlineamiento("", "", 0, 0.0, 0.0, 0.0);
    }
    
    // Mostrar información si verbose está activado
    if (config.verbose) {
        std::cout << "=== CONFIGURACIÓN DE PUNTUACIÓN ===\n";
        std::cout << "Matriz: " << obtenerNombreMatriz(config.puntuacion.tipo) << "\n";
        std::cout << "Penalidad de gap: " << obtenerPenalidadGap(config.puntuacion) << "\n\n";
        
        InfoMemoria infoMem = calcularUsoMemoria(m, n, true);
        imprimirInfoMemoria(infoMem);
        
        std::cout << "=== FASE 1: INICIALIZACIÓN ===\n";
    }
    
    // Crear matrices
    auto t_inicio_f1 = std::chrono::high_resolution_clock::now();
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));
    std::vector<std::vector<Direccion>> traceback(m + 1, std::vector<Direccion>(n + 1, Direccion::DIAGONAL));

    int penalidadGap = obtenerPenalidadGap(config.puntuacion);

    // Inicialización
    F[0][0] = 0;
    for (int i = 1; i <= m; ++i) {
        F[i][0] = F[i-1][0] + penalidadGap;
        traceback[i][0] = Direccion::ARRIBA;
    }
    for (int j = 1; j <= n; ++j) {
        F[0][j] = F[0][j-1] + penalidadGap;
        traceback[0][j] = Direccion::IZQUIERDA;
    }
    auto t_fin_f1 = std::chrono::high_resolution_clock::now();
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_f1 - t_inicio_f1).count();
    
    if (config.verbose) {
        std::cout << "Matriz inicializada. Dimensiones: " << (m+1) << "x" << (n+1) << "\n";
        std::cout << "\n=== FASE 2: CÁLCULO DE PUNTUACIONES ===\n";
    }
    
    // Llenar la matriz
    auto t_inicio_f2 = std::chrono::high_resolution_clock::now();
    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
            int eliminacion = F[i-1][j] + penalidadGap;
            int insercion = F[i][j-1] + penalidadGap;

            if (coincidencia >= eliminacion && coincidencia >= insercion) {
                F[i][j] = coincidencia;
                traceback[i][j] = Direccion::DIAGONAL;
            } else if (eliminacion >= insercion) {
                F[i][j] = eliminacion;
                traceback[i][j] = Direccion::ARRIBA;
            } else {
                F[i][j] = insercion;
                traceback[i][j] = Direccion::IZQUIERDA;
            }
        }
    }
    auto t_fin_f2 = std::chrono::high_resolution_clock::now();
    double tiempo_fase2_ms = std::chrono::duration<double, std::milli>(t_fin_f2 - t_inicio_f2).count();

    if (config.verbose) {
        std::cout << "Matriz de puntuaciones calculada. Puntuación final: " << F[m][n] << "\n";
        std::cout << "\n=== FASE 3: TRACEBACK CON PUNTEROS ===\n";
    }
    
    auto t_inicio_f3 = std::chrono::high_resolution_clock::now();
    std::string alineadaA = "";
    std::string alineadaB = "";
    int i = m, j = n;
    
    while (i > 0 || j > 0) {
        if (traceback[i][j] == Direccion::DIAGONAL) {
            alineadaA = secA[i-1] + alineadaA;
            alineadaB = secB[j-1] + alineadaB;
            i--; j--;
        } else if (traceback[i][j] == Direccion::ARRIBA) {
            alineadaA = secA[i-1] + alineadaA;
            alineadaB = "-" + alineadaB;
            i--;
        } else {
            alineadaA = "-" + alineadaA;
            alineadaB = secB[j-1] + alineadaB;
            j--;
        }
    }

    auto t_fin_f3 = std::chrono::high_resolution_clock::now();
    double tiempo_fase3_ms = std::chrono::duration<double, std::milli>(t_fin_f3 - t_inicio_f3).count();

    if (config.verbose) {
        std::cout << "Traceback completado.\n";
        std::cout << "Tiempos: llenado=" << tiempo_fase2_ms << " ms, traceback=" << tiempo_fase3_ms << " ms\n";
    }

    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}

/**
 * @brief Ejecuta Needleman–Wunsch con recálculo del traceback (memoria reducida).
 *
 * Calcula únicamente la matriz de puntuaciones y durante el traceback vuelve a
 * calcular las decisiones, ahorrando memoria a costa de cómputo adicional.
 */
ResultadoAlineamiento alineamientoNWR(const std::string& secA, const std::string& secB, 
                                                const ConfiguracionAlineamiento& config) {
    int m = secA.length();
    int n = secB.length();
    
    // Verificar límites de memoria
    if (!verificarLimitesSecuencia(m, n, true)) {
        std::cerr << "⚠️  Alineamiento cancelado por límites de memoria.\n";
        return ResultadoAlineamiento("", "", 0, 0.0, 0.0, 0.0);
    }
    
    // Mostrar información si verbose está activado
    if (config.verbose) {
        std::cout << "=== CONFIGURACIÓN DE PUNTUACIÓN ===\n";
        std::cout << "Matriz: " << obtenerNombreMatriz(config.puntuacion.tipo) << "\n";
        std::cout << "Penalidad de gap: " << obtenerPenalidadGap(config.puntuacion) << "\n\n";
        
        InfoMemoria infoMem = calcularUsoMemoria(m, n, false);
        imprimirInfoMemoria(infoMem);
        
        std::cout << "=== FASE 1: INICIALIZACIÓN ===\n";
    }
    
    // Crear solo matriz de puntuaciones
    auto t_inicio_fase1 = std::chrono::high_resolution_clock::now();
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));

    int penalidadGap = obtenerPenalidadGap(config.puntuacion);

    // Inicialización
    F[0][0] = 0;
    for (int i = 1; i <= m; ++i) {
        F[i][0] = F[i-1][0] + penalidadGap;
    }
    for (int j = 1; j <= n; ++j) {
        F[0][j] = F[0][j-1] + penalidadGap;
    }
    auto t_fin_fase1 = std::chrono::high_resolution_clock::now();
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_fase1 - t_inicio_fase1).count();

    if (config.verbose) {
        std::cout << "Matriz inicializada. Dimensiones: " << (m+1) << "x" << (n+1) << "\n";
        std::cout << "\n=== FASE 2: CÁLCULO DE PUNTUACIONES ===\n";
    }
    
    // Llenar la matriz
    auto t_inicio_fase2 = std::chrono::high_resolution_clock::now();
    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
            int eliminacion = F[i-1][j] + penalidadGap;
            int insercion = F[i][j-1] + penalidadGap;
            
            F[i][j] = std::max({coincidencia, eliminacion, insercion});
        }
    }
    auto t_fin_fase2 = std::chrono::high_resolution_clock::now();
    double tiempo_fase2_ms = std::chrono::duration<double, std::milli>(t_fin_fase2 - t_inicio_fase2).count();

    if (config.verbose) {
        std::cout << "Matriz de puntuaciones calculada. Puntuación final: " << F[m][n] << "\n";
        std::cout << "\n=== FASE 3: TRACEBACK CON RECÁLCULO ===\n";
    }
    
    auto t_inicio_fase3 = std::chrono::high_resolution_clock::now();
    std::string alineadaA = "";
    std::string alineadaB = "";
    int i = m, j = n;
    
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0) {
            int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
            int eliminacion = (i > 0) ? F[i-1][j] + penalidadGap : -999999;
            int insercion = (j > 0) ? F[i][j-1] + penalidadGap : -999999;
            
            if (F[i][j] == coincidencia) {
                alineadaA = secA[i-1] + alineadaA;
                alineadaB = secB[j-1] + alineadaB;
                i--; j--;
            } else if (F[i][j] == eliminacion) {
                alineadaA = secA[i-1] + alineadaA;
                alineadaB = "-" + alineadaB;
                i--;
            } else if (F[i][j] == insercion) {
                alineadaA = "-" + alineadaA;
                alineadaB = secB[j-1] + alineadaB;
                j--;
            }
        } else if (i > 0) {
            alineadaA = secA[i-1] + alineadaA;
            alineadaB = "-" + alineadaB;
            i--;
        } else {
            alineadaA = "-" + alineadaA;
            alineadaB = secB[j-1] + alineadaB;
            j--;
        }
    }

    auto t_fin_fase3 = std::chrono::high_resolution_clock::now();
    double tiempo_fase3_ms = std::chrono::duration<double, std::milli>(t_fin_fase3 - t_inicio_fase3).count();

    if (config.verbose) {
        std::cout << "Traceback completado.\n";
        std::cout << "Tiempos: llenado=" << tiempo_fase2_ms << " ms, traceback=" << tiempo_fase3_ms << " ms\n";
    }

    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}
