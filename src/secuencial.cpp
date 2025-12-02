#include "secuencial.h"
#include "puntuacion.h"
#include <vector>
#include <algorithm>
#include <chrono>

/**
 * @brief Ejecuta Needleman–Wunsch con recálculo del traceback (optimizado en memoria).
 *
 * Calcula únicamente la matriz de puntuaciones y durante el traceback vuelve a
 * calcular las decisiones, ahorrando memoria a costa de algún coste adicional de CPU durante el traceback.
 * 
 * @param secA Secuencia A (DNA)
 * @param secB Secuencia B (DNA)
 * @param config Configuración de alineamiento
 * @return ResultadoAlineamiento con puntuación y tiempos
 */
ResultadoAlineamiento AlgNW(const std::string& secA, const std::string& secB, 
                            const ConfiguracionAlineamiento& config) {
    int m = secA.length();
    int n = secB.length();
    
    auto t_inicio_fase1 = std::chrono::high_resolution_clock::now();
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
    auto t_fin_fase1 = std::chrono::high_resolution_clock::now();
    double tiempo_fase1_ms = std::chrono::duration<double, std::milli>(t_fin_fase1 - t_inicio_fase1).count();
    
    // Llenar la matriz
    auto t_inicio_fase2 = std::chrono::high_resolution_clock::now();
    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            int coincidencia = F[i-1][j-1] + obtenerPuntuacionDNA(secA[i-1], secB[j-1], config.puntuacion);
            int eliminacion = F[i-1][j] + penalidadGap;
            int insercion = F[i][j-1] + penalidadGap;
            
            F[i][j] = std::max({coincidencia, eliminacion, insercion});
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
    double tiempo_fase3_ms = std::chrono::duration<double, std::milli>(t_fin_fase3 - t_inicio_fase3).count();

    return ResultadoAlineamiento(alineadaA, alineadaB, F[m][n], tiempo_fase2_ms, tiempo_fase3_ms, tiempo_fase1_ms);
}