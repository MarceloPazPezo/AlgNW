#include "paralelo.h"
#include "puntuacion.h"
#include <vector>
#include <algorithm>
#include <chrono>
#include <omp.h>

// Soporte para eventos de Extrae (opcional)
#ifdef HAVE_EXTRAE
#include <extrae.h>
#endif

/**
 * @brief Alineamiento paralelo usando estrategia de antidiagonales
 * 
 * Solo paraleliza la fase 2 (llenado de matriz).
 * El schedule se lee de la variable de entorno OMP_SCHEDULE.
 * 
 * MEJORAS IMPLEMENTADAS:
 * - Usa schedule(runtime) para permitir experimentación con diferentes planificadores
 * - Variables firstprivate para evitar false sharing
 * - Mejor especificación de variables compartidas/privadas
 * 
 * RECOMENDACIÓN: Para antidiagonales con tamaño variable (pequeñas al inicio/final,
 * grandes en el centro), usar schedule dynamic o guided para mejor balance de carga.
 * 
 * @param secA Secuencia A (DNA)
 * @param secB Secuencia B (DNA)
 * @param config Configuración de alineamiento
 * @return ResultadoAlineamiento con puntuación y tiempos
 */
ResultadoAlineamiento alineamientoNWParaleloAntidiagonal(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config) {
    
    int m = secA.length();
    int n = secB.length();
    
    // FASE 1: Inicialización
    auto t_inicio_fase1 = std::chrono::high_resolution_clock::now();
#ifdef HAVE_EXTRAE
    Extrae_event(1000, 1);
#endif
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));
    int penalidadGap = obtenerPenalidadGapDNA(config.puntuacion);
    
    F[0][0] = 0;
    for (int i = 1; i <= m; ++i) {
        F[i][0] = F[i-1][0] + penalidadGap;
    }
    for (int j = 1; j <= n; ++j) {
        F[0][j] = F[0][j-1] + penalidadGap;
    }
    auto t_fin_fase1 = std::chrono::high_resolution_clock::now();
#ifdef HAVE_EXTRAE
    Extrae_event(1000, 0);
#endif
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_fase1 - t_inicio_fase1).count();
    
    // FASE 2: Llenado de matriz
    auto t_inicio_fase2 = std::chrono::high_resolution_clock::now();
#ifdef HAVE_EXTRAE
    Extrae_event(2000, 1);
#endif
    for (int k = 2; k <= m + n; ++k) {
#ifdef HAVE_EXTRAE
	Extrae_event(3000, k);
#endif
        int i_min = std::max(1, k - n);
        int i_max = std::min(m, k - 1);
        #pragma omp parallel for schedule(runtime) \
            firstprivate(i_min, i_max, k, n)
        for (int i = i_min; i <= i_max; ++i) {
#ifdef HAVE_EXTRAE
            Extrae_event(4000, i+1);
#endif
            int j = k - i;
            if (j >= 1 && j <= n) {
                int coincidencia = F[i-1][j-1] + obtenerPuntuacionDNA(secA[i-1], secB[j-1], config.puntuacion);
                int eliminacion = F[i-1][j] + penalidadGap;
                int insercion = F[i][j-1] + penalidadGap;
                F[i][j] = std::max({coincidencia, eliminacion, insercion});
            }
#ifdef HAVE_EXTRAE
            Extrae_event(4000, 0);
#endif
        }
#ifdef HAVE_EXTRAE
	Extrae_event(3000, 0);
#endif
    }
    
    auto t_fin_fase2 = std::chrono::high_resolution_clock::now();
#ifdef HAVE_EXTRAE
    Extrae_event(2000, 0);
#endif
    double tiempo_fase2_ms = std::chrono::duration<double, std::milli>(t_fin_fase2 - t_inicio_fase2).count();
    
    // FASE 3: Traceback
    auto t_inicio_fase3 = std::chrono::high_resolution_clock::now();
#ifdef HAVE_EXTRAE
    Extrae_event(5000, 1);
#endif
    std::string alineadaA = "";
    std::string alineadaB = "";
    int i = m, j = n;
    
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0) {
            int coincidencia = F[i-1][j-1] + obtenerPuntuacionDNA(secA[i-1], secB[j-1], config.puntuacion);
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
#ifdef HAVE_EXTRAE
    Extrae_event(5000, 0);
#endif
    double tiempo_fase3_ms = std::chrono::duration<double, std::milli>(t_fin_fase3 - t_inicio_fase3).count();
    
    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}

/**
 * @brief Alineamiento paralelo usando estrategia de bloques
 * 
 * Solo paraleliza la fase 2 (llenado de matriz).
 * 
 * MEJORAS IMPLEMENTADAS:
 * - Tamaño de bloque optimizado para arquitectura de caché (64-128 elementos)
 * - Permite que múltiples bloques vivan en L2 simultáneamente, reduciendo conflictos
 * - Tamaño adaptativo basado en número de threads disponibles
 * - Variables firstprivate para evitar false sharing
 * - Mejor especificación de variables compartidas/privadas
 * 
 * @param secA Secuencia A (DNA)
 * @param secB Secuencia B (DNA)
 * @param config Configuración de alineamiento
 * @return ResultadoAlineamiento con puntuación y tiempos
 */
