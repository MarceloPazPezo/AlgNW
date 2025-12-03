# Script para compilar el proyecto para Windows
# Requiere MinGW-w64 con soporte OpenMP o Visual Studio

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "COMPILACIÓN PARA WINDOWS" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Verificar compiladores disponibles
$compilador = $null
$openmpFlag = ""

# Intentar encontrar MinGW
$mingw = Get-Command g++ -ErrorAction SilentlyContinue
if ($mingw) {
    Write-Host "✓ Encontrado: g++ (MinGW)" -ForegroundColor Green
    $compilador = "g++"
    $openmpFlag = "-fopenmp"
    
    # Verificar si tiene soporte OpenMP
    $testOpenMP = & g++ -fopenmp --version 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ Soporte OpenMP disponible" -ForegroundColor Green
    } else {
        Write-Host "⚠ Advertencia: OpenMP puede no estar disponible" -ForegroundColor Yellow
    }
}
# Intentar encontrar Visual Studio
else {
    $vs = Get-Command cl -ErrorAction SilentlyContinue
    if ($vs) {
        Write-Host "✓ Encontrado: cl (Visual Studio)" -ForegroundColor Green
        $compilador = "cl"
        $openmpFlag = "/openmp"
        Write-Host "⚠ Nota: Necesitarás configurar el entorno de Visual Studio" -ForegroundColor Yellow
    }
}

if (-not $compilador) {
    Write-Host "✗ No se encontró compilador C++" -ForegroundColor Red
    Write-Host ""
    Write-Host "Opciones:" -ForegroundColor Yellow
    Write-Host "1. Instalar MinGW-w64 con OpenMP:" -ForegroundColor White
    Write-Host "   - Descargar de: https://www.mingw-w64.org/" -ForegroundColor White
    Write-Host "   - O usar MSYS2: pacman -S mingw-w64-x86_64-gcc" -ForegroundColor White
    Write-Host ""
    Write-Host "2. Usar Visual Studio con OpenMP" -ForegroundColor White
    Write-Host ""
    Write-Host "3. Ejecutar manualmente usando WSL o el binario de Linux" -ForegroundColor White
    exit 1
}

Write-Host ""
Write-Host "Compilando para Windows..." -ForegroundColor Cyan
Write-Host ""

# Crear directorio bin si no existe
if (-not (Test-Path "bin")) {
    New-Item -ItemType Directory -Path "bin" | Out-Null
}

# Flags de compilación
$CXXFLAGS = "-O3 -Wall -std=c++11 -Iinclude -Isrc"
$CXXFLAGS_PARALELO = "$CXXFLAGS $openmpFlag"

# Archivos fuente
$PARALELO_SRCS = @(
    "src/main-paralelo.cpp",
    "src/secuencial.cpp",
    "src/paralelo.cpp",
    "src/puntuacion.cpp",
    "src/utilidades.cpp"
)

# Nombre del ejecutable
$EXE_NAME = "bin\main-paralelo.exe"

# Compilar
Write-Host "Compilando main-paralelo..." -ForegroundColor Yellow
if ($compilador -eq "g++") {
    $comando = "$compilador $CXXFLAGS_PARALELO -o $EXE_NAME $($PARALELO_SRCS -join ' ')"
    Invoke-Expression $comando
} elseif ($compilador -eq "cl") {
    # Visual Studio requiere configuración especial
    Write-Host "⚠ Para Visual Studio, ejecuta desde 'Developer Command Prompt'" -ForegroundColor Yellow
    Write-Host "   cl $CXXFLAGS_PARALELO /Fe:$EXE_NAME $($PARALELO_SRCS -join ' ')" -ForegroundColor White
    exit 1
}

if ($LASTEXITCODE -eq 0 -and (Test-Path $EXE_NAME)) {
    Write-Host ""
    Write-Host "✓ Compilación exitosa!" -ForegroundColor Green
    Write-Host "  Ejecutable: $EXE_NAME" -ForegroundColor Cyan
} else {
    Write-Host ""
    Write-Host "✗ Error en la compilación" -ForegroundColor Red
    exit 1
}

