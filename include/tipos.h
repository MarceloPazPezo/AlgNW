#ifndef TIPOS_H
#define TIPOS_H

#include <string>
#include "puntuacion.h"

/**
 * @brief Resultado de un alineamiento global de DNA.
 *
 * Contiene las secuencias alineadas, la puntuación final y tiempos medidos
 * para las fases internas (llenado de la matriz DP y traceback).
 */
struct ResultadoAlineamiento {
    std::string secA;              /**< Secuencia A (alineada). */
    std::string secB;              /**< Secuencia B (alineada). */
    int puntuacion;                /**< Puntuación final del alineamiento. */
    double tiempo_fase1_ms;       /**< Tiempo (ms) empleado en la inicialización. */
    double tiempo_fase2_ms;       /**< Tiempo (ms) empleado en el llenado DP. */
    double tiempo_fase3_ms;       /**< Tiempo (ms) empleado en el traceback. */

    ResultadoAlineamiento()
        : secA(""), secB(""), puntuacion(0),
          tiempo_fase1_ms(0.0), tiempo_fase2_ms(0.0), tiempo_fase3_ms(0.0) {}

    ResultadoAlineamiento(const std::string& a, const std::string& b, int puntua,
                         double llenado_ms = 0.0, double traceback_ms = 0.0, double inicializacion_ms = 0.0)
        : secA(a), secB(b), puntuacion(puntua), 
          tiempo_fase1_ms(inicializacion_ms), tiempo_fase2_ms(llenado_ms), tiempo_fase3_ms(traceback_ms) {}
};

/**
 * @brief Configuración para ejecutar un alineamiento de DNA.
 *
 * Agrupa la configuración de puntuación y opciones de ejecución.
 */
struct ConfiguracionAlineamiento {
    ConfiguracionPuntuacionDNA puntuacion;  /**< Configuración de puntuación DNA. */
    bool verbose;                            /**< Habilita salida verbosa si es true. */
    
    /**
     * @brief Constructor por defecto.
     * Usa match=2, mismatch=-1, gap=-2 por defecto.
     */
    ConfiguracionAlineamiento() 
        : puntuacion(2, -1, -2), verbose(false) {}
    
    /**
     * @brief Constructor con esquema simple (coincidencia/sustitución/gap).
     * @param coincidencia Puntuación por coincidencia.
     * @param sustitucion Puntuación por sustitución.
     * @param penalidad_gap Penalidad por gap.
     * @param verboso Flag verbose.
     */
    ConfiguracionAlineamiento(int coincidencia, int sustitucion, int penalidad_gap, bool verboso = false)
        : puntuacion(coincidencia, sustitucion, penalidad_gap), verbose(verboso) {}
    
    /**
     * @brief Constructor con un objeto ConfiguracionPuntuacionDNA completo.
     * @param config_punt Configuración de puntuación ya construida.
     * @param verboso Flag verbose.
     */
    ConfiguracionAlineamiento(const ConfiguracionPuntuacionDNA& config_punt, bool verboso = false)
        : puntuacion(config_punt), verbose(verboso) {}
};

#endif // TIPOS_H

