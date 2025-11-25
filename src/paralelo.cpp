#include "paralelo.h"
#include "puntuacion.h"
#include "utilidades.h"
#include <vector>
#include <algorithm>
#include <chrono>
#include <omp.h>

// ============================================================================
// ESTRATEGIA: ANTIDIAGONALES
// ============================================================================

/**
 * @brief Antidiagonales con schedule STATIC
 */
ResultadoAlineamiento alineamientoNWParaleloAntidiagonalStatic(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config) {
    
    int m = secA.length();
    int n = secB.length();
    
    auto t_inicio_fase1 = std::chrono::high_resolution_clock::now();
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));
    int penalidadGap = obtenerPenalidadGap(config.puntuacion);
    
    F[0][0] = 0;
    #pragma omp parallel for schedule(static)
    for (int i = 1; i <= m; ++i) {
        F[i][0] = F[i-1][0] + penalidadGap;
    }
    #pragma omp parallel for schedule(static)
    for (int j = 1; j <= n; ++j) {
        F[0][j] = F[0][j-1] + penalidadGap;
    }
    auto t_fin_fase1 = std::chrono::high_resolution_clock::now();
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_fase1 - t_inicio_fase1).count();
    
    auto t_inicio_fase2 = std::chrono::high_resolution_clock::now();
    
    for (int k = 2; k <= m + n; ++k) {
        int i_min = std::max(1, k - n);
        int i_max = std::min(m, k - 1);
        
        #pragma omp parallel for schedule(static) \
            shared(F, secA, secB, config, penalidadGap, k, i_min, i_max, n)
        for (int i = i_min; i <= i_max; ++i) {
            int j = k - i;
            if (j >= 1 && j <= n) {
                int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
                int eliminacion = F[i-1][j] + penalidadGap;
                int insercion = F[i][j-1] + penalidadGap;
                F[i][j] = std::max({coincidencia, eliminacion, insercion});
            }
        }
    }
    
    auto t_fin_fase2 = std::chrono::high_resolution_clock::now();
    double tiempo_fase2_ms = std::chrono::duration<double, std::milli>(t_fin_fase2 - t_inicio_fase2).count();
    
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
    
    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}

/**
 * @brief Antidiagonales con schedule DYNAMIC
 */
ResultadoAlineamiento alineamientoNWParaleloAntidiagonalDynamic(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config) {
    
    int m = secA.length();
    int n = secB.length();
    int num_threads = omp_get_max_threads();
    
    auto t_inicio_fase1 = std::chrono::high_resolution_clock::now();
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));
    int penalidadGap = obtenerPenalidadGap(config.puntuacion);
    
    F[0][0] = 0;
    #pragma omp parallel for schedule(static)
    for (int i = 1; i <= m; ++i) {
        F[i][0] = F[i-1][0] + penalidadGap;
    }
    #pragma omp parallel for schedule(static)
    for (int j = 1; j <= n; ++j) {
        F[0][j] = F[0][j-1] + penalidadGap;
    }
    auto t_fin_fase1 = std::chrono::high_resolution_clock::now();
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_fase1 - t_inicio_fase1).count();
    
    auto t_inicio_fase2 = std::chrono::high_resolution_clock::now();
    
    for (int k = 2; k <= m + n; ++k) {
        int i_min = std::max(1, k - n);
        int i_max = std::min(m, k - 1);
        int num_elementos = i_max - i_min + 1;
        
        if (num_elementos <= 0) continue;
        
        int chunk_size = std::max(1, num_elementos / (num_threads * 4));
        if (chunk_size < 1) chunk_size = 1;
        
        #pragma omp parallel for schedule(dynamic, chunk_size) \
            shared(F, secA, secB, config, penalidadGap, k, i_min, i_max, n)
        for (int i = i_min; i <= i_max; ++i) {
            int j = k - i;
            if (j >= 1 && j <= n) {
                int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
                int eliminacion = F[i-1][j] + penalidadGap;
                int insercion = F[i][j-1] + penalidadGap;
                F[i][j] = std::max({coincidencia, eliminacion, insercion});
            }
        }
    }
    
    auto t_fin_fase2 = std::chrono::high_resolution_clock::now();
    double tiempo_fase2_ms = std::chrono::duration<double, std::milli>(t_fin_fase2 - t_inicio_fase2).count();
    
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
    
    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}

