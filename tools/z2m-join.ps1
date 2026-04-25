<#
.SYNOPSIS
Enables, disables, or checks Zigbee2MQTT permit join over MQTT.

.DESCRIPTION
This script publishes to the Zigbee2MQTT MQTT API topic
zigbee2mqtt/bridge/request/permit_join.

By default it prompts for MQTT credentials when needed. If -SaveCredential is
used, the credential is stored with Windows DPAPI in %LOCALAPPDATA%, outside
the synced project workspace.

Mosquitto's command line tools require the password as a process argument. This
script does not echo the command, write secrets to logs, or create secret files
inside the repository.

.EXAMPLE
.\tools\z2m-join.ps1 ein

Allows devices to join Zigbee2MQTT for the default duration, then saves the MQTT
credential with Windows DPAPI when no stored credential exists yet and
-SaveCredential is added.

.EXAMPLE
.\tools\z2m-join.ps1 aus

Disables joining immediately.

.EXAMPLE
.\tools\z2m-join.ps1 status

Reads the retained Zigbee2MQTT bridge info topic and prints the current permit
join state.
#>

[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [ValidateSet("ein", "aus", "on", "off", "status")]
    [string]$State = "status",

    [Alias("Server")]
    [string]$BrokerHost = $(if ($env:Z2M_MQTT_HOST) { $env:Z2M_MQTT_HOST } else { "192.168.178.2" }),

    [int]$Port = $(if ($env:Z2M_MQTT_PORT) { [int]$env:Z2M_MQTT_PORT } else { 1883 }),

    [string]$BaseTopic = $(if ($env:Z2M_MQTT_BASE_TOPIC) { $env:Z2M_MQTT_BASE_TOPIC } else { "zigbee2mqtt" }),

    [ValidateRange(1, 254)]
    [int]$DurationSec = 254,

    [string]$Device = "",

    [string]$Username = $(if ($env:Z2M_MQTT_USERNAME) { $env:Z2M_MQTT_USERNAME } else { "zigbee2mqtt" }),

    [string]$ClientId = "",

    [ValidateRange(0, 2)]
    [int]$Qos = 0,

    [ValidateRange(1, 60)]
    [int]$StatusTimeoutSec = 5,

    [switch]$NoCredential,

    [switch]$SaveCredential,

    [switch]$ForgetCredential,

    [switch]$SkipStatus,

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
    } else {
        Write-Host "No stored MQTT credential found at: $Path"
    }
}

function Convert-StateName {
    param([Parameter(Mandatory = $true)][string]$Value)

    switch ($Value.ToLowerInvariant()) {
        "ein" { "on"; break }
        "on" { "on"; break }
        "aus" { "off"; break }
        "off" { "off"; break }
        default { "status" }
    }
}

function New-MqttArgs {
    param(
        [Parameter(Mandatory = $true)][string]$Topic,
        [Parameter(Mandatory = $true)][string]$ToolName
    )

    $args = @("-h", $BrokerHost, "-p", $Port.ToString(), "-t", $Topic, "-q", $Qos.ToString())

    if (-not [string]::IsNullOrWhiteSpace($ClientId)) {
        $clientSuffix = if ($ToolName -eq "mosquitto_sub") { "-status" } else { "-set" }
        $args += @("-i", "$ClientId$clientSuffix")
    }

    if ($credential) {
        $args += @("-u", $credential.UserName, "-P", $plainPassword)
    }

    if ($VerboseMqtt) {
        $args += "-d"
    }

    $args
}

function Read-BridgeInfoPayload {
    param([Parameter(Mandatory = $true)][string]$Topic)

    $subPath = Resolve-MosquittoTool -BaseName "mosquitto_sub"
    $subArgs = New-MqttArgs -Topic $Topic -ToolName "mosquitto_sub"
    $subArgs += @("-C", "1", "-W", $StatusTimeoutSec.ToString())

    $output = & $subPath @subArgs 2>&1
    $exitCode = $LASTEXITCODE

    if ($exitCode -ne 0) {
        throw "Could not read Zigbee2MQTT bridge info from '$Topic' within $StatusTimeoutSec seconds. mosquitto_sub exit code: $exitCode. Output: $output"
    }

    if (-not $output) {
        throw "No bridge info payload was received from '$Topic'."
    }

    ($output | Select-Object -First 1).ToString()
}

