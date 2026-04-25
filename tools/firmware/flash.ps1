param(
  [string]$Port = "COM3",
  [string]$Fqbn = "esp32:esp32:esp32c6:ZigbeeMode=ed,PartitionScheme=zigbee,CDCOnBoot=cdc",
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

$buildPath = Get-BuildPath -BoardFqbn $Fqbn -Root (Join-Path $repoRoot $BuildRoot)

if (-not (Test-Path $buildPath)) {
  Write-Error "Build-Pfad nicht gefunden: $buildPath. Bitte zuerst .\\tools\\firmware\\build.ps1 ausfuehren."
  exit 1
}

if (-not (Test-Path $sketchPath)) {
  Write-Error "Sketch-Pfad nicht gefunden: $sketchPath"
  exit 1
}

Write-Host "Upload from build path: $buildPath"
& $arduinoCli upload -p $Port --fqbn $Fqbn --build-path $buildPath $sketchPath