/**
 * @brief Antidiagonales con schedule GUIDED
 */
ResultadoAlineamiento alineamientoNWParaleloAntidiagonalGuided(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config) {
    
    int m = secA.length();
    int n = secB.length();
    
    auto t_inicio_fase1 = std::chrono::high_resolution_clock::now();
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));
    int penalidadGap = obtenerPenalidadGap(config.puntuacion);
    
    F[0][0] = 0;
    #pragma omp parallel for schedule(static)
    for (int i = 1; i <= m; ++i) {
        F[i][0] = F[i-1][0] + penalidadGap;
    }
    #pragma omp parallel for schedule(static)
    for (int j = 1; j <= n; ++j) {
        F[0][j] = F[0][j-1] + penalidadGap;
    }
    auto t_fin_fase1 = std::chrono::high_resolution_clock::now();
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_fase1 - t_inicio_fase1).count();
    
    auto t_inicio_fase2 = std::chrono::high_resolution_clock::now();
    
    for (int k = 2; k <= m + n; ++k) {
        int i_min = std::max(1, k - n);
        int i_max = std::min(m, k - 1);
        
        #pragma omp parallel for schedule(guided, 1) \
            shared(F, secA, secB, config, penalidadGap, k, i_min, i_max, n)
        for (int i = i_min; i <= i_max; ++i) {
            int j = k - i;
            if (j >= 1 && j <= n) {
                int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
                int eliminacion = F[i-1][j] + penalidadGap;
                int insercion = F[i][j-1] + penalidadGap;
                F[i][j] = std::max({coincidencia, eliminacion, insercion});
            }
        }
    }
    
    auto t_fin_fase2 = std::chrono::high_resolution_clock::now();
    double tiempo_fase2_ms = std::chrono::duration<double, std::milli>(t_fin_fase2 - t_inicio_fase2).count();
    
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
    
    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}

/**
 * @brief Antidiagonales con schedule AUTO
 */
ResultadoAlineamiento alineamientoNWParaleloAntidiagonalAuto(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config) {
    
    int m = secA.length();
    int n = secB.length();
    
    auto t_inicio_fase1 = std::chrono::high_resolution_clock::now();
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));
    int penalidadGap = obtenerPenalidadGap(config.puntuacion);
    
    F[0][0] = 0;
    #pragma omp parallel for schedule(static)
    for (int i = 1; i <= m; ++i) {
        F[i][0] = F[i-1][0] + penalidadGap;
    }
    #pragma omp parallel for schedule(static)
    for (int j = 1; j <= n; ++j) {
        F[0][j] = F[0][j-1] + penalidadGap;
    }
    auto t_fin_fase1 = std::chrono::high_resolution_clock::now();
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_fase1 - t_inicio_fase1).count();
    
    auto t_inicio_fase2 = std::chrono::high_resolution_clock::now();
    
    for (int k = 2; k <= m + n; ++k) {
        int i_min = std::max(1, k - n);
        int i_max = std::min(m, k - 1);
        
        #pragma omp parallel for schedule(auto) \
            shared(F, secA, secB, config, penalidadGap, k, i_min, i_max, n)
        for (int i = i_min; i <= i_max; ++i) {
            int j = k - i;
            if (j >= 1 && j <= n) {
                int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
                int eliminacion = F[i-1][j] + penalidadGap;
                int insercion = F[i][j-1] + penalidadGap;
                F[i][j] = std::max({coincidencia, eliminacion, insercion});
            }
        }
    }
    
    auto t_fin_fase2 = std::chrono::high_resolution_clock::now();
    double tiempo_fase2_ms = std::chrono::duration<double, std::milli>(t_fin_fase2 - t_inicio_fase2).count();
    
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
    
    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}

// ============================================================================
// ESTRATEGIA: BLOQUES
// ============================================================================

/**
 * @brief Bloques con schedule STATIC
 */
