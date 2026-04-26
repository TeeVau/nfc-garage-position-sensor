[CmdletBinding()]
param(
  [string]$Port = "COM3",
  [int]$Baudrate = 115200,
  [int]$DurationSeconds = 15,
  [string]$OutputFile = "",
  [switch]$Append,
  [switch]$Timestamp,
  [switch]$Quiet
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ($DurationSeconds -le 0) {
  throw "DurationSeconds muss groesser als 0 sein."
}

function Format-CaptureLine {
  param(
    [Parameter(Mandatory = $true)][string]$Line,
    [switch]$AddTimestamp
  )

  if (-not $AddTimestamp) {
    return $Line
  }

  $stamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss.fff"
  return "[$stamp] $Line"
}

$writer = $null
$serialPort = [System.IO.Ports.SerialPort]::new($Port, $Baudrate, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
$serialPort.ReadTimeout = 500
$serialPort.DtrEnable = $false
$serialPort.RtsEnable = $false
$serialPort.NewLine = "`n"

try {
  if (-not [string]::IsNullOrWhiteSpace($OutputFile)) {
    $outputDir = Split-Path -Parent $OutputFile
    if (-not [string]::IsNullOrWhiteSpace($outputDir)) {
      New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
    }

    $writer = [System.IO.StreamWriter]::new($OutputFile, $Append, [System.Text.Encoding]::UTF8)
  }

  $serialPort.Open()

  if (-not $Quiet) {
    Write-Host "Capture gestartet: $Port @ $Baudrate Baud fuer $DurationSeconds s"
    if ($writer -ne $null) {
      Write-Host "Logdatei: $OutputFile"
    }
  }

  $serialPort.DiscardInBuffer()
  $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()

  while ($stopwatch.Elapsed.TotalSeconds -lt $DurationSeconds) {
    try {
      $line = $serialPort.ReadLine().TrimEnd("`r")
      if ([string]::IsNullOrWhiteSpace($line)) {
        continue
      }

      $formattedLine = Format-CaptureLine -Line $line -AddTimestamp:$Timestamp

      if (-not $Quiet) {
        Write-Output $formattedLine
      }

      if ($writer -ne $null) {
        $writer.WriteLine($formattedLine)
        $writer.Flush()
      }
    }
    catch [System.TimeoutException] {
    }
  }
}
finally {
  if ($writer -ne $null) {
    $writer.Dispose()
  }

  if ($serialPort.IsOpen) {
    $serialPort.Close()
  }

  $serialPort.Dispose()
}
