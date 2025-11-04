#include "paralelo.h"
#include "puntuacion.h"
#include "utilidades.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <omp.h>

// ============================================================================
// ESTRATEGIA 1: ANTIDIAGONALES (WAVEFRONT)
// ============================================================================

ResultadoAlineamiento alineamientoNWP_OpenMP_Antidiag(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
) {
    int m = secA.length();
    int n = secB.length();
    
    // Verificar límites de memoria
    if (!verificarLimitesSecuencia(m, n, true)) {
        std::cerr << "Alineamiento cancelado por límites de memoria.\n";
        return ResultadoAlineamiento("", "", 0, 0.0, 0.0, 0.0);
    }
    
    if (config.verbose) {
        std::cout << "=== PARALELIZACIÓN: ANTIDIAGONALES ===\n";
        std::cout << "Threads disponibles: " << omp_get_max_threads() << "\n";
        std::cout << "Matriz: " << obtenerNombreMatriz(config.puntuacion.tipo) << "\n";
        std::cout << "Penalidad de gap: " << obtenerPenalidadGap(config.puntuacion) << "\n\n";
        
        InfoMemoria infoMem = calcularUsoMemoria(m, n, true);
        imprimirInfoMemoria(infoMem);
    }
    
    // FASE 1: Inicialización
    auto t_inicio_f1 = std::chrono::high_resolution_clock::now();
    
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));
    std::vector<std::vector<Direccion>> traceback(m + 1, std::vector<Direccion>(n + 1, Direccion::DIAGONAL));
    
    int gap = obtenerPenalidadGap(config.puntuacion);
    
    // Inicialización de bordes (secuencial - muy rápido)
    F[0][0] = 0;
    for (int i = 1; i <= m; ++i) {
        F[i][0] = F[i-1][0] + gap;
        traceback[i][0] = Direccion::ARRIBA;
    }
    for (int j = 1; j <= n; ++j) {
        F[0][j] = F[0][j-1] + gap;
        traceback[0][j] = Direccion::IZQUIERDA;
    }
    
    auto t_fin_f1 = std::chrono::high_resolution_clock::now();
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_f1 - t_inicio_f1).count();
    
    if (config.verbose) {
        std::cout << "Fase 1 completada en " << tiempo_fase1_ms << " ms\n";
        std::cout << "\n=== FASE 2: LLENADO POR ANTIDIAGONALES ===\n";
        std::cout << "Número de antidiagonales: " << (m + n - 1) << "\n";
    }
    
    // FASE 2: Llenado paralelo por antidiagonales
    auto t_inicio_f2 = std::chrono::high_resolution_clock::now();
    
    // Para cada antidiagonal k (donde k = i + j)
    for (int k = 2; k <= m + n; ++k) {
        // Calcular rango de filas para esta antidiagonal
        int i_start = std::max(1, k - n);
        int i_end = std::min(m, k - 1);
        
        // Procesar todas las celdas de esta antidiagonal EN PARALELO
        #pragma omp parallel for schedule(dynamic)
        for (int i = i_start; i <= i_end; ++i) {
            int j = k - i;  // j calculado para que i + j = k
            
            if (j >= 1 && j <= n) {
                // Calcular puntuaciones
                int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
                int eliminacion = F[i-1][j] + gap;
                int insercion = F[i][j-1] + gap;
                
                // Elegir el máximo y guardar dirección
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
        // Barrera implícita al final del #pragma omp parallel for
    }
    
    auto t_fin_f2 = std::chrono::high_resolution_clock::now();
    double tiempo_fase2_ms = std::chrono::duration<double, std::milli>(t_fin_f2 - t_inicio_f2).count();
    
    if (config.verbose) {
        std::cout << "Matriz calculada. Puntuación final: " << F[m][n] << "\n";
        std::cout << "Fase 2 completada en " << tiempo_fase2_ms << " ms\n";
        std::cout << "\n=== FASE 3: TRACEBACK ===\n";
    }
    
    // FASE 3: Traceback (secuencial - inherentemente serial)
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
        std::cout << "Traceback completado en " << tiempo_fase3_ms << " ms\n";
        std::cout << "\nTiempo total fase 2 (llenado): " << tiempo_fase2_ms << " ms\n";
    }
    
    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}