ResultadoAlineamiento alineamientoNWParaleloBloquesStatic(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config) {
    
    int m = secA.length();
    int n = secB.length();
    
    int tam_bloque = std::max(50, std::min(m, n) / 10);
    if (tam_bloque < 10) tam_bloque = 10;
    
    int num_bloques_i = (m + tam_bloque - 1) / tam_bloque;
    int num_bloques_j = (n + tam_bloque - 1) / tam_bloque;
    
    auto t_inicio_fase1 = std::chrono::high_resolution_clock::now();
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));
    int penalidadGap = obtenerPenalidadGap(config.puntuacion);
    
    F[0][0] = 0;
    #pragma omp parallel for schedule(static)
    for (int i = 1; i <= m; ++i) {
        F[i][0] = F[i-1][0] + penalidadGap;
    }
    #pragma omp parallel for schedule(static)
    for (int j = 1; j <= n; ++j) {
        F[0][j] = F[0][j-1] + penalidadGap;
    }
    auto t_fin_fase1 = std::chrono::high_resolution_clock::now();
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_fase1 - t_inicio_fase1).count();
    
    auto t_inicio_fase2 = std::chrono::high_resolution_clock::now();
    
    for (int k = 0; k <= num_bloques_i + num_bloques_j - 2; ++k) {
        std::vector<std::pair<int, int>> bloques_en_antidiagonal;
        for (int bi = 0; bi < num_bloques_i; ++bi) {
            int bj = k - bi;
            if (bj >= 0 && bj < num_bloques_j) {
                bloques_en_antidiagonal.push_back({bi, bj});
            }
        }
        
        #pragma omp parallel for schedule(static) \
            shared(F, secA, secB, config, penalidadGap, bloques_en_antidiagonal, tam_bloque, m, n)
        for (size_t idx = 0; idx < bloques_en_antidiagonal.size(); ++idx) {
            int bi = bloques_en_antidiagonal[idx].first;
            int bj = bloques_en_antidiagonal[idx].second;
            
            int i_inicio = bi * tam_bloque + 1;
            int i_fin = std::min((bi + 1) * tam_bloque, m);
            int j_inicio = bj * tam_bloque + 1;
            int j_fin = std::min((bj + 1) * tam_bloque, n);
            
            for (int i = i_inicio; i <= i_fin; ++i) {
                for (int j = j_inicio; j <= j_fin; ++j) {
                    int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
                    int eliminacion = F[i-1][j] + penalidadGap;
                    int insercion = F[i][j-1] + penalidadGap;
                    F[i][j] = std::max({coincidencia, eliminacion, insercion});
                }
            }
        }
    }
    
    auto t_fin_fase2 = std::chrono::high_resolution_clock::now();
    double tiempo_fase2_ms = std::chrono::duration<double, std::milli>(t_fin_fase2 - t_inicio_fase2).count();
    
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
    
    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}

/**
 * @brief Bloques con schedule DYNAMIC
 */
ResultadoAlineamiento alineamientoNWParaleloBloquesDynamic(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config) {
    
    int m = secA.length();
    int n = secB.length();
    
    int tam_bloque = std::max(50, std::min(m, n) / 10);
    if (tam_bloque < 10) tam_bloque = 10;
    
    int num_bloques_i = (m + tam_bloque - 1) / tam_bloque;
    int num_bloques_j = (n + tam_bloque - 1) / tam_bloque;
    
    auto t_inicio_fase1 = std::chrono::high_resolution_clock::now();
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));
    int penalidadGap = obtenerPenalidadGap(config.puntuacion);
    
    F[0][0] = 0;
    #pragma omp parallel for schedule(static)
    for (int i = 1; i <= m; ++i) {
        F[i][0] = F[i-1][0] + penalidadGap;
    }
    #pragma omp parallel for schedule(static)
    for (int j = 1; j <= n; ++j) {
        F[0][j] = F[0][j-1] + penalidadGap;
    }
    auto t_fin_fase1 = std::chrono::high_resolution_clock::now();
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_fase1 - t_inicio_fase1).count();
    
    auto t_inicio_fase2 = std::chrono::high_resolution_clock::now();
    
    for (int k = 0; k <= num_bloques_i + num_bloques_j - 2; ++k) {
        std::vector<std::pair<int, int>> bloques_en_antidiagonal;
        for (int bi = 0; bi < num_bloques_i; ++bi) {
            int bj = k - bi;
            if (bj >= 0 && bj < num_bloques_j) {
                bloques_en_antidiagonal.push_back({bi, bj});
            }
        }
        
        #pragma omp parallel for schedule(dynamic, 1) \
            shared(F, secA, secB, config, penalidadGap, bloques_en_antidiagonal, tam_bloque, m, n)
        for (size_t idx = 0; idx < bloques_en_antidiagonal.size(); ++idx) {
            int bi = bloques_en_antidiagonal[idx].first;
            int bj = bloques_en_antidiagonal[idx].second;
            
            int i_inicio = bi * tam_bloque + 1;
            int i_fin = std::min((bi + 1) * tam_bloque, m);
            int j_inicio = bj * tam_bloque + 1;
            int j_fin = std::min((bj + 1) * tam_bloque, n);
            
            for (int i = i_inicio; i <= i_fin; ++i) {
                for (int j = j_inicio; j <= j_fin; ++j) {
                    int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
                    int eliminacion = F[i-1][j] + penalidadGap;
                    int insercion = F[i][j-1] + penalidadGap;
                    F[i][j] = std::max({coincidencia, eliminacion, insercion});
                }
            }
        }
    }
    
    auto t_fin_fase2 = std::chrono::high_resolution_clock::now();
    double tiempo_fase2_ms = std::chrono::duration<double, std::milli>(t_fin_fase2 - t_inicio_fase2).count();
    
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
    
    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}

