#include "puntuacion.h"
#include <cctype>

// Matrices de puntuación predefinidas
const std::map<std::pair<char, char>, int> BLOSUM62 = {
    {{'A', 'A'}, 4}, {{'A', 'R'}, -1}, {{'A', 'N'}, -2}, {{'A', 'D'}, -2}, {{'A', 'C'}, 0},
    {{'A', 'Q'}, -1}, {{'A', 'E'}, -1}, {{'A', 'G'}, 0}, {{'A', 'H'}, -2}, {{'A', 'I'}, -1},
    {{'A', 'L'}, -1}, {{'A', 'K'}, -1}, {{'A', 'M'}, -1}, {{'A', 'F'}, -2}, {{'A', 'P'}, -1},
    {{'A', 'S'}, 1}, {{'A', 'T'}, 0}, {{'A', 'W'}, -3}, {{'A', 'Y'}, -2}, {{'A', 'V'}, 0},
    {{'R', 'R'}, 5}, {{'R', 'N'}, 0}, {{'R', 'D'}, -2}, {{'R', 'C'}, -3}, {{'R', 'Q'}, 1},
    {{'R', 'E'}, 0}, {{'R', 'G'}, -2}, {{'R', 'H'}, 0}, {{'R', 'I'}, -3}, {{'R', 'L'}, -2},
    {{'R', 'K'}, 2}, {{'R', 'M'}, -1}, {{'R', 'F'}, -3}, {{'R', 'P'}, -2}, {{'R', 'S'}, -1},
    {{'R', 'T'}, -1}, {{'R', 'W'}, -3}, {{'R', 'Y'}, -2}, {{'R', 'V'}, -3},
    {{'N', 'N'}, 6}, {{'N', 'D'}, 1}, {{'N', 'C'}, -3}, {{'N', 'Q'}, 0}, {{'N', 'E'}, 0},
    {{'N', 'G'}, 0}, {{'N', 'H'}, 1}, {{'N', 'I'}, -3}, {{'N', 'L'}, -3}, {{'N', 'K'}, 0},
    {{'N', 'M'}, -2}, {{'N', 'F'}, -3}, {{'N', 'P'}, -2}, {{'N', 'S'}, 1}, {{'N', 'T'}, 0},
    {{'N', 'W'}, -4}, {{'N', 'Y'}, -2}, {{'N', 'V'}, -3},
    {{'D', 'D'}, 6}, {{'D', 'C'}, -3}, {{'D', 'Q'}, 0}, {{'D', 'E'}, 2}, {{'D', 'G'}, -1},
    {{'D', 'H'}, -1}, {{'D', 'I'}, -3}, {{'D', 'L'}, -4}, {{'D', 'K'}, -1}, {{'D', 'M'}, -3},
    {{'D', 'F'}, -3}, {{'D', 'P'}, -1}, {{'D', 'S'}, 0}, {{'D', 'T'}, -1}, {{'D', 'W'}, -4},
    {{'D', 'Y'}, -3}, {{'D', 'V'}, -3},
    {{'C', 'C'}, 9}, {{'C', 'Q'}, -3}, {{'C', 'E'}, -4}, {{'C', 'G'}, -3}, {{'C', 'H'}, -3},
    {{'C', 'I'}, -1}, {{'C', 'L'}, -1}, {{'C', 'K'}, -3}, {{'C', 'M'}, -1}, {{'C', 'F'}, -2},
    {{'C', 'P'}, -3}, {{'C', 'S'}, -1}, {{'C', 'T'}, -1}, {{'C', 'W'}, -2}, {{'C', 'Y'}, -2},
    {{'C', 'V'}, -1},
    {{'Q', 'Q'}, 5}, {{'Q', 'E'}, 2}, {{'Q', 'G'}, -2}, {{'Q', 'H'}, 0}, {{'Q', 'I'}, -3},
    {{'Q', 'L'}, -2}, {{'Q', 'K'}, 1}, {{'Q', 'M'}, 0}, {{'Q', 'F'}, -3}, {{'Q', 'P'}, -1},
    {{'Q', 'S'}, 0}, {{'Q', 'T'}, -1}, {{'Q', 'W'}, -2}, {{'Q', 'Y'}, -1}, {{'Q', 'V'}, -2},
    {{'E', 'E'}, 5}, {{'E', 'G'}, -2}, {{'E', 'H'}, 0}, {{'E', 'I'}, -3}, {{'E', 'L'}, -3},
    {{'E', 'K'}, 1}, {{'E', 'M'}, -2}, {{'E', 'F'}, -3}, {{'E', 'P'}, -1}, {{'E', 'S'}, 0},
    {{'E', 'T'}, -1}, {{'E', 'W'}, -3}, {{'E', 'Y'}, -2}, {{'E', 'V'}, -2},
    {{'G', 'G'}, 6}, {{'G', 'H'}, -2}, {{'G', 'I'}, -4}, {{'G', 'L'}, -4}, {{'G', 'K'}, -2},
    {{'G', 'M'}, -3}, {{'G', 'F'}, -3}, {{'G', 'P'}, -2}, {{'G', 'S'}, 0}, {{'G', 'T'}, -2},
    {{'G', 'W'}, -2}, {{'G', 'Y'}, -3}, {{'G', 'V'}, -3},
    {{'H', 'H'}, 8}, {{'H', 'I'}, -3}, {{'H', 'L'}, -3}, {{'H', 'K'}, -1}, {{'H', 'M'}, -2},
    {{'H', 'F'}, -1}, {{'H', 'P'}, -2}, {{'H', 'S'}, -1}, {{'H', 'T'}, -2}, {{'H', 'W'}, -2},
    {{'H', 'Y'}, 2}, {{'H', 'V'}, -3},
    {{'I', 'I'}, 4}, {{'I', 'L'}, 2}, {{'I', 'K'}, -3}, {{'I', 'M'}, 1}, {{'I', 'F'}, 0},
    {{'I', 'P'}, -3}, {{'I', 'S'}, -2}, {{'I', 'T'}, -1}, {{'I', 'W'}, -3}, {{'I', 'Y'}, -1},
    {{'I', 'V'}, 3},
    {{'L', 'L'}, 4}, {{'L', 'K'}, -2}, {{'L', 'M'}, 2}, {{'L', 'F'}, 0}, {{'L', 'P'}, -3},
    {{'L', 'S'}, -2}, {{'L', 'T'}, -1}, {{'L', 'W'}, -2}, {{'L', 'Y'}, -1}, {{'L', 'V'}, 1},
    {{'K', 'K'}, 5}, {{'K', 'M'}, -1}, {{'K', 'F'}, -3}, {{'K', 'P'}, -1}, {{'K', 'S'}, 0},
    {{'K', 'T'}, -1}, {{'K', 'W'}, -3}, {{'K', 'Y'}, -2}, {{'K', 'V'}, -2},
    {{'M', 'M'}, 5}, {{'M', 'F'}, 0}, {{'M', 'P'}, -2}, {{'M', 'S'}, -1}, {{'M', 'T'}, -1},
    {{'M', 'W'}, -1}, {{'M', 'Y'}, -1}, {{'M', 'V'}, 1},
    {{'F', 'F'}, 6}, {{'F', 'P'}, -4}, {{'F', 'S'}, -2}, {{'F', 'T'}, -2}, {{'F', 'W'}, 1},
    {{'F', 'Y'}, 3}, {{'F', 'V'}, -1},
    {{'P', 'P'}, 7}, {{'P', 'S'}, -1}, {{'P', 'T'}, -1}, {{'P', 'W'}, -4}, {{'P', 'Y'}, -3},
    {{'P', 'V'}, -2},
    {{'S', 'S'}, 4}, {{'S', 'T'}, 1}, {{'S', 'W'}, -3}, {{'S', 'Y'}, -2}, {{'S', 'V'}, -2},
    {{'T', 'T'}, 5}, {{'T', 'W'}, -2}, {{'T', 'Y'}, -2}, {{'T', 'V'}, 0},
    {{'W', 'W'}, 11}, {{'W', 'Y'}, 2}, {{'W', 'V'}, -3},
    {{'Y', 'Y'}, 7}, {{'Y', 'V'}, -1},
    {{'V', 'V'}, 4}
};