ResultadoAlineamiento alineamientoNWParaleloBloques(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config) {
    
    int m = secA.length();
    int n = secB.length();
    int num_threads = omp_get_max_threads();
    // Optimización basada en arquitectura de caché:
    // - L2: 256 KB, L3: 8 MB compartida entre 4 núcleos
    // - Bloque de 256x256 = 256 KB (exactamente L2) causa conflictos con múltiples threads
    // - Bloques de 64-128 elementos (16-64 KB) permiten que múltiples bloques vivan en L2
    // - Esto reduce conflictos de caché y mejora el paralelismo
    int tam_bloque = std::min(m, n) / (num_threads * 2);
    // Límites: mínimo 64 (cabe en L1d), máximo 128 (múltiples bloques en L2)
    if (tam_bloque < 64) tam_bloque = 64;
    if (tam_bloque > 128) tam_bloque = 128;
    int num_bloques_i = (m + tam_bloque - 1) / tam_bloque;
    int num_bloques_j = (n + tam_bloque - 1) / tam_bloque;
    
    // FASE 1: Inicialización
    auto t_inicio_fase1 = std::chrono::high_resolution_clock::now();
#ifdef HAVE_EXTRAE
    Extrae_event(1000, 1);
#endif
    std::vector<std::vector<int>> F(m + 1, std::vector<int>(n + 1, 0));
    int penalidadGap = obtenerPenalidadGapDNA(config.puntuacion);
    
    F[0][0] = 0;
    for (int i = 1; i <= m; ++i) {
        F[i][0] = F[i-1][0] + penalidadGap;
    }
    for (int j = 1; j <= n; ++j) {
        F[0][j] = F[0][j-1] + penalidadGap;
    }
    auto t_fin_fase1 = std::chrono::high_resolution_clock::now();
#ifdef HAVE_EXTRAE
    Extrae_event(1000, 0);
#endif
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_fase1 - t_inicio_fase1).count();
    
    // FASE 2: Llenado de matriz por bloques
    auto t_inicio_fase2 = std::chrono::high_resolution_clock::now();
#ifdef HAVE_EXTRAE
    Extrae_event(2000, 1);
#endif
    
    for (int k = 0; k <= num_bloques_i + num_bloques_j - 2; ++k) {
#ifdef HAVE_EXTRAE
	Extrae_event(3000, k+1);
#endif
        std::vector<std::pair<int, int>> bloques_en_antidiagonal;
        for (int bi = 0; bi < num_bloques_i; ++bi) {
            int bj = k - bi;
            if (bj >= 0 && bj < num_bloques_j) {
                bloques_en_antidiagonal.push_back({bi, bj});
            }
        }
        
        #pragma omp parallel for schedule(runtime) \
            firstprivate(k)
        for (size_t idx = 0; idx < bloques_en_antidiagonal.size(); ++idx) {
#ifdef HAVE_EXTRAE
            Extrae_event(4000, static_cast<int>(idx) + 1);
#endif
            int bi = bloques_en_antidiagonal[idx].first;
            int bj = bloques_en_antidiagonal[idx].second;
            
            int i_inicio = bi * tam_bloque + 1;
            int i_fin = std::min((bi + 1) * tam_bloque, m);
            int j_inicio = bj * tam_bloque + 1;
            int j_fin = std::min((bj + 1) * tam_bloque, n);
            
            for (int i = i_inicio; i <= i_fin; ++i) {
                for (int j = j_inicio; j <= j_fin; ++j) {
                    int coincidencia = F[i-1][j-1] + obtenerPuntuacionDNA(secA[i-1], secB[j-1], config.puntuacion);
                    int eliminacion = F[i-1][j] + penalidadGap;
                    int insercion = F[i][j-1] + penalidadGap;
                    F[i][j] = std::max({coincidencia, eliminacion, insercion});
                }
            }
#ifdef HAVE_EXTRAE
            Extrae_event(4000, 0);
#endif
        }
#ifdef HAVE_EXTRAE
	Extrae_event(3000, 0);
#endif
    }
    
    auto t_fin_fase2 = std::chrono::high_resolution_clock::now();
#ifdef HAVE_EXTRAE
    Extrae_event(2000, 0);
#endif
    double tiempo_fase2_ms = std::chrono::duration<double, std::milli>(t_fin_fase2 - t_inicio_fase2).count();
    
    // FASE 3: Traceback
    auto t_inicio_fase3 = std::chrono::high_resolution_clock::now();
#ifdef HAVE_EXTRAE
    Extrae_event(5000, 1);
#endif
    std::string alineadaA = "";
    std::string alineadaB = "";
    int i = m, j = n;
    
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0) {
            int coincidencia = F[i-1][j-1] + obtenerPuntuacionDNA(secA[i-1], secB[j-1], config.puntuacion);
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
#ifdef HAVE_EXTRAE
    Extrae_event(5000, 0);
#endif
    double tiempo_fase3_ms = std::chrono::duration<double, std::milli>(t_fin_fase3 - t_inicio_fase3).count();
    
    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}

