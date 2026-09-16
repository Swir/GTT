param([Parameter(Mandatory=$true)][string]$PackageDirectory,[Parameter(Mandatory=$true)][string]$RuntimeLog,[string]$ExpectedGitSha=$env:GITHUB_SHA)
$ErrorActionPreference='Stop';Set-StrictMode -Version Latest
$PackageDirectory=[IO.Path]::GetFullPath($PackageDirectory);$RuntimeLog=[IO.Path]::GetFullPath($RuntimeLog)
if(-not(Test-Path $RuntimeLog)){throw "Runtime log missing: $RuntimeLog"};$log=Get-Content -Raw $RuntimeLog
$smokePath=Join-Path $PackageDirectory 'RUNTIME_SMOKE.json';if(-not(Test-Path $smokePath)){throw 'RUNTIME_SMOKE.json missing.'};$smoke=Get-Content -Raw $smokePath|ConvertFrom-Json
$required=@('WORLD','HUD','TRAFFIC','NPC','MISSION','COMBAT','FIELDMASTER','FIELDMASTER_MOTION','FIELDMASTER_CONTROL','RATTLEBACK','RATTLEBACK_MOTION','RATTLEBACK_CONTROL','MULEBOX','MULEBOX_MOTION','MULEBOX_CONTROL','WANTED_COMPONENT','WANTED_ESCALATION','POLICE_RESPONSE','PURSUIT_ACTIVE','PURSUIT_CLOSING','ROADBLOCK_ACTIVE','INTERCEPTION_ACTIVE','SAVE');$steps=@();$errors=@()
foreach($step in $required){$ok=$log -match ("DEMO_SCENARIO_STEP step="+[regex]::Escape($step)+" result=PASS");$steps+=[ordered]@{step=$step;passed=$ok};if(-not$ok){$errors+="missing scenario PASS: $step"}}
$crimeAction=$log -match 'DEMO_SCENARIO_ACTION action=INJECT_TEST_CRIME heat=130.0 target_wanted=4';if(-not$crimeAction){$errors+='deterministic wanted-4 crime action was not executed'}
$controlEvidence=@('Fieldmaster','Rattleback82','Mulebox1200')|ForEach-Object{$log -match ("DEMO_SCENARIO_CONTROL vehicle="+$_+" throttle=")};if($controlEvidence -contains $false){$errors+='native control telemetry incomplete'}
$pursuitEvidence=$log -match 'DEMO_SCENARIO_PURSUIT closing=PASS baseline_cm=';if(-not$pursuitEvidence){$errors+='pursuit did not prove measurable closing distance'}
$roadblockEvidence=$log -match 'DEMO_SCENARIO_ROADBLOCK active=PASS count=';if(-not$roadblockEvidence){$errors+='wanted-4 roadblock did not become active'}
$interceptionEvidence=$log -match 'DEMO_SCENARIO_INTERCEPTION active=PASS node=';if(-not$interceptionEvidence){$errors+='road-node interception did not become active'}
$nativeSpikeMatch=[regex]::Match($log,'ROADBLOCK_SPIKE_CONSEQUENCE vehicle=(Rattleback82|Mulebox1200) path=NATIVE_CHAOS tier=([0-9]+) hit=([0-9]+) tire_before=([0-9.]+) tire_after=([0-9.]+) tire_delta=([0-9.]+)')
$nativeSpikeEvidence=$nativeSpikeMatch.Success
$nativeSpikeVehicle='';$nativeSpikeBefore=0.0;$nativeSpikeAfter=0.0;$nativeSpikeDelta=0.0
if($nativeSpikeEvidence){$nativeSpikeVehicle=$nativeSpikeMatch.Groups[1].Value;$nativeSpikeBefore=[double]$nativeSpikeMatch.Groups[4].Value;$nativeSpikeAfter=[double]$nativeSpikeMatch.Groups[5].Value;$nativeSpikeDelta=[double]$nativeSpikeMatch.Groups[6].Value;if($nativeSpikeAfter -ge $nativeSpikeBefore -or $nativeSpikeDelta -le 0){$nativeSpikeEvidence=$false}}
if(-not$nativeSpikeEvidence){$errors+='no measurable Native Chaos spike-strip consequence was observed'}
if($log -notmatch 'DEMO_SCENARIO_COMPLETE result=PASS steps=23'){$errors+='wanted-4 roadblock/interception scenario did not complete with 23-step PASS'}
if($ExpectedGitSha -and $smoke.git_sha -and $smoke.git_sha -ne $ExpectedGitSha){$errors+='scenario/runtime SHA mismatch'}
$result=if($errors.Count){'FAIL'}else{'PASS'}
$out=Join-Path $PackageDirectory 'DEMO_SCENARIO.json';[ordered]@{schema='gtt.demo-scenario.v6';game='Grand Theft Tractor';result=$result;git_sha=$smoke.git_sha;route='native-roadblock-consequence';steps=$steps;required_step_count=$required.Count;crime_action_passed=$crimeAction;native_control_evidence=$controlEvidence;pursuit_closing_passed=$pursuitEvidence;roadblock_active_passed=$roadblockEvidence;interception_active_passed=$interceptionEvidence;native_spike_consequence_passed=$nativeSpikeEvidence;native_spike_vehicle=$nativeSpikeVehicle;native_spike_tire_before=$nativeSpikeBefore;native_spike_tire_after=$nativeSpikeAfter;native_spike_tire_delta=$nativeSpikeDelta;errors=$errors;observed_utc=(Get-Date).ToUniversalTime().ToString('o')}|ConvertTo-Json -Depth 6|Set-Content -Encoding UTF8 $out
Write-Host "[GTT] Native roadblock consequence demo scenario: $result -> $out";if($errors.Count){throw ('Demo scenario failed: '+($errors -join '; '))}