const std::map<std::pair<char, char>, int> BLOSUM50 = {
    {{'A', 'A'}, 5}, {{'A', 'R'}, -2}, {{'A', 'N'}, -1}, {{'A', 'D'}, -2}, {{'A', 'C'}, -1},
    {{'A', 'Q'}, -1}, {{'A', 'E'}, -1}, {{'A', 'G'}, 0}, {{'A', 'H'}, -2}, {{'A', 'I'}, -1},
    {{'A', 'L'}, -2}, {{'A', 'K'}, -1}, {{'A', 'M'}, -1}, {{'A', 'F'}, -3}, {{'A', 'P'}, -1},
    {{'A', 'S'}, 1}, {{'A', 'T'}, 0}, {{'A', 'W'}, -3}, {{'A', 'Y'}, -2}, {{'A', 'V'}, 0},
    {{'R', 'R'}, 7}, {{'C', 'C'}, 13}, {{'D', 'D'}, 8}, {{'E', 'E'}, 6}, {{'F', 'F'}, 8},
    {{'G', 'G'}, 8}, {{'H', 'H'}, 10}, {{'I', 'I'}, 5}, {{'K', 'K'}, 6},
    {{'L', 'L'}, 5}, {{'M', 'M'}, 7}, {{'P', 'P'}, 10}, {{'Q', 'Q'}, 7},
    {{'S', 'S'}, 5}, {{'T', 'T'}, 5}, {{'V', 'V'}, 5}, {{'W', 'W'}, 15}, {{'Y', 'Y'}, 8}
};

