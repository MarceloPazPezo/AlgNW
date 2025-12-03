# Script PowerShell para ejecutar benchmark con diferentes schedules y chunks
# Permite probar configuraciones específicas de OpenMP

param(
    [string]$Metodo = "bloques",  # "antidiagonal" o "bloques"
    [int[]]$Threads = @(2, 4, 6, 8),
    [string[]]$Schedules = @("static", "static,1", "static,2", "static,4", "static,8", "static,16", "static,32", "dynamic,1", "dynamic,2", "dynamic,4", "dynamic,8", "dynamic,16", "dynamic,32", "guided,1", "guided,2", "guided,4", "guided,8"),
    [string[]]$Archivos = @("dna_1k", "dna_2k", "dna_4k", "dna_8k", "dna_16k"),
    [int]$Repeticiones = 2,  # Menos repeticiones para explorar más configuraciones
    [string]$OutputDir = "resultados",
    [string]$OutputFile = "resultados_test.csv"
)

# Colores para output
function Write-ColorOutput {
    param([string]$Message, [string]$Color = "White")
    Write-Host $Message -ForegroundColor $Color
}

# Configuración
$DATOS_DIR = "datos"
$RESULTADOS_DIR = $OutputDir
$ARCHIVO_SALIDA = Join-Path $RESULTADOS_DIR $OutputFile

# Parámetros de puntuación
$MATCH = 2
$MISMATCH = -1
$GAP = -2

# Crear directorios
if (-not (Test-Path $RESULTADOS_DIR)) {
    New-Item -ItemType Directory -Path $RESULTADOS_DIR | Out-Null
}

# Limpiar archivo de salida anterior si existe
if (Test-Path $ARCHIVO_SALIDA) {
    Remove-Item $ARCHIVO_SALIDA -Force
}

# Verificar que los programas existen
if (-not (Test-Path "bin\main-paralelo.exe")) {
    Write-ColorOutput "Error: bin\main-paralelo.exe no encontrado" "Red"
    Write-ColorOutput "Ejecuta: make bin/main-paralelo" "Yellow"
    exit 1
}

Write-ColorOutput "========================================" "Green"
Write-ColorOutput "BENCHMARK CONFIGURABLE" "Green"
Write-ColorOutput "========================================" "Green"
Write-Host ""
Write-ColorOutput "Método: $Metodo" "Cyan"
Write-ColorOutput "Threads: $($Threads -join ', ')" "Cyan"
Write-ColorOutput "Schedules: $($Schedules -join ', ')" "Cyan"
Write-ColorOutput "Archivos: $($Archivos -join ', ')" "Cyan"
Write-ColorOutput "Repeticiones: $Repeticiones" "Cyan"
Write-ColorOutput "Archivo de salida: $ARCHIVO_SALIDA" "Cyan"
Write-Host ""

$totalTests = $Threads.Count * $Schedules.Count * $Archivos.Count
$currentTest = 0

foreach ($archivoBase in $Archivos) {
    $ARCHIVO = Join-Path $DATOS_DIR "$archivoBase.fasta"
    
    if (-not (Test-Path $ARCHIVO)) {
        Write-ColorOutput "Saltando $archivoBase (archivo no encontrado: $ARCHIVO)" "Yellow"
        continue
    }
    
    Write-ColorOutput "========================================" "Blue"
    Write-ColorOutput "Archivo: $archivoBase.fasta" "Blue"
    Write-ColorOutput "========================================" "Blue"
    Write-Host ""
    
    foreach ($schedule in $Schedules) {
        Write-ColorOutput ">>> Schedule: $schedule" "Yellow"
        
        # Configurar schedule para OpenMP
        $env:OMP_SCHEDULE = $schedule
        
        foreach ($t in $Threads) {
            $currentTest++
            $env:OMP_NUM_THREADS = $t.ToString()
            
            Write-Host -NoNewline "   Threads: $t... "
            
            # Ejecutar main-paralelo con método específico
            $process = Start-Process -FilePath "bin\main-paralelo.exe" `
                -ArgumentList @(
                    "-f", $ARCHIVO,
                    "-p", $MATCH, $MISMATCH, $GAP,
                    "-r", $Repeticiones,
                    "-m", $Metodo,
                    "-o", $ARCHIVO_SALIDA
                ) `
                -NoNewWindow `
                -Wait `
                -PassThru `
                -RedirectStandardOutput "nul" `
                -RedirectStandardError "nul"
            
            if ($process.ExitCode -eq 0) {
                Write-ColorOutput "OK" "Green"
            } else {
                Write-ColorOutput "FAIL (exit code: $($process.ExitCode))" "Red"
            }
        }
        Write-Host ""
    }
    Write-Host ""
}

Write-ColorOutput "========================================" "Green"
Write-ColorOutput "Benchmark finalizado" "Green"
Write-ColorOutput "========================================" "Green"
Write-Host ""
Write-ColorOutput "Resultados guardados en: $ARCHIVO_SALIDA" "Cyan"
Write-Host ""
Write-ColorOutput "Para analizar resultados:" "Yellow"
Write-Host "  python graficar_resultados.py -i $ARCHIVO_SALIDA -o graficos"

