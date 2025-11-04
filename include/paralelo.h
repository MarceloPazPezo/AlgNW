#ifndef PARALELO_H
#define PARALELO_H

#include <string>
#include "tipos.h"

/**
 * @file paralelo.h
 * @brief Implementaciones paralelas de Needleman-Wunsch con OpenMP.
 * 
 * Este archivo contiene diferentes estrategias de paralelización:
 * 1. Antidiagonales (wavefront celda por celda)
 * 2. Bloques (tiling/blocking)
 * 3. Tareas (tasks con dependencias)
 */

// ============================================================================
// ESTRATEGIA 1: ANTIDIAGONALES (WAVEFRONT)
// ============================================================================

/**
 * @brief NW con matriz de traceback paralelo por antidiagonales.
 * 
 * Procesa la matriz por antidiagonales (wavefront). Las celdas en la misma
 * antidiagonal son independientes y se calculan en paralelo.
 * 
 * Características:
 * - Paralelismo variable (1 a min(m,n) celdas por onda)
 * - Muchas sincronizaciones (m+n-1 barreras)
 * - Simple de implementar
 * - Bueno para matrices pequeñas-medianas
 * 
 * @param secA Secuencia A
 * @param secB Secuencia B
 * @param config Configuración de alineamiento
 * @return ResultadoAlineamiento con tiempos instrumentados
 */
ResultadoAlineamiento alineamientoNWP_OpenMP_Antidiag(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
);

/**
 * @brief NW con recálculo de traceback paralelo por antidiagonales.
 * 
 * Versión optimizada en memoria. Solo almacena matriz de puntuaciones.
 * 
 * @param secA Secuencia A
 * @param secB Secuencia B
 * @param config Configuración de alineamiento
 * @return ResultadoAlineamiento con tiempos instrumentados
 */
ResultadoAlineamiento alineamientoNWR_OpenMP_Antidiag(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
);

// ============================================================================
// ESTRATEGIA 2: BLOQUES (TILING)
// ============================================================================

/**
 * @brief NW con matriz de traceback paralelo por bloques.
 * 
 * Divide la matriz en bloques (tiles) y los procesa por antidiagonales de
 * bloques. Bloques en la misma antidiagonal se calculan en paralelo.
 * 
 * Características:
 * - Pocas sincronizaciones (mucho menos que antidiagonales)
 * - Excelente localidad de cache
 * - Paralelismo más estable
 * - Óptimo para matrices grandes
 * 
 * @param secA Secuencia A
 * @param secB Secuencia B
 * @param config Configuración de alineamiento
 * @param tile_size Tamaño del bloque (por defecto 64)
 * @return ResultadoAlineamiento con tiempos instrumentados
 */
ResultadoAlineamiento alineamientoNWP_OpenMP_Bloques(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config,
    int tile_size = 64
);

/**
 * @brief NW con recálculo de traceback paralelo por bloques.
 * 
 * Versión optimizada en memoria con estrategia de bloques.
 * 
 * @param secA Secuencia A
 * @param secB Secuencia B
 * @param config Configuración de alineamiento
 * @param tile_size Tamaño del bloque (por defecto 64)
 * @return ResultadoAlineamiento con tiempos instrumentados
 */
ResultadoAlineamiento alineamientoNWR_OpenMP_Bloques(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config,
    int tile_size = 64
);

// ============================================================================
// ESTRATEGIA 3: TAREAS (TASKS)
// ============================================================================

/**
 * @brief NW con matriz de traceback usando tareas de OpenMP.
 * 
 * Usa el sistema de tareas con dependencias de OpenMP 4.0+. Cada fila
 * se procesa como una tarea que depende de la fila anterior.
 * 
 * Características:
 * - Expresión natural de dependencias
 * - Runtime de OpenMP gestiona sincronización
 * - Overhead del sistema de tareas
 * - Requiere OpenMP 4.0+
 * - Útil para prototipado rápido
 * 
 * @param secA Secuencia A
 * @param secB Secuencia B
 * @param config Configuración de alineamiento
 * @return ResultadoAlineamiento con tiempos instrumentados
 */
ResultadoAlineamiento alineamientoNWP_OpenMP_Tasks(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
);

/**
 * @brief NW con recálculo de traceback usando tareas de OpenMP.
 * 
 * Versión optimizada en memoria con estrategia de tareas.
 * 
 * @param secA Secuencia A
 * @param secB Secuencia B
 * @param config Configuración de alineamiento
 * @return ResultadoAlineamiento con tiempos instrumentados
 */
ResultadoAlineamiento alineamientoNWR_OpenMP_Tasks(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
);

#endif // PARALELO_H

