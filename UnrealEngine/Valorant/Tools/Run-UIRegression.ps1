param(
    [Parameter(Mandatory)]
    [string]$EngineRoot,
    [ValidateRange(1024, 65535)]
    [int]$Port = 17941,
    [ValidateRange(60, 600)]
    [int]$TimeoutSeconds = 180
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$project = Join-Path $projectRoot 'Valorant.uproject'
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
if (!(Test-Path -LiteralPath $editor) -or !(Test-Path -LiteralPath $project)) {
    throw 'Supply the UE 5.5 engine root and run from the Valorant project checkout.'
}
if (Get-NetUDPEndpoint -LocalPort $Port -ErrorAction SilentlyContinue) {
    throw "UDP port $Port is already in use."
}

$archive = Join-Path $projectRoot 'Saved/Logs/UIRegression'
$run = Join-Path $archive (Get-Date -Format 'yyyyMMdd-HHmmss-fff')
New-Item -ItemType Directory -Path $run -Force | Out-Null
$required = @(Get-Content -LiteralPath (Join-Path $PSScriptRoot 'UIRegressionChecks.json') -Raw | ConvertFrom-Json)
$sourceCommit = (& git -C $projectRoot rev-parse HEAD).Trim()
$outcome = 'FAIL'
$failureReason = ''
$common = @(
    ('"{0}"' -f $project)
    '-game'
    '-RenderOffscreen'
    '-unattended'
    '-NoSplash'
    '-NoSound'
    '-nosteam'
    '-NoLoadingScreen'
    '-DisablePlugins=USDCore,USDImporter,USDMultiUser,ChaosCachingUSD'
    '-UIRegressionReplay'
    '-ini:Engine:[OnlineSubsystem]:DefaultPlatformService=NULL'
    '-ini:Engine:[HTTP]:bEnableHttp=False'
    '-LogCmds="LogMatchHUD Verbose"'
    '-ExecCmds="t.MaxFPS 30"'
)
$processes = @()
$records = @()
$hostLog = Join-Path $run 'host.log'
$clientLog = Join-Path $run 'client1.log'
$timeoutLog = Join-Path $run 'client2.log'
try {
    $hostArgs = $common + @('/Game/Maps/MatchMap?listen', "-port=$Port", '-multihome=127.0.0.1',
        '-nullrhi', '-UIRegressionAgent=1', ('-abslog="{0}"' -f $hostLog))
    $hostProcess = Start-Process -FilePath $editor -ArgumentList $hostArgs -PassThru -WindowStyle Hidden
    $processes += $hostProcess
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while (!(Test-Path -LiteralPath $hostLog) -or !(Select-String -LiteralPath $hostLog -Pattern "IpNetDriver listening on port $Port" -Quiet)) {
        if ($hostProcess.HasExited -or (Get-Date) -ge $deadline) {
            throw "Listen Server did not start. See $hostLog."
        }
        Start-Sleep -Seconds 1
    }
    foreach ($index in @(2, 3, 1)) {
        $log = Join-Path $run "client$index.log"
        $argsForClient = $common + @("127.0.0.1:$Port", ('-abslog="{0}"' -f $log),
            "-UIRegressionAgent=$($index + 1)", '-ini:Engine:[/Script/OnlineSubsystemUtils.IpNetDriver]:InitialConnectTimeout=75')
        if ($index -eq 1) {
            $argsForClient += @('-dx12', '-windowed', '-ForceRes', '-ResX=1280', '-ResY=720',
                ('-UIRegressionHud="{0}"' -f (Join-Path $run 'hud.png')),
                ('-UIRegressionShop="{0}"' -f (Join-Path $run 'shop.png')))
        } else {
            $argsForClient += '-nullrhi'
        }
        $processes += Start-Process -FilePath $editor -ArgumentList $argsForClient -PassThru -WindowStyle Hidden
    }

    do {
        $notes = @(Get-ChildItem -LiteralPath $run -Filter '*.log' |
            Select-String -Pattern 'SHOP CHECK (PASS|FAIL|BLOCKED)|REPLAY (PASS|FAIL|BLOCKED)|Fatal error')
        $records = @($notes | ForEach-Object {
            if ($_.Line -match '(?:SHOP CHECK|REPLAY) (PASS|FAIL|BLOCKED) \[([A-Z0-9_.]+)\]') {
                [pscustomobject]@{
                    id = $Matches[2]
                    process = [IO.Path]::GetFileNameWithoutExtension($_.Filename)
                    status = $Matches[1]
                }
            }
        })
        $missing = @($required | Where-Object {
            $expected = $_
            @($records | Where-Object { $_.id -eq $expected.id -and $_.process -eq $expected.process }).Count -ne 1
        })
        if ($notes.Line -match 'Fatal error|(?:SHOP CHECK|REPLAY) (FAIL|BLOCKED)') {
            throw 'A process reported a failed or blocked check.'
        }
        if ($records | Group-Object id | Where-Object Count -ne 1) {
            throw 'A regression check executed more than once.'
        }
        if ($records | Where-Object { $_.id -notin $required.id }) {
            throw 'The process returned a check absent from the required manifest.'
        }
        if ($processes.Count -ne 4 -or ($processes | Where-Object HasExited)) {
            throw 'A required replay process exited before the result was accepted.'
        }
        if ($missing.Count -eq 0) {
            $outcome = 'PASS'
            break
        }
        if (($processes | Where-Object HasExited) -or (Get-Date) -ge $deadline) {
            throw 'Replay ended before every required check executed.'
        }
        Start-Sleep -Seconds 1
    } while ($true)
} catch {
    $failureReason = $_.Exception.Message

} finally {
    foreach ($process in $processes) {
        if (!$process.HasExited) {
            Stop-Process -InputObject $process
        }
    }
}

$results = @($required | ForEach-Object {
    $expected = $_
    $found = @($records | Where-Object { $_.id -eq $expected.id -and $_.process -eq $expected.process })
    $status = 'NOT RUN'
    if ($found.Count -eq 1) { $status = $found[0].status }
    if ($found.Count -gt 1) { $status = 'FAIL' }
    [pscustomobject]@{ id = $expected.id; process = $expected.process; status = $status; occurrences = $found.Count }
})
[pscustomobject]@{
    status = $outcome
    sourceCommit = $sourceCommit
    requiredCount = $required.Count
    passedCount = @($results | Where-Object status -eq 'PASS').Count
    failure = $failureReason
    checks = $results
} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $run 'results.json') -Encoding utf8
$results | Format-Table process, id, status, occurrences -AutoSize
"Results and screenshots: $run"
if ($outcome -ne 'PASS') { throw $failureReason }
