#ifndef PUNTUACION_H
#define PUNTUACION_H

#include <string>

/**
 * @brief Esquema simple de puntuación para DNA: coincidencia, sustitución y gap.
 */
struct EsquemaPuntuacionDNA {
    int coincidencia;   /**< Puntuación por coincidencia (match). */
    int sustitucion;    /**< Puntuación por sustitución (mismatch). */
    int gap;            /**< Penalidad por gap. */
    
    EsquemaPuntuacionDNA(int coin = 2, int sust = -1, int g = -2) 
        : coincidencia(coin), sustitucion(sust), gap(g) {}
};

/**
 * @brief Configuración de puntuación para DNA.
 *
 * Usa esquema simple: coincidencia, sustitución y gap.
 */
struct ConfiguracionPuntuacionDNA {
    EsquemaPuntuacionDNA parametros;  /**< Parámetros de puntuación. */
    
    ConfiguracionPuntuacionDNA() 
        : parametros(2, -1, -2) {}  // match=2, mismatch=-1, gap=-2 por defecto
    
    ConfiguracionPuntuacionDNA(int coincidencia, int sustitucion, int gap)
        : parametros(coincidencia, sustitucion, gap) {}
};

/**
 * @brief Obtiene la puntuación entre dos bases DNA según la configuración.
 * @param a Primera base (A, T, G, C).
 * @param b Segunda base (A, T, G, C).
 * @param config Configuración de puntuación.
 * @return int Puntuación asignada (coincidencia o sustitución).
 */
int obtenerPuntuacionDNA(char a, char b, const ConfiguracionPuntuacionDNA& config);

/**
 * @brief Extrae la penalidad de gap de una configuración de puntuación.
 * @param config Configuración de puntuación.
 * @return int Penalidad de gap (usualmente negativa).
 */
int obtenerPenalidadGapDNA(const ConfiguracionPuntuacionDNA& config);

#endif // PUNTUACION_H