ResultadoAlineamiento alineamientoNWR_OpenMP_Antidiag(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
) {
    int m = secA.length();
    int n = secB.length();
    
    if (!verificarLimitesSecuencia(m, n, false)) {
        std::cerr << "Alineamiento cancelado por límites de memoria.\n";
        return ResultadoAlineamiento("", "", 0, 0.0, 0.0, 0.0);
    }
    
    if (config.verbose) {
        std::cout << "=== PARALELIZACIÓN: ANTIDIAGONALES (sin traceback matrix) ===\n";
        std::cout << "Threads disponibles: " << omp_get_max_threads() << "\n";
    }
    
    // FASE 1: Inicialización
    auto t_inicio_f1 = std::chrono::high_resolution_clock::now();
    
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));
    int gap = obtenerPenalidadGap(config.puntuacion);
    
    F[0][0] = 0;
    for (int i = 1; i <= m; ++i) F[i][0] = F[i-1][0] + gap;
    for (int j = 1; j <= n; ++j) F[0][j] = F[0][j-1] + gap;
    
    auto t_fin_f1 = std::chrono::high_resolution_clock::now();
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_f1 - t_inicio_f1).count();
    
    // FASE 2: Llenado paralelo por antidiagonales
    auto t_inicio_f2 = std::chrono::high_resolution_clock::now();
    
    for (int k = 2; k <= m + n; ++k) {
        int i_start = std::max(1, k - n);
        int i_end = std::min(m, k - 1);
        
        #pragma omp parallel for schedule(dynamic)
        for (int i = i_start; i <= i_end; ++i) {
            int j = k - i;
            if (j >= 1 && j <= n) {
                int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
                int eliminacion = F[i-1][j] + gap;
                int insercion = F[i][j-1] + gap;
                F[i][j] = std::max({coincidencia, eliminacion, insercion});
            }
        }
    }
    
    auto t_fin_f2 = std::chrono::high_resolution_clock::now();
    double tiempo_fase2_ms = std::chrono::duration<double, std::milli>(t_fin_f2 - t_inicio_f2).count();
    
    // FASE 3: Traceback con recálculo
    auto t_inicio_f3 = std::chrono::high_resolution_clock::now();
    
    std::string alineadaA = "";
    std::string alineadaB = "";
    int i = m, j = n;
    
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0) {
            int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
            int eliminacion = (i > 0) ? F[i-1][j] + gap : -999999;
            int insercion = (j > 0) ? F[i][j-1] + gap : -999999;
            
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
    
    auto t_fin_f3 = std::chrono::high_resolution_clock::now();
    double tiempo_fase3_ms = std::chrono::duration<double, std::milli>(t_fin_f3 - t_inicio_f3).count();
    
    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}

// ============================================================================
// ESTRATEGIA 2: BLOQUES (TILING)
// ============================================================================

