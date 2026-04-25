param(
  [string]$Port = "COM3",
  [int]$Baudrate = 115200,
  [int]$RetryDelayMs = 1500,
  [int]$PortSettleMs = 0
)

$arduinoCli = "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"

if (-not (Test-Path $arduinoCli)) {
  Write-Error "arduino-cli wurde nicht gefunden: $arduinoCli"
  exit 1
}

function Test-PortAvailable {
  param([string]$Name)

  return [System.IO.Ports.SerialPort]::GetPortNames() -contains $Name
}

Write-Host "Auto-Reconnect-Monitor fuer $Port mit $Baudrate Baud gestartet."
Write-Host "Port-Settle-Delay: $PortSettleMs ms"
Write-Host "Beenden mit Ctrl+C."

$waitingForPort = $false

while ($true) {
  if (-not (Test-PortAvailable -Name $Port)) {
    if (-not $waitingForPort) {
      Write-Host "Port $Port nicht verfuegbar. Warte auf Wiederanmeldung..."
      $waitingForPort = $true
    }
    Start-Sleep -Milliseconds $RetryDelayMs
    continue
  }

  $waitingForPort = $false

  if ($PortSettleMs -gt 0) {
    Write-Host "Port $Port erkannt. Warte $PortSettleMs ms bis USB/CDC stabil ist..."
    Start-Sleep -Milliseconds $PortSettleMs

    if (-not (Test-PortAvailable -Name $Port)) {
      Write-Host "Port $Port ist waehrend der Stabilisierung wieder verschwunden."
      Start-Sleep -Milliseconds $RetryDelayMs
      continue
    }
  }

  Write-Host "Starte Monitor auf $Port..."
  & $arduinoCli monitor -p $Port -c "baudrate=$Baudrate"

  Write-Host "Verbindung zu $Port beendet. Versuche Neuverbindung..."
  Start-Sleep -Milliseconds $RetryDelayMs
}
