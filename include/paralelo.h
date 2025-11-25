#ifndef PARALELO_H
#define PARALELO_H

#include <string>
#include "tipos.h"

/**
 * @file paralelo.h
 * @brief Interfaz pública para las implementaciones paralelas de Needleman–Wunsch.
 */

/**
 * @brief Ejecuta Needleman–Wunsch en paralelo usando estrategia de antidiagonales.
 *
 * Esta implementación procesa las antidiagonales de la matriz en orden, pero
 * paraleliza el procesamiento de los elementos dentro de cada antidiagonal.
 * Los elementos de una antidiagonal k (donde i+j=k) pueden procesarse en paralelo
 * ya que solo dependen de elementos de antidiagonales anteriores.
 *
 * @param secA Secuencia A (string) a alinear.
 * @param secB Secuencia B (string) a alinear.
 * @param config Configuración de alineamiento (puntuación, verbose, etc.).
 * @return ResultadoAlineamiento Contiene puntuación y tiempos instrumentados.
 */
ResultadoAlineamiento alineamientoNWParaleloAntidiagonal(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
);

/**
 * @brief Ejecuta Needleman–Wunsch en paralelo usando estrategia de bloques.
 *
 * Esta implementación divide la matriz en bloques y los procesa en paralelo.
 * Los bloques se procesan respetando las dependencias: un bloque solo puede
 * procesarse cuando sus bloques dependientes (arriba, izquierda, diagonal) están listos.
 *
 * @param secA Secuencia A (string) a alinear.
 * @param secB Secuencia B (string) a alinear.
 * @param config Configuración de alineamiento (puntuación, verbose, etc.).
 * @return ResultadoAlineamiento Contiene puntuación y tiempos instrumentados.
 */
ResultadoAlineamiento alineamientoNWParaleloBloques(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
);

/**
 * @brief Ejecuta Needleman–Wunsch en paralelo combinando antidiagonales y bloques.
 *
 * Esta implementación combina ambas estrategias: procesa antidiagonales en orden,
 * pero dentro de cada antidiagonal divide el trabajo en bloques que se procesan
 * en paralelo. Esto permite un mejor balanceo de carga y aprovechamiento de la
 * caché.
 *
 * @param secA Secuencia A (string) a alinear.
 * @param secB Secuencia B (string) a alinear.
 * @param config Configuración de alineamiento (puntuación, verbose, etc.).
 * @return ResultadoAlineamiento Contiene puntuación y tiempos instrumentados.
 */
ResultadoAlineamiento alineamientoNWParaleloCombinado(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
);

// ============================================================================
// ESTRATEGIA: ANTIDIAGONALES CON DIFERENTES SCHEDULES
// ============================================================================

ResultadoAlineamiento alineamientoNWParaleloAntidiagonalStatic(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
);

ResultadoAlineamiento alineamientoNWParaleloAntidiagonalDynamic(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
);

ResultadoAlineamiento alineamientoNWParaleloAntidiagonalGuided(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
);

ResultadoAlineamiento alineamientoNWParaleloAntidiagonalAuto(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
);

// ============================================================================
// ESTRATEGIA: BLOQUES CON DIFERENTES SCHEDULES
// ============================================================================

ResultadoAlineamiento alineamientoNWParaleloBloquesStatic(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
);

ResultadoAlineamiento alineamientoNWParaleloBloquesDynamic(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
);

ResultadoAlineamiento alineamientoNWParaleloBloquesGuided(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
);

ResultadoAlineamiento alineamientoNWParaleloBloquesAuto(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
);

// ============================================================================
// ESTRATEGIA: COMBINADO CON DIFERENTES SCHEDULES
// ============================================================================

ResultadoAlineamiento alineamientoNWParaleloCombinadoStatic(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
);

ResultadoAlineamiento alineamientoNWParaleloCombinadoDynamic(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
);

ResultadoAlineamiento alineamientoNWParaleloCombinadoGuided(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
);

ResultadoAlineamiento alineamientoNWParaleloCombinadoAuto(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
);

#endif // PARALELO_H

