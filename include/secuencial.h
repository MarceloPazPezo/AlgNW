#ifndef SECUENCIAL_H
#define SECUENCIAL_H

#include <string>
#include "tipos.h"

/**
 * @file secuencial.h
 * @brief Interfaz pública para las implementaciones secuenciales de Needleman–Wunsch.
 */

/**
 * @brief Ejecuta Needleman–Wunsch usando una matriz de traceback (punteros).
 *
 * Esta variante almacena la dirección de traceback en una matriz auxiliar y
 * realiza el traceback leyendo esa matriz. Es sencilla de entender y útil para
 * valores moderados de memoria.
 *
 * @param secA Secuencia A (string) a alinear.
 * @param secB Secuencia B (string) a alinear.
 * @param config Configuración de alineamiento (puntuación, verbose, etc.).
 * @return ResultadoAlineamiento Contiene puntuación y tiempos instrumentados si aplica.
 */
ResultadoAlineamiento alineamientoNWP(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
);

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
ResultadoAlineamiento alineamientoNWR(
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config
);

#endif // SECUENCIAL_H
