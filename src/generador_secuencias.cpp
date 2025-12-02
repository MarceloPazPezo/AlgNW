#include "generador_secuencias.h"
#include <random>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <thread>
#include <functional>
#include <cstring>
#include <cerrno>

#ifdef _WIN32
    #include <direct.h>
    #define mkdir _mkdir
#else
    #include <sys/stat.h>
    #include <sys/types.h>
#endif

const std::string ALFABETO_DNA = "ATGC";

static std::mt19937& obtenerGeneradorAleatorio() {
    static thread_local std::mt19937 gen(
        std::random_device{}() ^ 
        (std::hash<std::thread::id>{}(std::this_thread::get_id()) + std::time(nullptr))
    );
    return gen;
}

std::string generarSecuenciaDNAAleatoria(int longitud) {
    std::string secuencia;
    secuencia.reserve(longitud);
    
    std::uniform_int_distribution<int> dist(0, ALFABETO_DNA.length() - 1);
    auto& gen = obtenerGeneradorAleatorio();
    
    for (int i = 0; i < longitud; ++i) {
        secuencia += ALFABETO_DNA[dist(gen)];
    }
    
    return secuencia;
}

std::string generarSecuenciaDNASimilar(const std::string& original, 
                                      double similitud_objetivo) {
    if (similitud_objetivo < 0.0 || similitud_objetivo > 1.0) {
        similitud_objetivo = std::max(0.0, std::min(1.0, similitud_objetivo));
    }
    
    std::string similar = original;
    int longitud = original.length();
    
    // Calcular cuántas posiciones deben ser diferentes
    int num_diferencias = static_cast<int>((1.0 - similitud_objetivo) * longitud);
    
    // Crear vector de índices y mezclarlos
    std::vector<int> indices(longitud);
    for (int i = 0; i < longitud; ++i) {
        indices[i] = i;
    }
    
    auto& gen = obtenerGeneradorAleatorio();
    std::shuffle(indices.begin(), indices.end(), gen);
    
    // Cambiar las primeras num_diferencias posiciones
    std::uniform_int_distribution<int> dist(0, ALFABETO_DNA.length() - 1);
    
    for (int i = 0; i < num_diferencias; ++i) {
        int pos = indices[i];
        char caracter_original = similar[pos];
        char nuevo_caracter;
        
        // Asegurar que el nuevo carácter sea diferente
        do {
            nuevo_caracter = ALFABETO_DNA[dist(gen)];
        } while (nuevo_caracter == caracter_original && ALFABETO_DNA.length() > 1);
        
        similar[pos] = nuevo_caracter;
    }
    
    return similar;
}

double calcularSimilitudDNA(const std::string& sec1, const std::string& sec2) {
    if (sec1.length() != sec2.length() || sec1.empty()) {
        return 0.0;
    }
    
    int coincidencias = 0;
    for (size_t i = 0; i < sec1.length(); ++i) {
        if (sec1[i] == sec2[i]) {
            coincidencias++;
        }
    }
    
    return static_cast<double>(coincidencias) / sec1.length();
}

ParSecuenciasDNA generarParSecuenciasDNA(int longitud, 
                                         double similitud_objetivo) {
    ParSecuenciasDNA par;
    
    // Generar primera secuencia aleatoria
    par.sec1 = generarSecuenciaDNAAleatoria(longitud);
    
    // Generar segunda secuencia similar
    par.sec2 = generarSecuenciaDNASimilar(par.sec1, similitud_objetivo);
    
    // Calcular similitud real
    par.similitud_real = calcularSimilitudDNA(par.sec1, par.sec2);
    
    return par;
}

// Crear directorio si no existe
static bool crearDirectorio(const std::string& ruta) {
    #ifdef _WIN32
        return mkdir(ruta.c_str()) == 0 || errno == EEXIST;
    #else
        return mkdir(ruta.c_str(), 0755) == 0 || errno == EEXIST;
    #endif
}

// Formatea una secuencia en bloques para FASTA
static std::string formatearSecuenciaFASTA(const std::string& sec, int ancho_linea = 60) {
    std::string formateada;
    for (size_t i = 0; i < sec.length(); i += ancho_linea) {
        formateada += sec.substr(i, ancho_linea) + "\n";
    }
    return formateada;
}

bool guardarParSecuenciasDNAFASTA(const ParSecuenciasDNA& par, 
                                  const std::string& nombreArchivo,
                                  const std::string& id_sec1,
                                  const std::string& id_sec2) {
    std::ofstream archivo(nombreArchivo);
    
    if (!archivo.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo " << nombreArchivo << "\n";
        return false;
    }
    
    // Primera secuencia
    archivo << ">" << id_sec1 << " longitud=" << par.sec1.length() 
            << " tipo=DNA\n";
    archivo << formatearSecuenciaFASTA(par.sec1);
    
    // Segunda secuencia
    archivo << ">" << id_sec2 << " longitud=" << par.sec2.length() 
            << " tipo=DNA similitud=" << std::fixed << std::setprecision(4) 
            << par.similitud_real << "\n";
    archivo << formatearSecuenciaFASTA(par.sec2);
    
    archivo.close();
    return true;
}

ParSecuenciasDNA cargarParSecuenciasDNAFASTA(const std::string& nombreArchivo) {
    std::ifstream archivo(nombreArchivo);
    ParSecuenciasDNA par;
    
    if (!archivo.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo " << nombreArchivo << "\n";
        return par;
    }
    
    std::string linea;
    std::string secuencia_actual;
    int contador_sec = 0;
    
    while (std::getline(archivo, linea)) {
        if (linea.empty()) continue;
        
        if (linea[0] == '>') {
            // Cabecera de secuencia
            if (contador_sec == 1) {
                par.sec1 = secuencia_actual;
                secuencia_actual.clear();
            }
            
            // Parsear información de la cabecera
            if (linea.find("similitud=") != std::string::npos) {
                size_t pos = linea.find("similitud=") + 10;
                par.similitud_real = std::stod(linea.substr(pos));
            }
            
            contador_sec++;
        } else {
            // Línea de secuencia
            secuencia_actual += linea;
        }
    }
    
    if (contador_sec == 2) {
        par.sec2 = secuencia_actual;
    }
    
    archivo.close();
    return par;
}

int generarLoteSecuenciasDNA(const std::string& directorio_base,
                             const std::vector<int>& longitudes,
                             const std::vector<double>& similitudes) {
    // Crear directorio base si no existe
    if (!crearDirectorio(directorio_base)) {
        std::cerr << "Error: No se pudo crear el directorio " << directorio_base << "\n";
        return 0;
    }
    
    int archivos_generados = 0;
    
    // Generar pares para cada combinación de longitud y similitud
    for (int longitud : longitudes) {
        for (double similitud : similitudes) {
            // Generar par
            ParSecuenciasDNA par = generarParSecuenciasDNA(longitud, similitud);
            
            // Crear nombre de archivo
            std::ostringstream nombre_archivo;
            nombre_archivo << directorio_base << "/"
                          << "dna_lon" << longitud
                          << "_sim" << static_cast<int>(similitud * 100) << ".fasta";
            
            // Guardar
            if (guardarParSecuenciasDNAFASTA(par, nombre_archivo.str())) {
                archivos_generados++;
                std::cout << "Generado: " << nombre_archivo.str() 
                         << " (similitud real: " << std::fixed << std::setprecision(4) 
                         << par.similitud_real << ")\n";
            } else {
                std::cerr << "Error al generar: " << nombre_archivo.str() << "\n";
            }
        }
    }
    
    return archivos_generados;
}