/**
 * @brief Bloques con schedule GUIDED
 */
ResultadoAlineamiento alineamientoNWParaleloBloquesGuided(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config) {
    
    int m = secA.length();
    int n = secB.length();
    
    int tam_bloque = std::max(50, std::min(m, n) / 10);
    if (tam_bloque < 10) tam_bloque = 10;
    
    int num_bloques_i = (m + tam_bloque - 1) / tam_bloque;
    int num_bloques_j = (n + tam_bloque - 1) / tam_bloque;
    
    auto t_inicio_fase1 = std::chrono::high_resolution_clock::now();
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));
    int penalidadGap = obtenerPenalidadGap(config.puntuacion);
    
    F[0][0] = 0;
    #pragma omp parallel for schedule(static)
    for (int i = 1; i <= m; ++i) {
        F[i][0] = F[i-1][0] + penalidadGap;
    }
    #pragma omp parallel for schedule(static)
    for (int j = 1; j <= n; ++j) {
        F[0][j] = F[0][j-1] + penalidadGap;
    }
    auto t_fin_fase1 = std::chrono::high_resolution_clock::now();
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_fase1 - t_inicio_fase1).count();
    
    auto t_inicio_fase2 = std::chrono::high_resolution_clock::now();
    
    for (int k = 0; k <= num_bloques_i + num_bloques_j - 2; ++k) {
        std::vector<std::pair<int, int>> bloques_en_antidiagonal;
        for (int bi = 0; bi < num_bloques_i; ++bi) {
            int bj = k - bi;
            if (bj >= 0 && bj < num_bloques_j) {
                bloques_en_antidiagonal.push_back({bi, bj});
            }
        }
        
        #pragma omp parallel for schedule(guided, 1) \
            shared(F, secA, secB, config, penalidadGap, bloques_en_antidiagonal, tam_bloque, m, n)
        for (size_t idx = 0; idx < bloques_en_antidiagonal.size(); ++idx) {
            int bi = bloques_en_antidiagonal[idx].first;
            int bj = bloques_en_antidiagonal[idx].second;
            
            int i_inicio = bi * tam_bloque + 1;
            int i_fin = std::min((bi + 1) * tam_bloque, m);
            int j_inicio = bj * tam_bloque + 1;
            int j_fin = std::min((bj + 1) * tam_bloque, n);
            
            for (int i = i_inicio; i <= i_fin; ++i) {
                for (int j = j_inicio; j <= j_fin; ++j) {
                    int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
                    int eliminacion = F[i-1][j] + penalidadGap;
                    int insercion = F[i][j-1] + penalidadGap;
                    F[i][j] = std::max({coincidencia, eliminacion, insercion});
                }
            }
        }
    }
    
    auto t_fin_fase2 = std::chrono::high_resolution_clock::now();
    double tiempo_fase2_ms = std::chrono::duration<double, std::milli>(t_fin_fase2 - t_inicio_fase2).count();
    
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
    
    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}

/**
 * @brief Bloques con schedule AUTO
 */
