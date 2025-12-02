#ifndef UTILIDADES_H
#define UTILIDADES_H

#include <string>
#include <vector>
#include "tipos.h"

/**
 * @brief Imprime un alineamiento en formato legible por humanos.
 * @param secA Alineamiento para la secuencia A.
 * @param secB Alineamiento para la secuencia B.
 * @param puntuacion Puntuación asociada.
 */
void imprimirAlineamiento(const std::string& secA, const std::string& secB, int puntuacion);

/**
 * @brief Imprime todos los campos de un ResultadoAlineamiento de forma legible.
 * @param resultado Resultado de alineamiento a mostrar.
 */
void imprimirResultadoAlineamiento(const ResultadoAlineamiento& resultado);

/**
 * @brief Compara dos resultados de alineamiento y muestra diferencias.
 * @param resultado1 Primer resultado.
 * @param resultado2 Segundo resultado.
 * @param metodo1 Nombre del método que generó resultado1.
 * @param metodo2 Nombre del método que generó resultado2.
 */
void compararResultados(
    const ResultadoAlineamiento& resultado1, 
    const ResultadoAlineamiento& resultado2, 
    const std::string& metodo1, 
    const std::string& metodo2
);

/**
 * @brief Compara dos resultados de alineamiento de forma detallada, incluyendo verificación de matrices.
 * 
 * Compara puntuaciones, secuencias alineadas y opcionalmente las matrices de puntuación.
 * Para matrices grandes, solo compara la puntuación final (F[m][n]).
 * 
 * @param resultado1 Primer resultado.
 * @param resultado2 Segundo resultado.
 * @param metodo1 Nombre del método que generó resultado1.
 * @param metodo2 Nombre del método que generó resultado2.
 * @param secA Secuencia A original (necesaria para recalcular matriz si se compara).
 * @param secB Secuencia B original (necesaria para recalcular matriz si se compara).
 * @param config Configuración de alineamiento (necesaria para recalcular matriz si se compara).
 * @param comparar_matrices Si true, compara las matrices completas (solo para secuencias pequeñas).
 * @param umbral_tamano_matriz Tamaño máximo de matriz para comparación completa (default: 1000x1000).
 */
void compararResultadosDetallado(
    const ResultadoAlineamiento& resultado1, 
    const ResultadoAlineamiento& resultado2, 
    const std::string& metodo1, 
    const std::string& metodo2,
    const std::string& secA,
    const std::string& secB,
    const ConfiguracionAlineamiento& config,
    bool comparar_matrices = false,
    int umbral_tamano_matriz = 1000
);

/**
 * @brief Lee todas las secuencias encontradas en un archivo FASTA.
 * @param nombreArchivo Ruta del archivo FASTA.
 * @return std::vector<std::string> Vector con las secuencias leídas.
 */
std::vector<std::string> leerArchivoFasta(const std::string& nombreArchivo);

#endif // UTILIDADES_H