function Write-BridgeInfoStatus {
    param([Parameter(Mandatory = $true)][string]$Payload)

    $permitJoin = $false
    if ($Payload -match '"permit_join"\s*:\s*(true|false)') {
        $permitJoin = $Matches[1] -eq "true"
    } else {
        throw "Bridge info payload does not contain a permit_join field."
    }

    $message = if ($permitJoin) { "ON" } else { "OFF" }

    if ($permitJoin -and ($Payload -match '"permit_join_end"\s*:\s*(\d+)')) {
        $rawEndTime = [int64]$Matches[1]

        # Some Zigbee2MQTT installations expose permit_join_end in milliseconds.
        # Accept both units so the status helper stays robust across setups.
        if ($rawEndTime -gt 253402300799) {
            $rawEndTime = [int64]($rawEndTime / 1000)
        }

        if ($rawEndTime -ge -62135596800 -and $rawEndTime -le 253402300799) {
            $endTime = [DateTimeOffset]::FromUnixTimeSeconds($rawEndTime).LocalDateTime
            $message = "$message until $($endTime.ToString("yyyy-MM-dd HH:mm:ss"))"
        }
    }

    Write-Host "Zigbee2MQTT permit join: $message"
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
$credentialMaintenanceOnly = $ForgetCredential -and (
    @($PSBoundParameters.Keys | Where-Object { $_ -notin @("ForgetCredential", "CredentialName", "CredentialPath") }).Count -eq 0
)

if ($credentialMaintenanceOnly) {
    Remove-StoredCredential -Path $credentialPath
    return
}

if ([string]::IsNullOrWhiteSpace($BrokerHost)) {
    $BrokerHost = Read-Host "MQTT broker host"
}

$baseTopicClean = $BaseTopic.Trim("/")
$requestTopic = "$baseTopicClean/bridge/request/permit_join"
$infoTopic = "$baseTopicClean/bridge/info"
$normalizedState = Convert-StateName -Value $State

if ($ForgetCredential) {
    Remove-StoredCredential -Path $credentialPath
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

$plainPassword = $null
try {
    if ($credential) {
        $plainPassword = Convert-SecureStringToPlainText -SecureString $credential.Password
    }

    if ($normalizedState -eq "status") {
        $infoPayload = Read-BridgeInfoPayload -Topic $infoTopic
        Write-BridgeInfoStatus -Payload $infoPayload
        return
    }

    $payload = [ordered]@{
        time = if ($normalizedState -eq "on") { $DurationSec } else { 0 }
    }

    if (-not [string]::IsNullOrWhiteSpace($Device)) {
        $payload.device = $Device
    }

    $payloadJson = $payload | ConvertTo-Json -Compress
    $payloadFile = [System.IO.Path]::GetTempFileName()
    $pubPath = Resolve-MosquittoTool -BaseName "mosquitto_pub"
    $pubArgs = New-MqttArgs -Topic $requestTopic -ToolName "mosquitto_pub"
    Set-Content -LiteralPath $payloadFile -Value $payloadJson -Encoding ASCII -NoNewline
    $pubArgs += @("-f", $payloadFile)

    $actionText = if ($normalizedState -eq "on") { "Enabling" } else { "Disabling" }
    Write-Host "$actionText Zigbee2MQTT permit join via '$requestTopic' on $BrokerHost`:$Port."

    try {
        & $pubPath @pubArgs
        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }
    }
    finally {
        Remove-Item -LiteralPath $payloadFile -ErrorAction SilentlyContinue
    }

    if (-not $SkipStatus) {
        Start-Sleep -Seconds 1
        $infoPayload = Read-BridgeInfoPayload -Topic $infoTopic
        Write-BridgeInfoStatus -Payload $infoPayload
    }
}
finally {
    $plainPassword = $null
}
