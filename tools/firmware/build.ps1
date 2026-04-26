param(
  [string]$Fqbn = "esp32:esp32:esp32c6:ZigbeeMode=ed,PartitionScheme=zigbee,CDCOnBoot=cdc",
  [string]$OutputDir = "build",
  [string]$BuildRoot = ".arduino-build",
  [string]$SketchDir = "src\nfc-garage-position-sensor",
  [switch]$EnableBleDebug
)

$arduinoCli = "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\.."))
$sketchPath = Join-Path $repoRoot $SketchDir

if (-not (Test-Path $arduinoCli)) {
  Write-Error "arduino-cli wurde nicht gefunden: $arduinoCli"
  exit 1
}

function Get-BuildPath([string]$BoardFqbn, [string]$Root) {
  $safeName = $BoardFqbn -replace '[^A-Za-z0-9._-]', '_'
  return Join-Path $Root $safeName
}

function Get-BuildVariant([bool]$BleDebugEnabled) {
  if ($BleDebugEnabled) {
    return "ble-debug"
  }

  return "standard"
}

$buildRootPath = Join-Path $repoRoot $BuildRoot
$variant = Get-BuildVariant -BleDebugEnabled $EnableBleDebug.IsPresent
$outputDirPath = Join-Path (Join-Path $repoRoot $OutputDir) $variant
$buildPath = Join-Path (Get-BuildPath -BoardFqbn $Fqbn -Root $buildRootPath) $variant
New-Item -ItemType Directory -Force -Path $buildPath | Out-Null
New-Item -ItemType Directory -Force -Path $outputDirPath | Out-Null

if (-not (Test-Path $sketchPath)) {
  Write-Error "Sketch-Pfad nicht gefunden: $sketchPath"
  exit 1
}

Write-Host "Build path: $buildPath"
Write-Host "Build variant: $variant"

$compileArgs = @(
  "compile",
  "--fqbn", $Fqbn,
  "--build-path", $buildPath,
  "--output-dir", $outputDirPath
)

if ($EnableBleDebug) {
  $compileArgs += @("--build-property", "compiler.cpp.extra_flags=-DBLE_DEBUG_ENABLED=1")
}

$compileArgs += $sketchPath

& $arduinoCli @compileArgs