ResultadoAlineamiento alineamientoNWP_OpenMP_Bloques(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config,
    int tile_size
) {
    int m = secA.length();
    int n = secB.length();
    
    if (!verificarLimitesSecuencia(m, n, true)) {
        std::cerr << "Alineamiento cancelado por límites de memoria.\n";
        return ResultadoAlineamiento("", "", 0, 0.0, 0.0, 0.0);
    }
    
    // Calcular número de bloques
    int num_tiles_i = (m + tile_size - 1) / tile_size;
    int num_tiles_j = (n + tile_size - 1) / tile_size;
    int num_ondas = num_tiles_i + num_tiles_j - 1;
    
    if (config.verbose) {
        std::cout << "=== PARALELIZACIÓN: BLOQUES (TILING) ===\n";
        std::cout << "Threads disponibles: " << omp_get_max_threads() << "\n";
        std::cout << "Tamaño de bloque: " << tile_size << "×" << tile_size << "\n";
        std::cout << "Número de bloques: " << num_tiles_i << "×" << num_tiles_j 
                  << " = " << (num_tiles_i * num_tiles_j) << " bloques\n";
        std::cout << "Número de ondas: " << num_ondas << "\n\n";
        
        InfoMemoria infoMem = calcularUsoMemoria(m, n, true);
        imprimirInfoMemoria(infoMem);
    }
    
    // FASE 1: Inicialización
    auto t_inicio_f1 = std::chrono::high_resolution_clock::now();
    
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));
    std::vector<std::vector<Direccion>> traceback(m + 1, std::vector<Direccion>(n + 1, Direccion::DIAGONAL));
    
    int gap = obtenerPenalidadGap(config.puntuacion);
    
    F[0][0] = 0;
    for (int i = 1; i <= m; ++i) {
        F[i][0] = F[i-1][0] + gap;
        traceback[i][0] = Direccion::ARRIBA;
    }
    for (int j = 1; j <= n; ++j) {
        F[0][j] = F[0][j-1] + gap;
        traceback[0][j] = Direccion::IZQUIERDA;
    }
    
    auto t_fin_f1 = std::chrono::high_resolution_clock::now();
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_f1 - t_inicio_f1).count();
    
    if (config.verbose) {
        std::cout << "Fase 1 completada en " << tiempo_fase1_ms << " ms\n";
        std::cout << "\n=== FASE 2: LLENADO POR BLOQUES ===\n";
    }
    
    // FASE 2: Llenado paralelo por bloques
    auto t_inicio_f2 = std::chrono::high_resolution_clock::now();
    
    // Procesar por ondas (antidiagonales de bloques)
    for (int tile_diag = 0; tile_diag < num_ondas; ++tile_diag) {
        // Calcular qué bloques están en esta onda
        int ti_start = std::max(0, tile_diag - num_tiles_j + 1);
        int ti_end = std::min(num_tiles_i - 1, tile_diag);
        
        if (config.verbose && tile_diag % 10 == 0) {
            std::cout << "Procesando onda " << tile_diag << "/" << num_ondas 
                      << " (" << (ti_end - ti_start + 1) << " bloques)\n";
        }
        
        // Procesar todos los bloques de esta onda EN PARALELO
        #pragma omp parallel for schedule(dynamic)
        for (int ti = ti_start; ti <= ti_end; ++ti) {
            int tj = tile_diag - ti;
            
            // Calcular límites del bloque en la matriz
            int i_start = ti * tile_size + 1;
            int i_end = std::min((ti + 1) * tile_size, m);
            int j_start = tj * tile_size + 1;
            int j_end = std::min((tj + 1) * tile_size, n);
            
            // Procesar todas las celdas del bloque SECUENCIALMENTE
            for (int i = i_start; i <= i_end; ++i) {
                for (int j = j_start; j <= j_end; ++j) {
                    int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
                    int eliminacion = F[i-1][j] + gap;
                    int insercion = F[i][j-1] + gap;
                    
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
        }
        // Barrera implícita al final del parallel for
    }
    
    auto t_fin_f2 = std::chrono::high_resolution_clock::now();
    double tiempo_fase2_ms = std::chrono::duration<double, std::milli>(t_fin_f2 - t_inicio_f2).count();
    
    if (config.verbose) {
        std::cout << "Matriz calculada. Puntuación final: " << F[m][n] << "\n";
        std::cout << "Fase 2 completada en " << tiempo_fase2_ms << " ms\n";
        std::cout << "\n=== FASE 3: TRACEBACK ===\n";
    }
    
    // FASE 3: Traceback (secuencial)
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
        std::cout << "Traceback completado en " << tiempo_fase3_ms << " ms\n";
        std::cout << "\nTiempo total fase 2 (llenado): " << tiempo_fase2_ms << " ms\n";
    }
    
    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}

