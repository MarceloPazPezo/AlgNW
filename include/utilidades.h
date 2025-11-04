#ifndef UTILIDADES_H
#define UTILIDADES_H

#include <string>
#include <functional>
#include <vector>
#include "tipos.h"

/**
 * @brief Resultado de un caso de benchmark.
 *
 * Almacena los metadatos del test y el tiempo/puntuación obtenidos.
 */
struct ResultadoBenchmark {
    std::string nombre_test;      /**< Nombre del test o caso. */
    std::string metodo;           /**< Método utilizado (p.ej. "recalculado"). */
    int longitud_sec_a;           /**< Longitud de la secuencia A. */
    int longitud_sec_b;           /**< Longitud de la secuencia B. */
    bool es_proteina;             /**< True si se usó puntuación de proteínas. */
    int penalidad_gap;            /**< Valor de penalidad de gap usado. */
    double tiempo_init_ms;        /**< Tiempo (ms) de inicialización. */
    double tiempo_llenado_ms;     /**< Tiempo (ms) del llenado DP. */
    double tiempo_trace_ms;       /**< Tiempo (ms) del traceback. */
    double tiempo_total_ms;       /**< Tiempo total del caso en milisegundos. */
    int puntuacion;               /**< Puntuación final del alineamiento. */

    ResultadoBenchmark(const std::string& nombre, const std::string& met,
                      int lenA, int lenB, bool proteina, int gap,
                      double init_t, double llenado_t, double trace_t, int puntua)
        : nombre_test(nombre), metodo(met), longitud_sec_a(lenA), longitud_sec_b(lenB),
          es_proteina(proteina), penalidad_gap(gap), tiempo_init_ms(init_t), 
          tiempo_llenado_ms(llenado_t), tiempo_trace_ms(trace_t),
          tiempo_total_ms(init_t + llenado_t + trace_t), puntuacion(puntua) {}
};

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
 * @brief Mide el tiempo que tarda en ejecutarse una función.
 * @param func Función a ejecutar y medir.
 * @return double Tiempo transcurrido en milisegundos.
 */
double medirTiempo(std::function<void()> func);

/**
 * @brief Lee todas las secuencias encontradas en un archivo FASTA.
 * @param nombreArchivo Ruta del archivo FASTA.
 * @return std::vector<std::string> Vector con las secuencias leídas.
 */
std::vector<std::string> leerArchivoFasta(const std::string& nombreArchivo);

/**
 * @brief Exporta un conjunto de resultados de benchmark a CSV.
 * @param resultados Vector de ResultadoBenchmark.
 * @param nombreArchivo Archivo de salida CSV.
 * @return true si la escritura fue exitosa.
 */
bool exportarBenchmarkACSV(const std::vector<ResultadoBenchmark>& resultados, 
                          const std::string& nombreArchivo);

/**
 * @brief Añade una fila al CSV de benchmarking en modo append.
 * @param resultado Resultado a añadir.
 * @param nombreArchivo Archivo CSV destino.
 * @return true si la operación fue exitosa.
 */
bool anexarBenchmarkACSV(const ResultadoBenchmark& resultado, 
                        const std::string& nombreArchivo);

// ========== FUNCIONES DE GESTIÓN DE MEMORIA ==========

// Constantes para límites de memoria
const size_t LONGITUD_MAXIMA_SECUENCIA = 30000;  // 30K caracteres por secuencia
const size_t LONGITUD_ADVERTENCIA_SECUENCIA = 10000; // Advertencia a partir de 10K

// Estructura para información de memoria
struct InfoMemoria {
    size_t tamaño_matriz_mb;           // Tamaño de matriz de puntuaciones en MB
    size_t tamaño_traceback_mb;        // Tamaño de matriz de traceback en MB (0 si recalculado)
    size_t memoria_total_mb;           // Memoria total estimada en MB
    size_t longitud_sec_a;
    size_t longitud_sec_b;
    bool usa_matriz_traceback;         // true si usa matriz de direcciones
};

/**
 * @brief Calcula la memoria estimada para el alineamiento.
 * @param lenA Longitud de la secuencia A.
 * @param lenB Longitud de la secuencia B.
 * @param conPunteros True si usa matriz de traceback.
 * @return InfoMemoria Información detallada de memoria.
 */
InfoMemoria calcularUsoMemoria(size_t lenA, size_t lenB, bool conPunteros);

/**
 * @brief Verifica si las secuencias son demasiado grandes y muestra advertencias.
 * @param lenA Longitud de la secuencia A.
 * @param lenB Longitud de la secuencia B.
 * @param mostrarAdvertencias Si true, imprime advertencias en stderr.
 * @return true si es seguro continuar, false si excede límites.
 */
bool verificarLimitesSecuencia(size_t lenA, size_t lenB, bool mostrarAdvertencias = true);

/**
 * @brief Imprime información detallada sobre el uso de memoria.
 * @param info Información de memoria a imprimir.
 */
void imprimirInfoMemoria(const InfoMemoria& info);

/**
 * @brief Formatea un tamaño en bytes a una cadena legible (KB, MB, GB).
 * @param bytes Tamaño en bytes.
 * @return std::string Cadena formateada.
 */
std::string formatearBytes(size_t bytes);

#endif // UTILIDADES_H
