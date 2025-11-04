#ifndef GENERADOR_SECUENCIAS_H
#define GENERADOR_SECUENCIAS_H

#include <string>
#include <vector>

/**
 * @brief Tipos de secuencias biológicas soportadas por el generador.
 */
enum class TipoSecuencia {
    DNA,      /**< Bases A, T, G, C. */
    RNA,      /**< Bases A, U, G, C. */
    PROTEINA  /**< 20 aminoácidos estándar. */
};

/**
 * @brief Par de secuencias generadas y metadatos asociados.
 *
 * Guarda las dos cadenas, la similitud real calculada entre ellas y el tipo
 * de secuencia (DNA/RNA/PROTEINA).
 */
struct ParSecuencias {
    std::string sec1;              /**< Primera secuencia. */
    std::string sec2;              /**< Segunda secuencia. */
    double similitud_real;         /**< Similitud real calculada (0.0 - 1.0). */
    TipoSecuencia tipo;            /**< Tipo de secuencia. */
    
    ParSecuencias() : similitud_real(0.0), tipo(TipoSecuencia::DNA) {}
    ParSecuencias(const std::string& s1, const std::string& s2, double sim, TipoSecuencia t)
        : sec1(s1), sec2(s2), similitud_real(sim), tipo(t) {}
};

/**
 * @brief Genera una secuencia aleatoria del tipo y longitud especificados.
 * @param longitud Longitud de la secuencia deseada.
 * @param tipo Tipo de secuencia (DNA/RNA/PROTEINA).
 * @return std::string Secuencia generada.
 */
std::string generarSecuenciaAleatoria(int longitud, TipoSecuencia tipo);

/**
 * @brief Genera una secuencia con similitud objetivo respecto a otra.
 *
 * Crea una variante de la secuencia "original" con una similitud aproximada
 * igual a "similitud_objetivo" (0.0 - 1.0).
 *
 * @param original Secuencia original.
 * @param similitud_objetivo Similitud objetivo entre 0.0 y 1.0.
 * @param tipo Tipo de secuencia.
 * @return std::string Secuencia similar generada.
 */
std::string generarSecuenciaSimilar(const std::string& original, 
                                   double similitud_objetivo,
                                   TipoSecuencia tipo);

/**
 * @brief Calcula la similitud (fracción de coincidencias) entre dos secuencias.
 * @param sec1 Primera secuencia.
 * @param sec2 Segunda secuencia.
 * @return double Similitud entre 0.0 y 1.0.
 */
double calcularSimilitud(const std::string& sec1, const std::string& sec2);

/**
 * @brief Genera un par de secuencias con similitud controlada.
 * @param longitud Longitud deseada para cada secuencia.
 * @param similitud_objetivo Similitud objetivo (0.0 - 1.0).
 * @param tipo Tipo de secuencia.
 * @return ParSecuencias Par generado con la similitud real calculada.
 */
ParSecuencias generarParSecuencias(int longitud, 
                                  double similitud_objetivo,
                                  TipoSecuencia tipo);

/**
 * @brief Guarda un par de secuencias en un único archivo FASTA (dos registros).
 * @param par Par de secuencias a guardar.
 * @param nombreArchivo Ruta del archivo de salida.
 * @param id_sec1 Identificador para la primera secuencia (opcional).
 * @param id_sec2 Identificador para la segunda secuencia (opcional).
 * @return true si se escribió correctamente, false en caso de error.
 */
bool guardarParSecuenciasFASTA(const ParSecuencias& par, 
                              const std::string& nombreArchivo,
                              const std::string& id_sec1 = "sec1",
                              const std::string& id_sec2 = "sec2");

/**
 * @brief Carga el primer par de secuencias encontrado en un archivo FASTA.
 *
 * Busca hasta dos registros FASTA y los devuelve como ParSecuencias. Si el
 * archivo contiene menos de dos secuencias, las cadenas vacías estarán
 * presentes en el resultado.
 *
 * @param nombreArchivo Ruta del archivo FASTA de entrada.
 * @return ParSecuencias Par de secuencias cargado.
 */
ParSecuencias cargarParSecuenciasFASTA(const std::string& nombreArchivo);

/**
 * @brief Genera múltiples pares de secuencias en lote y los guarda en archivos FASTA.
 * @param directorio_base Directorio donde guardar los archivos.
 * @param longitudes Vector de longitudes a generar.
 * @param similitudes Vector de similitudes objetivo.
 * @param tipo Tipo de secuencia.
 * @return int Número de archivos generados exitosamente.
 */
int generarLoteSecuencias(const std::string& directorio_base,
                         const std::vector<int>& longitudes,
                         const std::vector<double>& similitudes,
                         TipoSecuencia tipo);

#endif // GENERADOR_SECUENCIAS_H