ResultadoAlineamiento alineamientoNWR_OpenMP_Bloques(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config,
    int tile_size
) {
    int m = secA.length();
    int n = secB.length();
    
    if (!verificarLimitesSecuencia(m, n, false)) {
        std::cerr << "Alineamiento cancelado por límites de memoria.\n";
        return ResultadoAlineamiento("", "", 0, 0.0, 0.0, 0.0);
    }
    
    int num_tiles_i = (m + tile_size - 1) / tile_size;
    int num_tiles_j = (n + tile_size - 1) / tile_size;
    int num_ondas = num_tiles_i + num_tiles_j - 1;
    
    if (config.verbose) {
        std::cout << "=== PARALELIZACIÓN: BLOQUES (sin traceback matrix) ===\n";
        std::cout << "Threads disponibles: " << omp_get_max_threads() << "\n";
        std::cout << "Bloques: " << num_tiles_i << "×" << num_tiles_j << ", Ondas: " << num_ondas << "\n";
    }
    
    // FASE 1: Inicialización
    auto t_inicio_f1 = std::chrono::high_resolution_clock::now();
    
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));
    int gap = obtenerPenalidadGap(config.puntuacion);
    
    F[0][0] = 0;
    for (int i = 1; i <= m; ++i) F[i][0] = F[i-1][0] + gap;
    for (int j = 1; j <= n; ++j) F[0][j] = F[0][j-1] + gap;
    
    auto t_fin_f1 = std::chrono::high_resolution_clock::now();
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_f1 - t_inicio_f1).count();
    
    // FASE 2: Llenado paralelo por bloques
    auto t_inicio_f2 = std::chrono::high_resolution_clock::now();
    
    for (int tile_diag = 0; tile_diag < num_ondas; ++tile_diag) {
        int ti_start = std::max(0, tile_diag - num_tiles_j + 1);
        int ti_end = std::min(num_tiles_i - 1, tile_diag);
        
        #pragma omp parallel for schedule(dynamic)
        for (int ti = ti_start; ti <= ti_end; ++ti) {
            int tj = tile_diag - ti;
            
            int i_start = ti * tile_size + 1;
            int i_end = std::min((ti + 1) * tile_size, m);
            int j_start = tj * tile_size + 1;
            int j_end = std::min((tj + 1) * tile_size, n);
            
            for (int i = i_start; i <= i_end; ++i) {
                for (int j = j_start; j <= j_end; ++j) {
                    int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
                    int eliminacion = F[i-1][j] + gap;
                    int insercion = F[i][j-1] + gap;
                    F[i][j] = std::max({coincidencia, eliminacion, insercion});
                }
            }
        }
    }
    
    auto t_fin_f2 = std::chrono::high_resolution_clock::now();
    double tiempo_fase2_ms = std::chrono::duration<double, std::milli>(t_fin_f2 - t_inicio_f2).count();
    
    // FASE 3: Traceback con recálculo
    auto t_inicio_f3 = std::chrono::high_resolution_clock::now();
    
    std::string alineadaA = "";
    std::string alineadaB = "";
    int i = m, j = n;
    
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0) {
            int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
            int eliminacion = (i > 0) ? F[i-1][j] + gap : -999999;
            int insercion = (j > 0) ? F[i][j-1] + gap : -999999;
            
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
    
    auto t_fin_f3 = std::chrono::high_resolution_clock::now();
    double tiempo_fase3_ms = std::chrono::duration<double, std::milli>(t_fin_f3 - t_inicio_f3).count();
    
    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}

// ============================================================================
// ESTRATEGIA 3: TAREAS (TASKS)
// ============================================================================

ResultadoAlineamiento alineamientoNWP_OpenMP_Tasks(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
) {
    int m = secA.length();
    int n = secB.length();
    
    if (!verificarLimitesSecuencia(m, n, true)) {
        std::cerr << "Alineamiento cancelado por límites de memoria.\n";
        return ResultadoAlineamiento("", "", 0, 0.0, 0.0, 0.0);
    }
    
    if (config.verbose) {
        std::cout << "=== PARALELIZACIÓN: TAREAS (TASKS) ===\n";
        std::cout << "Threads disponibles: " << omp_get_max_threads() << "\n";
        std::cout << "NOTA: Requiere OpenMP 4.0+\n\n";
    }
    
    // FASE 1: Inicialización
    auto t_inicio_f1 = std::chrono::high_resolution_clock::now();
    
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));
    std::vector<std::vector<Direccion>> traceback(m + 1, std::vector<Direccion>(n + 1, Direccion::DIAGONAL));
    
    int gap = obtenerPenalidadGap(config.puntuacion);
    
    F[0][0] = 0;
    for (int i = 1; i <= m; ++i) {
        F[i][0] = F[i-1][0] + gap;
        traceback[i][0] = Direccion::ARRIBA;
    }
    for (int j = 1; j <= n; ++j) {
        F[0][j] = F[0][j-1] + gap;
        traceback[0][j] = Direccion::IZQUIERDA;
    }
    
    auto t_fin_f1 = std::chrono::high_resolution_clock::now();
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_f1 - t_inicio_f1).count();
    
    if (config.verbose) {
        std::cout << "Fase 1 completada en " << tiempo_fase1_ms << " ms\n";
        std::cout << "\n=== FASE 2: LLENADO CON TAREAS ===\n";
    }
    
    // FASE 2: Llenado usando tareas con dependencias
    auto t_inicio_f2 = std::chrono::high_resolution_clock::now();
    
    #pragma omp parallel
    {
        #pragma omp single
        {
            // Crear tareas por filas
            for (int i = 1; i <= m; ++i) {
                #pragma omp task shared(F, traceback, secA, secB, gap) firstprivate(i)
                {
                    for (int j = 1; j <= n; ++j) {
                        int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
                        int eliminacion = F[i-1][j] + gap;
                        int insercion = F[i][j-1] + gap;
                        
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
                
                // Sincronizar cada pocas filas para evitar demasiadas tareas pendientes
                if (i % 10 == 0) {
                    #pragma omp taskwait
                }
            }
            #pragma omp taskwait
        }
    }
    
    auto t_fin_f2 = std::chrono::high_resolution_clock::now();
    double tiempo_fase2_ms = std::chrono::duration<double, std::milli>(t_fin_f2 - t_inicio_f2).count();
    
    if (config.verbose) {
        std::cout << "Matriz calculada. Puntuación final: " << F[m][n] << "\n";
        std::cout << "Fase 2 completada en " << tiempo_fase2_ms << " ms\n";
        std::cout << "\n=== FASE 3: TRACEBACK ===\n";
    }
    
    // FASE 3: Traceback (secuencial)
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
        std::cout << "Traceback completado en " << tiempo_fase3_ms << " ms\n";
    }
    
    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}

