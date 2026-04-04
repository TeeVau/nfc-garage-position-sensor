param(
  [string]$Port = "COM3",
  [string]$Fqbn = "esp32:esp32:esp32c6:ZigbeeMode=ed,PartitionScheme=zigbee,CDCOnBoot=cdc,EraseFlash=all"
)

$arduinoCli = "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"

if (-not (Test-Path $arduinoCli)) {
  Write-Error "arduino-cli wurde nicht gefunden: $arduinoCli"
  exit 1
}

& $arduinoCli upload -p $Port --fqbn $Fqbn .
