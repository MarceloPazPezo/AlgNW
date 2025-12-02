#ifndef SECUENCIAL_H
#define SECUENCIAL_H

#include <string>
#include "tipos.h"

/**
 * @file secuencial.h
 * @brief Interfaz para la implementación secuencial de Needleman–Wunsch con recálculo de traceback.
 */

/**
 * @brief Ejecuta Needleman–Wunsch con recálculo del traceback (optimizado en memoria).
 *
 * En lugar de almacenar una matriz de direcciones, esta versión recalcula
 * las decisiones de traceback consultando la matriz de puntuaciones. Reduce memoria
 * a costa de algún coste adicional de CPU durante el traceback.
 *
 * @param secA Secuencia A (string) a alinear.
 * @param secB Secuencia B (string) a alinear.
 * @param config Configuración de alineamiento (puntuación, verbose, etc.).
 * @return ResultadoAlineamiento Contiene puntuación y tiempos instrumentados si aplica.
 */
ResultadoAlineamiento AlgNW(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
);

#endif // SECUENCIAL_H