ResultadoAlineamiento alineamientoNWParaleloBloquesAuto(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config) {
    
    int m = secA.length();
    int n = secB.length();
    
    int tam_bloque = std::max(50, std::min(m, n) / 10);
    if (tam_bloque < 10) tam_bloque = 10;
    
    int num_bloques_i = (m + tam_bloque - 1) / tam_bloque;
    int num_bloques_j = (n + tam_bloque - 1) / tam_bloque;
    
    auto t_inicio_fase1 = std::chrono::high_resolution_clock::now();
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));
    int penalidadGap = obtenerPenalidadGap(config.puntuacion);
    
    F[0][0] = 0;
    #pragma omp parallel for schedule(static)
    for (int i = 1; i <= m; ++i) {
        F[i][0] = F[i-1][0] + penalidadGap;
    }
    #pragma omp parallel for schedule(static)
    for (int j = 1; j <= n; ++j) {
        F[0][j] = F[0][j-1] + penalidadGap;
    }
    auto t_fin_fase1 = std::chrono::high_resolution_clock::now();
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_fase1 - t_inicio_fase1).count();
    
    auto t_inicio_fase2 = std::chrono::high_resolution_clock::now();
    
    for (int k = 0; k <= num_bloques_i + num_bloques_j - 2; ++k) {
        std::vector<std::pair<int, int>> bloques_en_antidiagonal;
        for (int bi = 0; bi < num_bloques_i; ++bi) {
            int bj = k - bi;
            if (bj >= 0 && bj < num_bloques_j) {
                bloques_en_antidiagonal.push_back({bi, bj});
            }
        }
        
        #pragma omp parallel for schedule(auto) \
            shared(F, secA, secB, config, penalidadGap, bloques_en_antidiagonal, tam_bloque, m, n)
        for (size_t idx = 0; idx < bloques_en_antidiagonal.size(); ++idx) {
            int bi = bloques_en_antidiagonal[idx].first;
            int bj = bloques_en_antidiagonal[idx].second;
            
            int i_inicio = bi * tam_bloque + 1;
            int i_fin = std::min((bi + 1) * tam_bloque, m);
            int j_inicio = bj * tam_bloque + 1;
            int j_fin = std::min((bj + 1) * tam_bloque, n);
            
            for (int i = i_inicio; i <= i_fin; ++i) {
                for (int j = j_inicio; j <= j_fin; ++j) {
                    int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
                    int eliminacion = F[i-1][j] + penalidadGap;
                    int insercion = F[i][j-1] + penalidadGap;
                    F[i][j] = std::max({coincidencia, eliminacion, insercion});
                }
            }
        }
    }
    
    auto t_fin_fase2 = std::chrono::high_resolution_clock::now();
    double tiempo_fase2_ms = std::chrono::duration<double, std::milli>(t_fin_fase2 - t_inicio_fase2).count();
    
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
    
    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}

// ============================================================================
// ESTRATEGIA: COMBINADO (ANTIDIAGONALES + BLOQUES)
// ============================================================================

/**
 * @brief Combinado con schedule STATIC
 */
ResultadoAlineamiento alineamientoNWParaleloCombinadoStatic(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config) {
    
    int m = secA.length();
    int n = secB.length();
    
    int tam_bloque_antidiag = std::max(20, std::min(m, n) / 20);
    if (tam_bloque_antidiag < 5) tam_bloque_antidiag = 5;
    
    auto t_inicio_fase1 = std::chrono::high_resolution_clock::now();
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));
    int penalidadGap = obtenerPenalidadGap(config.puntuacion);
    
    F[0][0] = 0;
    #pragma omp parallel for schedule(static)
    for (int i = 1; i <= m; ++i) {
        F[i][0] = F[i-1][0] + penalidadGap;
    }
    #pragma omp parallel for schedule(static)
    for (int j = 1; j <= n; ++j) {
        F[0][j] = F[0][j-1] + penalidadGap;
    }
    auto t_fin_fase1 = std::chrono::high_resolution_clock::now();
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_fase1 - t_inicio_fase1).count();
    
    auto t_inicio_fase2 = std::chrono::high_resolution_clock::now();
    
    for (int k = 2; k <= m + n; ++k) {
        int i_min = std::max(1, k - n);
        int i_max = std::min(m, k - 1);
        int num_elementos = i_max - i_min + 1;
        
        if (num_elementos <= 0) continue;
        
        int num_bloques = (num_elementos + tam_bloque_antidiag - 1) / tam_bloque_antidiag;
        
        #pragma omp parallel for schedule(static) \
            shared(F, secA, secB, config, penalidadGap, k, i_min, i_max, n, tam_bloque_antidiag)
        for (int bloque = 0; bloque < num_bloques; ++bloque) {
            int inicio_bloque = i_min + bloque * tam_bloque_antidiag;
            int fin_bloque = std::min(inicio_bloque + tam_bloque_antidiag - 1, i_max);
            
            for (int i = inicio_bloque; i <= fin_bloque; ++i) {
                int j = k - i;
                if (j >= 1 && j <= n) {
                    int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
                    int eliminacion = F[i-1][j] + penalidadGap;
                    int insercion = F[i][j-1] + penalidadGap;
                    F[i][j] = std::max({coincidencia, eliminacion, insercion});
                }
            }
        }
    }
    
    auto t_fin_fase2 = std::chrono::high_resolution_clock::now();
    double tiempo_fase2_ms = std::chrono::duration<double, std::milli>(t_fin_fase2 - t_inicio_fase2).count();
    
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
    
    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}

