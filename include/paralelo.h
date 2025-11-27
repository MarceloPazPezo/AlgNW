#ifndef PARALELO_H
#define PARALELO_H

#include <string>
#include "tipos.h"

/**
 * @file paralelo.h
 * @brief Interfaz pública para las implementaciones paralelas de Needleman–Wunsch.
 * 
 * NOTA: El schedule de OpenMP se configura mediante la variable de entorno OMP_SCHEDULE.
 * Ejemplos:
 *   export OMP_SCHEDULE="static"          # static sin chunk específico
 *   export OMP_SCHEDULE="static,1"        # static con chunk size 1
 *   export OMP_SCHEDULE="static,10"      # static con chunk size 10
 *   export OMP_SCHEDULE="dynamic,1"      # dynamic con chunk size 1
 *   export OMP_SCHEDULE="dynamic,10"      # dynamic con chunk size 10
 *   export OMP_SCHEDULE="guided,1"       # guided con chunk size mínimo 1
 *   export OMP_SCHEDULE="auto"            # auto (OpenMP decide)
 * 
 * El formato es: "tipo" o "tipo,chunk_size"
 * 
 * Solo la fase 2 (llenado de matriz) está paralelizada.
 */

/**
 * @brief Ejecuta Needleman–Wunsch en paralelo usando estrategia de antidiagonales.
 *
 * Esta implementación procesa las antidiagonales de la matriz en orden, pero
 * paraleliza el procesamiento de los elementos dentro de cada antidiagonal.
 * Los elementos de una antidiagonal k (donde i+j=k) pueden procesarse en paralelo
 * ya que solo dependen de elementos de antidiagonales anteriores.
 * 
 * Solo paraleliza la fase 2 (llenado de matriz). La fase 1 (inicialización)
 * y fase 3 (traceback) se ejecutan secuencialmente.
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
 * Solo paraleliza la fase 2 (llenado de matriz). La fase 1 (inicialización)
 * y fase 3 (traceback) se ejecutan secuencialmente.
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

#endif // PARALELO_H
