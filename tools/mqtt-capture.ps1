[CmdletBinding()]
param(
    [Alias("Server")]
    [string]$BrokerHost = $(if ($env:Z2M_MQTT_HOST) { $env:Z2M_MQTT_HOST } else { "localhost" }),

    [int]$Port = $(if ($env:Z2M_MQTT_PORT) { [int]$env:Z2M_MQTT_PORT } else { 1883 }),

    [string]$BaseTopic = $(if ($env:Z2M_MQTT_BASE_TOPIC) { $env:Z2M_MQTT_BASE_TOPIC } else { "zigbee2mqtt" }),

    [string[]]$Topic = @(),

    [string]$DeviceId = "",

    [ValidateRange(1, 3600)]
    [int]$DurationSeconds = 30,

    [string]$OutputFile = "",

    [string]$ErrorFile = "",

    [string]$Username = $(if ($env:Z2M_MQTT_USERNAME) { $env:Z2M_MQTT_USERNAME } else { "zigbee2mqtt" }),

    [string]$ClientId = "",

    [ValidateRange(0, 2)]
    [int]$Qos = 0,

    [switch]$IncludeDefaultTopics,

    [switch]$PassThru,

    [switch]$NoCredential,

    [switch]$SaveCredential,

    [switch]$ForgetCredential,

    [switch]$VerboseMqtt,

    [string]$CredentialPath = $env:Z2M_MQTT_CREDENTIAL_PATH,

    [string]$CredentialName = "default"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-MosquittoTool {
    param([Parameter(Mandatory = $true)][string]$BaseName)

    $command = Get-Command "$BaseName.exe" -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    $programFilesPath = Join-Path $env:ProgramFiles "mosquitto\$BaseName.exe"
    if (Test-Path -LiteralPath $programFilesPath) {
        return $programFilesPath
    }

    throw "$BaseName.exe was not found. Add C:\Program Files\mosquitto to PATH or reinstall the Mosquitto clients."
}

function Get-CredentialStorePath {
    param([Parameter(Mandatory = $true)][string]$Name)

    $safeName = $Name -replace "[^A-Za-z0-9._-]", "_"
    $storeDir = Join-Path $env:LOCALAPPDATA "nfc-garage-position-sensor\mqtt"
    Join-Path $storeDir "$safeName.credential.xml"
}

function Get-AquariumCredentialStorePath {
    param([Parameter(Mandatory = $true)][string]$Name)

    $safeName = $Name -replace "[^A-Za-z0-9._-]", "_"
    $storeDir = Join-Path $env:LOCALAPPDATA "aquarium-cooling-controller\mqtt"
    Join-Path $storeDir "$safeName.credential.xml"
}

function Convert-SecureStringToPlainText {
    param([Parameter(Mandatory = $true)][securestring]$SecureString)

    $bstr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($SecureString)
    try {
        [Runtime.InteropServices.Marshal]::PtrToStringBSTR($bstr)
    }
    finally {
        [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($bstr)
    }
}

function Read-MqttCredential {
    param([string]$DefaultUsername)

    $promptUsername = $DefaultUsername
    if ([string]::IsNullOrWhiteSpace($promptUsername)) {
        $promptUsername = Read-Host "MQTT username"
    }

    $promptPassword = Read-Host "MQTT password" -AsSecureString
    [pscredential]::new($promptUsername, $promptPassword)
}

function Remove-StoredCredential {
    param([Parameter(Mandatory = $true)][string]$Path)

    if (Test-Path -LiteralPath $Path) {
        Remove-Item -LiteralPath $Path
        Write-Host "Removed stored MQTT credential: $Path"
    }
}

function Resolve-Topics {
    $topics = [System.Collections.Generic.List[string]]::new()
    $baseTopicClean = $BaseTopic.Trim("/")

    if ($IncludeDefaultTopics -or $Topic.Count -eq 0) {
        $topics.Add("$baseTopicClean/bridge/logging")
        $topics.Add("$baseTopicClean/bridge/event")
        $topics.Add("$baseTopicClean/bridge/response/#")
        $topics.Add("$baseTopicClean/bridge/devices")
    }

    foreach ($topicItem in $Topic) {
        if (-not [string]::IsNullOrWhiteSpace($topicItem)) {
            $topics.Add($topicItem)
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($DeviceId)) {
        $topics.Add("$baseTopicClean/$DeviceId")
    }

    $topics | Select-Object -Unique
}

function Ensure-ParentDirectory {
    param([Parameter(Mandatory = $true)][string]$Path)

    $parent = Split-Path -Parent $Path
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        New-Item -ItemType Directory -Force -Path $parent | Out-Null
    }
}

$defaultCredentialPath = Get-CredentialStorePath -Name $CredentialName
$fallbackCredentialPath = Get-AquariumCredentialStorePath -Name $CredentialName
$credentialPath = if (-not [string]::IsNullOrWhiteSpace($CredentialPath)) {
    $CredentialPath
} elseif (Test-Path -LiteralPath $defaultCredentialPath) {
    $defaultCredentialPath
} elseif (Test-Path -LiteralPath $fallbackCredentialPath) {
    $fallbackCredentialPath
} else {
    $defaultCredentialPath
}

if ($ForgetCredential) {
    Remove-StoredCredential -Path $credentialPath
}

$topics = Resolve-Topics
if ($topics.Count -eq 0) {
    throw "Es wurde kein MQTT-Topic ausgewaehlt."
}

if ([string]::IsNullOrWhiteSpace($BrokerHost)) {
    $BrokerHost = Read-Host "MQTT broker host"
}

$credential = $null
if (-not $NoCredential) {
    if (Test-Path -LiteralPath $credentialPath) {
        $credential = Import-Clixml -LiteralPath $credentialPath
        Write-Host "Using DPAPI-protected credential from: $credentialPath"
    } else {
        $credential = Read-MqttCredential -DefaultUsername $Username

        if ($SaveCredential) {
            $credentialDir = Split-Path -Parent $defaultCredentialPath
            New-Item -ItemType Directory -Force -Path $credentialDir | Out-Null
            $credential | Export-Clixml -LiteralPath $defaultCredentialPath
            Write-Host "Saved DPAPI-protected credential to: $defaultCredentialPath"
        }
    }
}

$effectiveOutputFile = $OutputFile
if ([string]::IsNullOrWhiteSpace($effectiveOutputFile)) {
    $effectiveOutputFile = Join-Path $env:TEMP ("mqtt-capture-{0:yyyyMMdd-HHmmss}.log" -f (Get-Date))
}

$effectiveErrorFile = $ErrorFile
if ([string]::IsNullOrWhiteSpace($effectiveErrorFile)) {
    $effectiveErrorFile = [System.IO.Path]::ChangeExtension($effectiveOutputFile, ".err.log")
}

Ensure-ParentDirectory -Path $effectiveOutputFile
Ensure-ParentDirectory -Path $effectiveErrorFile

$plainPassword = $null
try {
    if ($credential) {
        $plainPassword = Convert-SecureStringToPlainText -SecureString $credential.Password
    }

    $subPath = Resolve-MosquittoTool -BaseName "mosquitto_sub"
    $subArgs = @("-h", $BrokerHost, "-p", $Port.ToString(), "-q", $Qos.ToString(), "-v")

    if (-not [string]::IsNullOrWhiteSpace($ClientId)) {
        $subArgs += @("-i", $ClientId)
    }

    if ($credential) {
        $subArgs += @("-u", $credential.UserName, "-P", $plainPassword)
    }

    if ($VerboseMqtt) {
        $subArgs += "-d"
    }

    foreach ($topicItem in $topics) {
        $subArgs += @("-t", $topicItem)
    }

    Write-Host "MQTT capture gestartet fuer $DurationSeconds s"
    Write-Host "Broker: $BrokerHost`:$Port"
    Write-Host "Topics:"
    foreach ($topicItem in $topics) {
        Write-Host "  $topicItem"
    }
    Write-Host "Logdatei: $effectiveOutputFile"
    Write-Host "Fehlerlog: $effectiveErrorFile"

    $process = Start-Process -FilePath $subPath -ArgumentList $subArgs -RedirectStandardOutput $effectiveOutputFile -RedirectStandardError $effectiveErrorFile -WindowStyle Hidden -PassThru

    try {
        if (-not $process.WaitForExit($DurationSeconds * 1000)) {
            Stop-Process -Id $process.Id -Force
            $process.WaitForExit()
        }
    }
    finally {
        $process.Dispose()
    }

    if ($PassThru -or [string]::IsNullOrWhiteSpace($OutputFile)) {
        if (Test-Path -LiteralPath $effectiveOutputFile) {
            Get-Content -LiteralPath $effectiveOutputFile
        }
    }
}
finally {
    $plainPassword = $null
}
