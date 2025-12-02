#ifndef GENERADOR_SECUENCIAS_H
#define GENERADOR_SECUENCIAS_H

#include <string>
#include <vector>

/**
 * @brief Par de secuencias DNA generadas y metadatos asociados.
 */
struct ParSecuenciasDNA {
    std::string sec1;              /**< Primera secuencia. */
    std::string sec2;              /**< Segunda secuencia. */
    double similitud_real;         /**< Similitud real calculada (0.0 - 1.0). */
    
    ParSecuenciasDNA() : similitud_real(0.0) {}
    ParSecuenciasDNA(const std::string& s1, const std::string& s2, double sim)
        : sec1(s1), sec2(s2), similitud_real(sim) {}
};

/**
 * @brief Genera una secuencia DNA aleatoria de la longitud especificada.
 * @param longitud Longitud de la secuencia deseada.
 * @return std::string Secuencia DNA generada (bases A, T, G, C).
 */
std::string generarSecuenciaDNAAleatoria(int longitud);

/**
 * @brief Genera una secuencia DNA con similitud objetivo respecto a otra.
 *
 * Crea una variante de la secuencia "original" con una similitud aproximada
 * igual a "similitud_objetivo" (0.0 - 1.0).
 *
 * @param original Secuencia DNA original.
 * @param similitud_objetivo Similitud objetivo entre 0.0 y 1.0.
 * @return std::string Secuencia DNA similar generada.
 */
std::string generarSecuenciaDNASimilar(const std::string& original, 
                                      double similitud_objetivo);

/**
 * @brief Calcula la similitud (fracción de coincidencias) entre dos secuencias DNA.
 * @param sec1 Primera secuencia.
 * @param sec2 Segunda secuencia.
 * @return double Similitud entre 0.0 y 1.0.
 */
double calcularSimilitudDNA(const std::string& sec1, const std::string& sec2);

/**
 * @brief Genera un par de secuencias DNA con similitud controlada.
 * @param longitud Longitud deseada para cada secuencia.
 * @param similitud_objetivo Similitud objetivo (0.0 - 1.0).
 * @return ParSecuenciasDNA Par generado con la similitud real calculada.
 */
ParSecuenciasDNA generarParSecuenciasDNA(int longitud, 
                                         double similitud_objetivo);

/**
 * @brief Guarda un par de secuencias DNA en un archivo FASTA (dos registros).
 * @param par Par de secuencias a guardar.
 * @param nombreArchivo Ruta del archivo de salida.
 * @param id_sec1 Identificador para la primera secuencia (opcional).
 * @param id_sec2 Identificador para la segunda secuencia (opcional).
 * @return true si se escribió correctamente, false en caso de error.
 */
bool guardarParSecuenciasDNAFASTA(const ParSecuenciasDNA& par, 
                                  const std::string& nombreArchivo,
                                  const std::string& id_sec1 = "sec1",
                                  const std::string& id_sec2 = "sec2");

/**
 * @brief Carga el primer par de secuencias DNA encontrado en un archivo FASTA.
 * @param nombreArchivo Ruta del archivo FASTA de entrada.
 * @return ParSecuenciasDNA Par de secuencias cargado.
 */
ParSecuenciasDNA cargarParSecuenciasDNAFASTA(const std::string& nombreArchivo);

/**
 * @brief Genera múltiples pares de secuencias DNA en lote y los guarda en archivos FASTA.
 * @param directorio_base Directorio donde guardar los archivos.
 * @param longitudes Vector de longitudes a generar.
 * @param similitudes Vector de similitudes objetivo.
 * @return int Número de archivos generados exitosamente.
 */
int generarLoteSecuenciasDNA(const std::string& directorio_base,
                             const std::vector<int>& longitudes,
                             const std::vector<double>& similitudes);

#endif // GENERADOR_SECUENCIAS_H