/**
 * @brief Combinado con schedule DYNAMIC
 */
ResultadoAlineamiento alineamientoNWParaleloCombinadoDynamic(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config) {
    
    int m = secA.length();
    int n = secB.length();
    int num_threads = omp_get_max_threads();
    
    int tam_bloque_antidiag = std::max(20, std::min(m, n) / 20);
    if (tam_bloque_antidiag < 5) tam_bloque_antidiag = 5;
    
    auto t_inicio_fase1 = std::chrono::high_resolution_clock::now();
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));
    int penalidadGap = obtenerPenalidadGap(config.puntuacion);
    
    F[0][0] = 0;
    #pragma omp parallel for schedule(static)
    for (int i = 1; i <= m; ++i) {
        F[i][0] = F[i-1][0] + penalidadGap;
    }
    #pragma omp parallel for schedule(static)
    for (int j = 1; j <= n; ++j) {
        F[0][j] = F[0][j-1] + penalidadGap;
    }
    auto t_fin_fase1 = std::chrono::high_resolution_clock::now();
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_fase1 - t_inicio_fase1).count();
    
    auto t_inicio_fase2 = std::chrono::high_resolution_clock::now();
    
    for (int k = 2; k <= m + n; ++k) {
        int i_min = std::max(1, k - n);
        int i_max = std::min(m, k - 1);
        int num_elementos = i_max - i_min + 1;
        
        if (num_elementos <= 0) continue;
        
        int num_bloques = (num_elementos + tam_bloque_antidiag - 1) / tam_bloque_antidiag;
        int chunk_size = std::max(1, num_bloques / (num_threads * 2));
        if (chunk_size < 1) chunk_size = 1;
        
        #pragma omp parallel for schedule(dynamic, chunk_size) \
            shared(F, secA, secB, config, penalidadGap, k, i_min, i_max, n, tam_bloque_antidiag)
        for (int bloque = 0; bloque < num_bloques; ++bloque) {
            int inicio_bloque = i_min + bloque * tam_bloque_antidiag;
            int fin_bloque = std::min(inicio_bloque + tam_bloque_antidiag - 1, i_max);
            
            for (int i = inicio_bloque; i <= fin_bloque; ++i) {
                int j = k - i;
                if (j >= 1 && j <= n) {
                    int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
                    int eliminacion = F[i-1][j] + penalidadGap;
                    int insercion = F[i][j-1] + penalidadGap;
                    F[i][j] = std::max({coincidencia, eliminacion, insercion});
                }
            }
        }
    }
    
    auto t_fin_fase2 = std::chrono::high_resolution_clock::now();
    double tiempo_fase2_ms = std::chrono::duration<double, std::milli>(t_fin_fase2 - t_inicio_fase2).count();
    
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
    
    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}

/**
 * @brief Combinado con schedule GUIDED
 */
