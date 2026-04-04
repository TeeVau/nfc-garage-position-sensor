param(
  [string]$Port = "COM3",
  [int]$Baudrate = 115200,
  [int]$RetryDelayMs = 1500
)

$arduinoCli = "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"

if (-not (Test-Path $arduinoCli)) {
  Write-Error "arduino-cli wurde nicht gefunden: $arduinoCli"
  exit 1
}

Write-Host "Auto-Reconnect-Monitor fuer $Port mit $Baudrate Baud gestartet."
Write-Host "Beenden mit Ctrl+C."

while ($true) {
  $availablePorts = [System.IO.Ports.SerialPort]::GetPortNames()

  if ($availablePorts -notcontains $Port) {
    Write-Host "Port $Port nicht verfuegbar. Warte auf Wiederanmeldung..."
    Start-Sleep -Milliseconds $RetryDelayMs
    continue
  }

  Write-Host "Verbinde mit $Port..."
  & $arduinoCli monitor -p $Port -c "baudrate=$Baudrate"

  Write-Host "Verbindung zu $Port beendet. Versuche Neuverbindung..."
  Start-Sleep -Milliseconds $RetryDelayMs
}
