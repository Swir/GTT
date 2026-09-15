param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [Parameter(Mandatory=$true)][string]$RuntimeLog,
    [string]$ExpectedGitSha = $env:GITHUB_SHA,
    [int]$MinimumRuntimeSeconds = 20
)
$ErrorActionPreference='Stop'; Set-StrictMode -Version Latest
$PackageDirectory=[IO.Path]::GetFullPath($PackageDirectory); $RuntimeLog=[IO.Path]::GetFullPath($RuntimeLog)
if(-not(Test-Path $RuntimeLog -PathType Leaf)){throw "Runtime log missing: $RuntimeLog"}
$smokePath=Join-Path $PackageDirectory 'RUNTIME_SMOKE.json'; if(-not(Test-Path $smokePath)){throw 'RUNTIME_SMOKE.json missing.'}
$smoke=Get-Content -Raw $smokePath|ConvertFrom-Json; $log=Get-Content -Raw $RuntimeLog
$errors=@(); if($smoke.result -ne 'PASS'){$errors+='runtime smoke is not PASS'}
if([int]$smoke.survived_seconds -lt $MinimumRuntimeSeconds){$errors+="runtime shorter than $MinimumRuntimeSeconds seconds"}
if($ExpectedGitSha -and $smoke.git_sha -and $smoke.git_sha -ne $ExpectedGitSha){$errors+='runtime evidence SHA mismatch'}
$fatalPatterns=@('Fatal error:','Unhandled Exception:','LowLevelFatalError','Assertion failed:')
foreach($p in $fatalPatterns){if($log.Contains($p)){$errors+="fatal runtime signature: $p"}}
$vehicles=@(
  [ordered]@{id='RustyFieldmaster60';label='Fieldmaster';aliases=@('RustyFieldmaster60','Fieldmaster')},
  [ordered]@{id='Rattleback82';label='Rattleback';aliases=@('Rattleback82','Rattleback 82','Rattleback')},
  [ordered]@{id='Mulebox1200';label='Mulebox';aliases=@('Mulebox1200','Mulebox 1200','Mulebox')}
)
$vehicleEvidence=@(); foreach($v in $vehicles){$ready=$false; foreach($line in ($log -split "`r?`n")){if($line -notmatch 'NATIVE_CHAOS_SMOKE_READY'){continue}; foreach($a in $v.aliases){if($line -like "*$a*"){$ready=$true;break}}; if($ready){break}}
 $vehicleEvidence += [ordered]@{vehicle_id=$v.id; smoke_ready=$ready}; if(-not $ready){$errors+="missing NATIVE_CHAOS_SMOKE_READY for $($v.id)"}}
$coverage=[ordered]@{
 world_boot=($log -match 'GTTPrototypeWorld|PrototypeWorld|GTTGameMode');
 traffic_runtime=($log -match 'GTTTraffic|TrafficDirector|traffic');
 npc_runtime=($log -match 'GTTCitizen|CitizenPawn|civilian|NPC');
 mission_runtime=($log -match 'BorrowedTractor|Mission|mission');
 police_or_wanted_runtime=($log -match 'Police|Wanted|wanted');
}
$result=if($errors.Count -eq 0){'PASS'}else{'FAIL'}
$manifest=[ordered]@{schema='gtt.packaged-gameplay-smoke.v1';game='Grand Theft Tractor';result=$result;git_sha=$smoke.git_sha;runtime_seconds=[int]$smoke.survived_seconds;fatal_scan_passed=($fatalPatterns|Where-Object{$log.Contains($_)}).Count -eq 0;native_fleet=$vehicleEvidence;representative_coverage=$coverage;coverage_note='Coverage booleans are diagnostic until dedicated deterministic scenario markers are emitted; fleet Native Chaos evidence and fatal scan are hard gates.';errors=$errors;observed_utc=(Get-Date).ToUniversalTime().ToString('o')}
$out=Join-Path $PackageDirectory 'GAMEPLAY_SMOKE.json'; $manifest|ConvertTo-Json -Depth 8|Set-Content -Encoding UTF8 $out
Write-Host "[GTT] Packaged gameplay smoke: $result -> $out"; if($errors.Count){throw ('Packaged gameplay smoke failed: '+($errors -join '; '))}