const std::map<std::pair<char, char>, int> BLOSUM80 = {
    {{'A', 'A'}, 7}, {{'C', 'C'}, 13}, {{'D', 'D'}, 10}, {{'E', 'E'}, 8}, {{'F', 'F'}, 10},
    {{'G', 'G'}, 9}, {{'H', 'H'}, 12}, {{'I', 'I'}, 7}, {{'K', 'K'}, 8},
    {{'L', 'L'}, 6}, {{'M', 'M'}, 8}, {{'N', 'N'}, 9}, {{'P', 'P'}, 10},
    {{'Q', 'Q'}, 9}, {{'S', 'S'}, 7}, {{'T', 'T'}, 7}, {{'V', 'V'}, 7}, 
    {{'W', 'W'}, 16}, {{'Y', 'Y'}, 10}
};

const std::map<std::pair<char, char>, int> PAM250 = {
    {{'A', 'A'}, 2}, {{'C', 'C'}, 12}, {{'D', 'D'}, 4}, {{'E', 'E'}, 4}, {{'F', 'F'}, 9},
    {{'G', 'G'}, 5}, {{'H', 'H'}, 6}, {{'I', 'I'}, 5}, {{'K', 'K'}, 5},
    {{'L', 'L'}, 6}, {{'M', 'M'}, 6}, {{'N', 'N'}, 2}, {{'P', 'P'}, 6},
    {{'Q', 'Q'}, 4}, {{'S', 'S'}, 3}, {{'T', 'T'}, 3}, {{'V', 'V'}, 4},
    {{'W', 'W'}, 17}, {{'Y', 'Y'}, 10}
};

const std::map<std::pair<char, char>, int> DNA_MATRIX = {
    {{'A', 'A'}, 2}, {{'A', 'T'}, -1}, {{'A', 'G'}, -1}, {{'A', 'C'}, -1}, {{'A', 'U'}, -1},
    {{'T', 'T'}, 2}, {{'T', 'G'}, -1}, {{'T', 'C'}, -1}, {{'T', 'U'}, 2},
    {{'G', 'G'}, 2}, {{'G', 'C'}, -1}, {{'G', 'U'}, -1},
    {{'C', 'C'}, 2}, {{'C', 'U'}, -1},
    {{'U', 'U'}, 2}
};

/**
 * @brief Devuelve un puntero a la matriz predefinida correspondiente.
 */
const std::map<std::pair<char, char>, int>* obtenerMatrizPorTipo(TipoMatrizPuntuacion tipo) {
    switch(tipo) {
        case TipoMatrizPuntuacion::BLOSUM62:
            return &BLOSUM62;
        case TipoMatrizPuntuacion::BLOSUM50:
            return &BLOSUM50;
        case TipoMatrizPuntuacion::BLOSUM80:
            return &BLOSUM80;
        case TipoMatrizPuntuacion::PAM250:
        case TipoMatrizPuntuacion::PAM120:
        case TipoMatrizPuntuacion::PAM30:
            return &PAM250;
        case TipoMatrizPuntuacion::DNA_SIMPLE:
        case TipoMatrizPuntuacion::RNA_SIMPLE:
            return &DNA_MATRIX;
        default:
            return nullptr;
    }
}

int obtenerPuntuacionConConfig(char a, char b, const ConfiguracionPuntuacion& config) {
    a = std::toupper(a);
    b = std::toupper(b);
    
    if (config.tipo == TipoMatrizPuntuacion::SIMPLE_PERSONALIZADO) {
        return (a == b) ? config.parametrosSimples.coincidencia : config.parametrosSimples.sustitucion;
    }
    
    if (config.tipo == TipoMatrizPuntuacion::MATRIZ_PERSONALIZADA && config.matrizPersonalizada != nullptr) {
        auto it = config.matrizPersonalizada->find({a, b});
        if (it != config.matrizPersonalizada->end()) return it->second;
        it = config.matrizPersonalizada->find({b, a});
        if (it != config.matrizPersonalizada->end()) return it->second;
        return -4;
    }
    
    const auto* matriz = obtenerMatrizPorTipo(config.tipo);
    if (matriz != nullptr) {
        auto it = matriz->find({a, b});
        if (it != matriz->end()) return it->second;
        it = matriz->find({b, a});
        if (it != matriz->end()) return it->second;
    }
    
    return (a == b) ? 2 : -1;
}

std::string obtenerNombreMatriz(TipoMatrizPuntuacion tipo) {
    switch(tipo) {
        case TipoMatrizPuntuacion::BLOSUM62: return "BLOSUM62";
        case TipoMatrizPuntuacion::BLOSUM50: return "BLOSUM50";
        case TipoMatrizPuntuacion::BLOSUM80: return "BLOSUM80";
        case TipoMatrizPuntuacion::PAM250: return "PAM250";
        case TipoMatrizPuntuacion::PAM120: return "PAM120";
        case TipoMatrizPuntuacion::PAM30: return "PAM30";
        case TipoMatrizPuntuacion::DNA_SIMPLE: return "DNA Simple";
        case TipoMatrizPuntuacion::RNA_SIMPLE: return "RNA Simple";
        case TipoMatrizPuntuacion::SIMPLE_PERSONALIZADO: return "Simple Personalizado";
        case TipoMatrizPuntuacion::MATRIZ_PERSONALIZADA: return "Matriz Personalizada";
        default: return "Desconocido";
    }
}

int obtenerPenalidadGap(const ConfiguracionPuntuacion& config) {
    return config.parametrosSimples.gap;
}
