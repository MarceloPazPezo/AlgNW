#ifndef TIPOS_H
#define TIPOS_H

#include <string>
#include "puntuacion.h"

/**
 * @brief Direcciones usadas en el traceback del alineamiento.
 *
 * DIAGONAL: corresponde a un match o mismatch entre caracteres.
 * ARRIBA: indica una inserción de un gap en la secuencia B (deleción en A).
 * IZQUIERDA: indica una inserción de un gap en la secuencia A (inserción en B).
 */
enum class Direccion {
    DIAGONAL,    // coincidencia/sustitución
    ARRIBA,      // gap en B (eliminación)
    IZQUIERDA    // gap en A (inserción)
};

/**
 * @brief Resultado de un alineamiento global.
 *
 * Contiene las secuencias alineadas (o sus representaciones resultantes),
 * la puntuación final y tiempos medidos para las fases internas si están
 * instrumentadas (llenado de la matriz DP y traceback).
 */
struct ResultadoAlineamiento {
    std::string secA;              /**< Secuencia A (alineada o resultado). */
    std::string secB;              /**< Secuencia B (alineada o resultado). */
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
 * @brief Configuración para ejecutar un alineamiento.
 *
 * Agrupa la configuración de puntuación y opciones de ejecución (por ejemplo
 * verbose). Proporciona múltiples constructores de conveniencia para crear
 * configuraciones rápidas (matrices predefinidas, esquema simple, etc.).
 */
struct ConfiguracionAlineamiento {
    ConfiguracionPuntuacion puntuacion;  /**< Configuración de puntuación a utilizar. */
    bool verbose;                        /**< Habilita salida verbosa si es true. */
    
    /**
     * @brief Constructor por defecto.
     * Usa el esquema DNA simple por defecto y penalidad de gap = -2.
     */
    ConfiguracionAlineamiento() 
        : puntuacion(), verbose(false) {}
    
    /**
     * @brief Constructor basado en una matriz predefinida.
     * @param tipoMatriz Matriz de sustitución a usar (p.ej. BLOSUM62).
     * @param penalidad_gap Penalidad para gaps (valor negativo usualmente).
     * @param verboso Activa modo verbose si es true.
     */
    ConfiguracionAlineamiento(TipoMatrizPuntuacion tipoMatriz, int penalidad_gap = -2, bool verboso = false)
        : puntuacion(tipoMatriz), verbose(verboso) {
        puntuacion.parametrosSimples.gap = penalidad_gap;
    }
    
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
     * @brief Constructor con un objeto ConfiguracionPuntuacion completo.
     * @param config_punt Configuración de puntuación ya construida.
     * @param verboso Flag verbose.
     */
    ConfiguracionAlineamiento(const ConfiguracionPuntuacion& config_punt, bool verboso = false)
        : puntuacion(config_punt), verbose(verboso) {}
    
    /**
     * @brief Constructor legacy para compatibilidad con versiones previas.
     * @param penalidad_gap Penalidad por gap.
     * @param esProteina True si se desea una matriz de proteínas por defecto.
     * @param verboso Flag verbose.
     */
    ConfiguracionAlineamiento(int penalidad_gap, bool esProteina, bool verboso = false)
        : verbose(verboso) {
        if (esProteina) {
            puntuacion = ConfiguracionPuntuacion(TipoMatrizPuntuacion::BLOSUM62);
        } else {
            puntuacion = ConfiguracionPuntuacion(TipoMatrizPuntuacion::DNA_SIMPLE);
        }
        puntuacion.parametrosSimples.gap = penalidad_gap;
    }
};

#endif // TIPOS_H
