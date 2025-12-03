# Script PowerShell para benchmarks con tamaños grandes (32k)
# Enfocado en verificar que el paralelo supere al secuencial

param(
    [Parameter(Mandatory=$false)]
    [string]$Metodo = "bloques",
    
    [Parameter(Mandatory=$false)]
    [int[]]$Threads = @(8),
    
    [Parameter(Mandatory=$false)]
    [string[]]$Archivos = @("dna_32k"),
    
    [Parameter(Mandatory=$false)]
    [int]$Repeticiones = 5,  # Más repeticiones para estabilidad
    
    [Parameter(Mandatory=$false)]
    [string]$OutputFile = "resultados_grandes.csv"
)

# Colores para output
function Write-ColorOutput {
    param([string]$Message, [string]$Color = "White")
    Write-Host $Message -ForegroundColor $Color
}

# Schedules más prometedores basados en análisis anterior
$Schedules = @(
    "dynamic",           # Mejor en resultados anteriores
    "dynamic,1",
    "dynamic,2",
    "guided,1",          # Mejor en exploratorio
    "guided,2",
    "static,4",          # Buen balance
    "static,8"
)

# Configuración
$DATOS_DIR = "datos"
$RESULTADOS_DIR = "resultados"
$ARCHIVO_SALIDA = Join-Path $RESULTADOS_DIR $OutputFile

# Parámetros de puntuación
$MATCH = 2
$MISMATCH = -1
$GAP = -2

# Crear directorios
if (-not (Test-Path $RESULTADOS_DIR)) {
    New-Item -ItemType Directory -Path $RESULTADOS_DIR | Out-Null
}

# Verificar que los programas existen
$exePath = "bin\main-paralelo.exe"
if (-not (Test-Path $exePath)) {
    # Intentar sin .exe (Linux/WSL)
    $exePath = "bin\main-paralelo"
    if (-not (Test-Path $exePath)) {
        Write-ColorOutput "Error: bin\main-paralelo no encontrado" "Red"
        Write-ColorOutput "Ejecuta: .\compilar-windows.ps1" "Yellow"
        exit 1
    }
}

Write-ColorOutput "========================================" "Green"
Write-ColorOutput "BENCHMARK TAMAÑOS GRANDES" "Green"
Write-ColorOutput "========================================" "Green"
Write-Host ""
Write-ColorOutput "Método: $Metodo" "Cyan"
Write-ColorOutput "Threads: $($Threads -join ', ')" "Cyan"
Write-ColorOutput "Schedules: $($Schedules.Count) configuraciones" "Cyan"
Write-ColorOutput "Archivos: $($Archivos -join ', ')" "Cyan"
Write-ColorOutput "Repeticiones: $Repeticiones" "Cyan"
Write-ColorOutput "Archivo de salida: $ARCHIVO_SALIDA" "Cyan"
Write-Host ""

# Primero ejecutar secuencial para comparación
Write-ColorOutput "========================================" "Blue"
Write-ColorOutput "Ejecutando SECUENCIAL (baseline)" "Blue"
Write-ColorOutput "========================================" "Blue"
Write-Host ""

foreach ($archivoBase in $Archivos) {
    $ARCHIVO = Join-Path $DATOS_DIR "$archivoBase.fasta"
    
    if (-not (Test-Path $ARCHIVO)) {
        Write-ColorOutput "Saltando $archivoBase (archivo no encontrado: $ARCHIVO)" "Yellow"
        continue
    }
    
    Write-Host -NoNewline "  Ejecutando secuencial para $archivoBase... "
    
    $env:OMP_NUM_THREADS = "1"
    $process = Start-Process -FilePath $exePath `
        -ArgumentList @(
            "-f", $ARCHIVO,
            "-p", $MATCH, $MISMATCH, $GAP,
            "-r", $Repeticiones,
            "-m", "secuencial",
            "-o", $ARCHIVO_SALIDA
        ) `
        -NoNewWindow `
        -Wait `
        -PassThru `
        -RedirectStandardOutput "$env:TEMP\benchmark_out.txt" `
        -RedirectStandardError "$env:TEMP\benchmark_err.txt"
    
    if ($process.ExitCode -eq 0) {
        Write-ColorOutput "OK" "Green"
    } else {
        Write-ColorOutput "FAIL" "Red"
    }
}

Write-Host ""

# Ahora ejecutar paralelo
Write-ColorOutput "========================================" "Blue"
Write-ColorOutput "Ejecutando PARALELO" "Blue"
Write-ColorOutput "========================================" "Blue"
Write-Host ""

$startTime = Get-Date
$totalTests = $Threads.Count * $Schedules.Count * $Archivos.Count
$currentTest = 0

foreach ($archivoBase in $Archivos) {
    $ARCHIVO = Join-Path $DATOS_DIR "$archivoBase.fasta"
    
    if (-not (Test-Path $ARCHIVO)) {
        Write-ColorOutput "Saltando $archivoBase (archivo no encontrado: $ARCHIVO)" "Yellow"
        continue
    }
    
    Write-ColorOutput "----------------------------------------" "Blue"
    Write-ColorOutput "Archivo: $archivoBase.fasta" "Blue"
    Write-ColorOutput "----------------------------------------" "Blue"
    Write-Host ""
    
    foreach ($schedule in $Schedules) {
        # Configurar schedule para OpenMP
        $env:OMP_SCHEDULE = $schedule
        
        foreach ($t in $Threads) {
            $currentTest++
            $env:OMP_NUM_THREADS = $t.ToString()
            
            $porcentaje = [Math]::Round(($currentTest / $totalTests) * 100, 1)
            Write-Host -NoNewline "  [$porcentaje%] $schedule (T=$t)... "
            
            # Ejecutar main-paralelo con método específico
            $process = Start-Process -FilePath $exePath `
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
                -RedirectStandardOutput "$env:TEMP\benchmark_out.txt" `
                -RedirectStandardError "$env:TEMP\benchmark_err.txt"
            
            if ($process.ExitCode -eq 0) {
                Write-ColorOutput "OK" "Green"
            } else {
                Write-ColorOutput "FAIL" "Red"
            }
        }
    }
    Write-Host ""
}

$endTime = Get-Date
$duration = $endTime - $startTime

Write-ColorOutput "========================================" "Green"
Write-ColorOutput "Benchmark finalizado" "Green"
Write-ColorOutput "========================================" "Green"
Write-Host ""
Write-ColorOutput "Tiempo total: $($duration.ToString('hh\:mm\:ss'))" "Cyan"
Write-ColorOutput "Pruebas completadas: $currentTest" "Cyan"
Write-ColorOutput "Resultados guardados en: $ARCHIVO_SALIDA" "Cyan"
Write-Host ""
Write-ColorOutput "Para comparar secuencial vs paralelo:" "Yellow"
Write-Host "  python comparar_secuencial_paralelo.py $ARCHIVO_SALIDA"
Write-Host ""

