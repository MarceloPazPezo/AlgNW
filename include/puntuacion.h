#ifndef PUNTUACION_H
#define PUNTUACION_H

#include <map>
#include <utility>
#include <string>

/**
 * @brief Tipos de matrices de puntuación soportadas.
 *
 * Incluye matrices predefinidas para proteínas (BLOSUM/PAM), esquemas simples
 * para ácidos nucleicos y opciones para configuraciones personalizadas.
 */
enum class TipoMatrizPuntuacion {
    BLOSUM62,          /**< Matriz por defecto para proteínas. */
    BLOSUM50,
    BLOSUM80,
    PAM250,
    PAM120,
    PAM30,
    DNA_SIMPLE,        /**< Esquema simple para DNA (coincidencia/sustitución/gap). */
    RNA_SIMPLE,        /**< Esquema simple para RNA. */
    SIMPLE_PERSONALIZADO,  /**< Esquema simple personalizado. */
    MATRIZ_PERSONALIZADA   /**< Matriz completa personalizada (mapa de pares). */
};

/**
 * @brief Esquema simple de puntuación: coincidencia, sustitución y gap.
 */
struct EsquemaPuntuacionSimple {
    int coincidencia;   /**< Puntuación por coincidencia (match). */
    int sustitucion;    /**< Puntuación por sustitución (mismatch). */
    int gap;            /**< Penalidad por gap. */
    
    EsquemaPuntuacionSimple(int coin = 2, int sust = -1, int g = -2) 
        : coincidencia(coin), sustitucion(sust), gap(g) {}
};

/**
 * @brief Configuración unificada de puntuación.
 *
 * Dependiendo de `tipo`, se usan `parametrosSimples` (SIMPLE_PERSONALIZADO) o
 * `matrizPersonalizada` (MATRIZ_PERSONALIZADA). Para matrices predefinidas se 
 * usan valores internos conocidos por el código.
 */
struct ConfiguracionPuntuacion {
    TipoMatrizPuntuacion tipo;                              /**< Tipo de matriz. */
    EsquemaPuntuacionSimple parametrosSimples;              /**< Parámetros si SIMPLE_PERSONALIZADO. */
    std::map<std::pair<char, char>, int>* matrizPersonalizada; /**< Puntero a matriz completa si MATRIZ_PERSONALIZADA. */
    
    ConfiguracionPuntuacion() 
        : tipo(TipoMatrizPuntuacion::DNA_SIMPLE), 
          matrizPersonalizada(nullptr) {}
    
    explicit ConfiguracionPuntuacion(TipoMatrizPuntuacion t) 
        : tipo(t), matrizPersonalizada(nullptr) {}
    
    ConfiguracionPuntuacion(int coincidencia, int sustitucion, int gap)
        : tipo(TipoMatrizPuntuacion::SIMPLE_PERSONALIZADO),
          parametrosSimples(coincidencia, sustitucion, gap),
          matrizPersonalizada(nullptr) {}
    
    explicit ConfiguracionPuntuacion(std::map<std::pair<char, char>, int>* matriz)
        : tipo(TipoMatrizPuntuacion::MATRIZ_PERSONALIZADA),
          matrizPersonalizada(matriz) {}
};

/**
 * @brief Matrices predefinidas expuestas por la librería.
 *
 * Estas variables extern proporcionan acceso a las tablas de puntuación
 * usadas por `obtenerPuntuacionConConfig` cuando se seleccionan matrices
 * predefinidas.
 */
extern const std::map<std::pair<char, char>, int> BLOSUM62;
extern const std::map<std::pair<char, char>, int> DNA_MATRIX;
extern const std::map<std::pair<char, char>, int> BLOSUM50;
extern const std::map<std::pair<char, char>, int> BLOSUM80;
extern const std::map<std::pair<char, char>, int> PAM250;

/**
 * @brief Obtiene la puntuación entre dos símbolos según la configuración.
 * @param a Primer carácter (base o aminoácido).
 * @param b Segundo carácter (base o aminoácido).
 * @param config Configuración de puntuación que decide la matriz/esquema.
 * @return int Puntuación asignada (mayor para coincidencias, menor para gaps/sustituciones).
 */
int obtenerPuntuacionConConfig(char a, char b, const ConfiguracionPuntuacion& config);

/**
 * @brief Obtiene un nombre legible para un tipo de matriz.
 * @param tipo Tipo de matriz.
 * @return std::string Nombre legible (por ejemplo "BLOSUM62").
 */
std::string obtenerNombreMatriz(TipoMatrizPuntuacion tipo);

/**
 * @brief Extrae la penalidad de gap de una configuración de puntuación.
 * @param config Configuración de puntuación.
 * @return int Penalidad de gap (usualmente negativa).
 */
int obtenerPenalidadGap(const ConfiguracionPuntuacion& config);

#endif // PUNTUACION_H
