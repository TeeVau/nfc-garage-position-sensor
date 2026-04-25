param(
  [string]$Fqbn = "esp32:esp32:esp32c6:ZigbeeMode=ed,PartitionScheme=zigbee,CDCOnBoot=cdc",
  [string]$OutputDir = "build",
  [string]$BuildRoot = ".arduino-build",
  [string]$SketchDir = "src\nfc-garage-position-sensor"
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

$buildRootPath = Join-Path $repoRoot $BuildRoot
$outputDirPath = Join-Path $repoRoot $OutputDir
$buildPath = Get-BuildPath -BoardFqbn $Fqbn -Root $buildRootPath
New-Item -ItemType Directory -Force -Path $buildPath | Out-Null
New-Item -ItemType Directory -Force -Path $outputDirPath | Out-Null

if (-not (Test-Path $sketchPath)) {
  Write-Error "Sketch-Pfad nicht gefunden: $sketchPath"
  exit 1
}

Write-Host "Build path: $buildPath"
& $arduinoCli compile --fqbn $Fqbn --build-path $buildPath --output-dir $outputDirPath $sketchPath
