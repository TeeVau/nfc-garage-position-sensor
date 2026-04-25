param(
  [string]$Port = "COM3",
  [int]$Baudrate = 115200,
  [int]$DurationSeconds = 15,
  [string]$OutputFile = ""
)

if ($DurationSeconds -le 0) {
  Write-Error "DurationSeconds muss groesser als 0 sein."
  exit 1
}

$serialPort = New-Object System.IO.Ports.SerialPort $Port, $Baudrate, 'None', 8, 'one'
$serialPort.ReadTimeout = 500
$serialPort.DtrEnable = $false
$serialPort.RtsEnable = $false

$writer = $null

try {
  if ($OutputFile) {
    $writer = [System.IO.StreamWriter]::new($OutputFile, $true, [System.Text.Encoding]::UTF8)
  }

  $serialPort.Open()
  Write-Host "Capture gestartet: $Port @ $Baudrate Baud fuer $DurationSeconds s"
  if ($OutputFile) {
    Write-Host "Logdatei: $OutputFile"
  }

  $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()

  while ($stopwatch.Elapsed.TotalSeconds -lt $DurationSeconds) {
    try {
      $line = $serialPort.ReadLine()
      if (-not [string]::IsNullOrWhiteSpace($line)) {
        Write-Output $line
        if ($writer -ne $null) {
          $writer.WriteLine($line)
          $writer.Flush()
        }
      }
    } catch [System.TimeoutException] {
    }
  }
} finally {
  if ($writer -ne $null) {
    $writer.Dispose()
  }

  if ($serialPort.IsOpen) {
    $serialPort.Close()
  }
}