ResultadoAlineamiento alineamientoNWParaleloCombinadoGuided(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config) {
    
    int m = secA.length();
    int n = secB.length();
    
    int tam_bloque_antidiag = std::max(20, std::min(m, n) / 20);
    if (tam_bloque_antidiag < 5) tam_bloque_antidiag = 5;
    
    auto t_inicio_fase1 = std::chrono::high_resolution_clock::now();
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));
    int penalidadGap = obtenerPenalidadGap(config.puntuacion);
    
    F[0][0] = 0;
    #pragma omp parallel for schedule(static)
    for (int i = 1; i <= m; ++i) {
        F[i][0] = F[i-1][0] + penalidadGap;
    }
    #pragma omp parallel for schedule(static)
    for (int j = 1; j <= n; ++j) {
        F[0][j] = F[0][j-1] + penalidadGap;
    }
    auto t_fin_fase1 = std::chrono::high_resolution_clock::now();
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_fase1 - t_inicio_fase1).count();
    
    auto t_inicio_fase2 = std::chrono::high_resolution_clock::now();
    
    for (int k = 2; k <= m + n; ++k) {
        int i_min = std::max(1, k - n);
        int i_max = std::min(m, k - 1);
        int num_elementos = i_max - i_min + 1;
        
        if (num_elementos <= 0) continue;
        
        int num_bloques = (num_elementos + tam_bloque_antidiag - 1) / tam_bloque_antidiag;
        
        #pragma omp parallel for schedule(guided, 1) \
            shared(F, secA, secB, config, penalidadGap, k, i_min, i_max, n, tam_bloque_antidiag)
        for (int bloque = 0; bloque < num_bloques; ++bloque) {
            int inicio_bloque = i_min + bloque * tam_bloque_antidiag;
            int fin_bloque = std::min(inicio_bloque + tam_bloque_antidiag - 1, i_max);
            
            for (int i = inicio_bloque; i <= fin_bloque; ++i) {
                int j = k - i;
                if (j >= 1 && j <= n) {
                    int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
                    int eliminacion = F[i-1][j] + penalidadGap;
                    int insercion = F[i][j-1] + penalidadGap;
                    F[i][j] = std::max({coincidencia, eliminacion, insercion});
                }
            }
        }
    }
    
    auto t_fin_fase2 = std::chrono::high_resolution_clock::now();
    double tiempo_fase2_ms = std::chrono::duration<double, std::milli>(t_fin_fase2 - t_inicio_fase2).count();
    
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
    
    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}

/**
 * @brief Combinado con schedule AUTO
 */
ResultadoAlineamiento alineamientoNWParaleloCombinadoAuto(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config) {
    
    int m = secA.length();
    int n = secB.length();
    
    int tam_bloque_antidiag = std::max(20, std::min(m, n) / 20);
    if (tam_bloque_antidiag < 5) tam_bloque_antidiag = 5;
    
    auto t_inicio_fase1 = std::chrono::high_resolution_clock::now();
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));
    int penalidadGap = obtenerPenalidadGap(config.puntuacion);
    
    F[0][0] = 0;
    #pragma omp parallel for schedule(static)
    for (int i = 1; i <= m; ++i) {
        F[i][0] = F[i-1][0] + penalidadGap;
    }
    #pragma omp parallel for schedule(static)
    for (int j = 1; j <= n; ++j) {
        F[0][j] = F[0][j-1] + penalidadGap;
    }
    auto t_fin_fase1 = std::chrono::high_resolution_clock::now();
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_fase1 - t_inicio_fase1).count();
    
    auto t_inicio_fase2 = std::chrono::high_resolution_clock::now();
    
    for (int k = 2; k <= m + n; ++k) {
        int i_min = std::max(1, k - n);
        int i_max = std::min(m, k - 1);
        int num_elementos = i_max - i_min + 1;
        
        if (num_elementos <= 0) continue;
        
        int num_bloques = (num_elementos + tam_bloque_antidiag - 1) / tam_bloque_antidiag;
        
        #pragma omp parallel for schedule(auto) \
            shared(F, secA, secB, config, penalidadGap, k, i_min, i_max, n, tam_bloque_antidiag)
        for (int bloque = 0; bloque < num_bloques; ++bloque) {
            int inicio_bloque = i_min + bloque * tam_bloque_antidiag;
            int fin_bloque = std::min(inicio_bloque + tam_bloque_antidiag - 1, i_max);
            
            for (int i = inicio_bloque; i <= fin_bloque; ++i) {
                int j = k - i;
                if (j >= 1 && j <= n) {
                    int coincidencia = F[i-1][j-1] + obtenerPuntuacionConConfig(secA[i-1], secB[j-1], config.puntuacion);
                    int eliminacion = F[i-1][j] + penalidadGap;
                    int insercion = F[i][j-1] + penalidadGap;
                    F[i][j] = std::max({coincidencia, eliminacion, insercion});
                }
            }
        }
    }
    
    auto t_fin_fase2 = std::chrono::high_resolution_clock::now();
    double tiempo_fase2_ms = std::chrono::duration<double, std::milli>(t_fin_fase2 - t_inicio_fase2).count();
    
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
    
    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}
