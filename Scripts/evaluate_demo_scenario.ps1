param([Parameter(Mandatory=$true)][string]$PackageDirectory,[Parameter(Mandatory=$true)][string]$RuntimeLog,[string]$ExpectedGitSha=$env:GITHUB_SHA)
$ErrorActionPreference='Stop';Set-StrictMode -Version Latest
$PackageDirectory=[IO.Path]::GetFullPath($PackageDirectory);$RuntimeLog=[IO.Path]::GetFullPath($RuntimeLog)
if(-not(Test-Path $RuntimeLog)){throw "Runtime log missing: $RuntimeLog"};$log=Get-Content -Raw $RuntimeLog
$smokePath=Join-Path $PackageDirectory 'RUNTIME_SMOKE.json';if(-not(Test-Path $smokePath)){throw 'RUNTIME_SMOKE.json missing.'};$smoke=Get-Content -Raw $smokePath|ConvertFrom-Json
$required=@('WORLD','HUD','TRAFFIC','NPC','MISSION','COMBAT','FIELDMASTER','FIELDMASTER_MOTION','FIELDMASTER_CONTROL','RATTLEBACK','RATTLEBACK_MOTION','RATTLEBACK_CONTROL','MULEBOX','MULEBOX_MOTION','MULEBOX_CONTROL','WANTED_COMPONENT','WANTED_ESCALATION','POLICE_RESPONSE','PURSUIT_ACTIVE','PURSUIT_CLOSING','SAVE');$steps=@();$errors=@()
foreach($step in $required){$ok=$log -match ("DEMO_SCENARIO_STEP step="+[regex]::Escape($step)+" result=PASS");$steps+=[ordered]@{step=$step;passed=$ok};if(-not$ok){$errors+="missing scenario PASS: $step"}}
$crimeAction=$log -match 'DEMO_SCENARIO_ACTION action=INJECT_TEST_CRIME heat=80.0 target_wanted=3';if(-not$crimeAction){$errors+='deterministic wanted-3 crime action was not executed'}
$controlEvidence=@('Fieldmaster','Rattleback82','Mulebox1200')|ForEach-Object{$log -match ("DEMO_SCENARIO_CONTROL vehicle="+$_+" throttle=")};if($controlEvidence -contains $false){$errors+='native control telemetry incomplete'}
$pursuitEvidence=$log -match 'DEMO_SCENARIO_PURSUIT closing=PASS baseline_cm=';if(-not$pursuitEvidence){$errors+='pursuit did not prove measurable closing distance'}
if($log -notmatch 'DEMO_SCENARIO_COMPLETE result=PASS steps=21'){$errors+='vehicle-control/pursuit-interaction scenario did not complete with 21-step PASS'}
if($ExpectedGitSha -and $smoke.git_sha -and $smoke.git_sha -ne $ExpectedGitSha){$errors+='scenario/runtime SHA mismatch'}
$result=if($errors.Count){'FAIL'}else{'PASS'}
$out=Join-Path $PackageDirectory 'DEMO_SCENARIO.json';[ordered]@{schema='gtt.demo-scenario.v4';game='Grand Theft Tractor';result=$result;git_sha=$smoke.git_sha;route='vehicle-control-pursuit-interaction';steps=$steps;required_step_count=$required.Count;crime_action_passed=$crimeAction;native_control_evidence=$controlEvidence;pursuit_closing_passed=$pursuitEvidence;errors=$errors;observed_utc=(Get-Date).ToUniversalTime().ToString('o')}|ConvertTo-Json -Depth 6|Set-Content -Encoding UTF8 $out
Write-Host "[GTT] Vehicle-control/pursuit-interaction demo scenario: $result -> $out";if($errors.Count){throw ('Demo scenario failed: '+($errors -join '; '))}
