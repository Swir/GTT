param([Parameter(Mandatory=$true)][string]$PackageDirectory,[Parameter(Mandatory=$true)][string]$RuntimeLog,[string]$ExpectedGitSha=$env:GITHUB_SHA)
$ErrorActionPreference='Stop';Set-StrictMode -Version Latest
$PackageDirectory=[IO.Path]::GetFullPath($PackageDirectory);$RuntimeLog=[IO.Path]::GetFullPath($RuntimeLog)
if(-not(Test-Path $RuntimeLog)){throw "Runtime log missing: $RuntimeLog"};$log=Get-Content -Raw $RuntimeLog
$smokePath=Join-Path $PackageDirectory 'RUNTIME_SMOKE.json';if(-not(Test-Path $smokePath)){throw 'RUNTIME_SMOKE.json missing.'};$smoke=Get-Content -Raw $smokePath|ConvertFrom-Json
$required=@('WORLD','HUD','TRAFFIC','NPC','MISSION','WANTED','SAVE');$steps=@();$errors=@()
foreach($step in $required){$ok=$log -match ("DEMO_SCENARIO_STEP step="+[regex]::Escape($step)+" result=PASS");$steps+=[ordered]@{step=$step;passed=$ok};if(-not$ok){$errors+="missing scenario PASS: $step"}}
if($log -notmatch 'DEMO_SCENARIO_COMPLETE result=PASS'){$errors+='scenario did not complete with PASS'}
if($ExpectedGitSha -and $smoke.git_sha -and $smoke.git_sha -ne $ExpectedGitSha){$errors+='scenario/runtime SHA mismatch'}
$result=if($errors.Count){'FAIL'}else{'PASS'}
$out=Join-Path $PackageDirectory 'DEMO_SCENARIO.json';[ordered]@{schema='gtt.demo-scenario.v1';game='Grand Theft Tractor';result=$result;git_sha=$smoke.git_sha;steps=$steps;required_step_count=$required.Count;errors=$errors;observed_utc=(Get-Date).ToUniversalTime().ToString('o')}|ConvertTo-Json -Depth 6|Set-Content -Encoding UTF8 $out
Write-Host "[GTT] Deterministic demo scenario: $result -> $out";if($errors.Count){throw ('Demo scenario failed: '+($errors -join '; '))}