ResultadoAlineamiento alineamientoNWR_OpenMP_Tasks(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
) {
    int m = secA.length();
    int n = secB.length();
    
    if (!verificarLimitesSecuencia(m, n, false)) {
        std::cerr << "Alineamiento cancelado por límites de memoria.\n";
        return ResultadoAlineamiento("", "", 0, 0.0, 0.0, 0.0);
    }
    
    if (config.verbose) {
        std::cout << "=== PARALELIZACIÓN: TAREAS (sin traceback matrix) ===\n";
        std::cout << "Threads disponibles: " << omp_get_max_threads() << "\n";
    }
    
    // FASE 1: Inicialización
    auto t_inicio_f1 = std::chrono::high_resolution_clock::now();
    
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));
    int gap = obtenerPenalidadGap(config.puntuacion);
    
    F[0][0] = 0;
    for (int i = 1; i <= m; ++i) F[i][0] = F[i-1][0] + gap;
    for (int j = 1; j <= n; ++j) F[0][j] = F[0][j-1] + gap;
    
    auto t_fin_f1 = std::chrono::high_resolution_clock::now();
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_f1 - t_inicio_f1).count();
    
    // FASE 2: Llenado usando tareas
    auto t_inicio_f2 = std::chrono::high_resolution_clock::now();
    
    #pragma omp parallel
    {
        #pragma omp single
        {
            for (int i = 1; i <= m; ++i) {
                #pragma omp task shared(F, secA, secB, gap) firstprivate(i)
                {
                    for (int j = 1; j <= n; ++j) {
                        int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
                        int eliminacion = F[i-1][j] + gap;
                        int insercion = F[i][j-1] + gap;
                        F[i][j] = std::max({coincidencia, eliminacion, insercion});
                    }
                }
                
                if (i % 10 == 0) {
                    #pragma omp taskwait
                }
            }
            #pragma omp taskwait
        }
    }
    
    auto t_fin_f2 = std::chrono::high_resolution_clock::now();
    double tiempo_fase2_ms = std::chrono::duration<double, std::milli>(t_fin_f2 - t_inicio_f2).count();
    
    // FASE 3: Traceback con recálculo
    auto t_inicio_f3 = std::chrono::high_resolution_clock::now();
    
    std::string alineadaA = "";
    std::string alineadaB = "";
    int i = m, j = n;
    
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0) {
            int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
            int eliminacion = (i > 0) ? F[i-1][j] + gap : -999999;
            int insercion = (j > 0) ? F[i][j-1] + gap : -999999;
            
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
    
    auto t_fin_f3 = std::chrono::high_resolution_clock::now();
    double tiempo_fase3_ms = std::chrono::duration<double, std::milli>(t_fin_f3 - t_inicio_f3).count();
    
    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}

