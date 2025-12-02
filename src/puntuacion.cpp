#include "puntuacion.h"
#include <cctype>

int obtenerPuntuacionDNA(char a, char b, const ConfiguracionPuntuacionDNA& config) {
    a = std::toupper(a);
    b = std::toupper(b);
    
    if (a == b) {
        return config.parametros.coincidencia;
    }
    
    return config.parametros.sustitucion;
}

int obtenerPenalidadGapDNA(const ConfiguracionPuntuacionDNA& config) {
    return config.parametros.gap;
}

