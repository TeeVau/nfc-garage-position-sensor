param(
  [string]$Fqbn = "esp32:esp32:esp32c6:ZigbeeMode=ed,PartitionScheme=zigbee,CDCOnBoot=cdc",
  [string]$OutputDir = "build",
  [string]$ReleaseDir = "bin",
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

function Get-SoftwareVersion([string]$ConfigPath) {
  if (-not (Test-Path $ConfigPath)) {
    Write-Error "Konfigurationsdatei nicht gefunden: $ConfigPath"
    exit 1
  }

  $configContent = Get-Content -Raw -Path $ConfigPath
  $versionMatch = [System.Text.RegularExpressions.Regex]::Match(
    $configContent,
    'SOFTWARE_VERSION\s*=\s*"([^"]+)"'
  )

  if (-not $versionMatch.Success) {
    Write-Error "SOFTWARE_VERSION konnte nicht aus $ConfigPath gelesen werden."
    exit 1
  }

  return $versionMatch.Groups[1].Value
}

function Get-VersionedFirmwareName([string]$ProjectName, [string]$Version, [string]$BuildVariant) {
  $variantSuffix = if ($BuildVariant -eq "standard") { "" } else { "-$BuildVariant" }
  return "{0}-{1}{2}.bin" -f $ProjectName, $Version, $variantSuffix
}

$buildRootPath = Join-Path $repoRoot $BuildRoot
$variant = Get-BuildVariant -BleDebugEnabled $EnableBleDebug.IsPresent
$outputDirPath = Join-Path (Join-Path $repoRoot $OutputDir) $variant
$releaseDirPath = Join-Path $repoRoot $ReleaseDir
$buildPath = Join-Path (Get-BuildPath -BoardFqbn $Fqbn -Root $buildRootPath) $variant
New-Item -ItemType Directory -Force -Path $buildPath | Out-Null
New-Item -ItemType Directory -Force -Path $outputDirPath | Out-Null
New-Item -ItemType Directory -Force -Path $releaseDirPath | Out-Null

if (-not (Test-Path $sketchPath)) {
  Write-Error "Sketch-Pfad nicht gefunden: $sketchPath"
  exit 1
}

$configPath = Join-Path $sketchPath "config.h"
$softwareVersion = Get-SoftwareVersion -ConfigPath $configPath
$projectName = Split-Path $sketchPath -Leaf

Write-Host "Build path: $buildPath"
Write-Host "Build variant: $variant"
Write-Host "Software version: $softwareVersion"

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

if ($LASTEXITCODE -ne 0) {
  exit $LASTEXITCODE
}

$firmwareFileName = "$projectName.ino.bin"
$firmwarePath = Join-Path $outputDirPath $firmwareFileName

if (-not (Test-Path $firmwarePath)) {
  Write-Error "Firmware-Datei nicht gefunden: $firmwarePath"
  exit 1
}

$releaseFileName = Get-VersionedFirmwareName -ProjectName $projectName -Version $softwareVersion -BuildVariant $variant
$releaseFilePath = Join-Path $releaseDirPath $releaseFileName
Copy-Item -LiteralPath $firmwarePath -Destination $releaseFilePath -Force

Write-Host "Versionierte Firmware: $releaseFilePath"
