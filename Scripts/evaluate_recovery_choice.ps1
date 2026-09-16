param([Parameter(Mandatory=$true)][string]$PackageDirectory,[Parameter(Mandatory=$true)][string]$RuntimeLog,[string]$ExpectedGitSha=$env:GITHUB_SHA)
$ErrorActionPreference='Stop';Set-StrictMode -Version Latest
$PackageDirectory=[IO.Path]::GetFullPath($PackageDirectory);$RuntimeLog=[IO.Path]::GetFullPath($RuntimeLog)
if(-not(Test-Path $RuntimeLog)){throw "Runtime log missing: $RuntimeLog"};$log=Get-Content -Raw $RuntimeLog
$smokePath=Join-Path $PackageDirectory 'RUNTIME_SMOKE.json';if(-not(Test-Path $smokePath)){throw 'RUNTIME_SMOKE.json missing.'};$smoke=Get-Content -Raw $smokePath|ConvertFrom-Json
$errors=@()
$offer=[regex]::Match($log,'DEMO_SCENARIO_RECOVERY_OFFER vehicle=(Rattleback82|Mulebox1200) result=PASS recommendation=(IMMOBILIZED|TOW_RECOMMENDED) tow_quote=([0-9]+) repair_quote=([0-9]+) severity=([0-9.]+)')
$choice=[regex]::Match($log,'DEMO_SCENARIO_RECOVERY_CHOICE vehicle=(Rattleback82|Mulebox1200) result=PASS waited=([0-9.]+) auto_tow=NO cash=([0-9]+) tire=([0-9.]+)')
$tow=[regex]::Match($log,'DEMO_SCENARIO_PLAYER_TOW vehicle=(Rattleback82|Mulebox1200) result=PASS requested=YES tow_paid=([0-9]+) cash_before=([0-9]+) cash_after=([0-9]+) damage_preserved=YES serviced=NO repair_quote=([0-9]+)')
$repair=[regex]::Match($log,'DEMO_SCENARIO_SEPARATE_REPAIR vehicle=(Rattleback82|Mulebox1200) result=PASS repair_paid=([0-9]+) cash_before=([0-9]+) cash_after=([0-9]+) condition_after=([0-9.]+) tire_after=([0-9.]+) body_min_after=([0-9.]+)')
$complete=$log -match 'DEMO_SCENARIO_RECOVERY_CHOICE_COMPLETE result=PASS vehicle=(Rattleback82|Mulebox1200) route=stranded-offer-manual-tow-paid-repair'
if(-not$offer.Success){$errors+='missing player recovery offer evidence'}
if(-not$choice.Success){$errors+='missing no-auto-tow player choice evidence'}elseif([double]$choice.Groups[2].Value -lt 7.5){$errors+='player choice window was too short to disprove legacy auto-tow'}
if(-not$tow.Success){$errors+='missing player-authorized tow evidence'}else{if([int]$tow.Groups[2].Value -le 0 -or [int]$tow.Groups[3].Value-[int]$tow.Groups[4].Value -ne [int]$tow.Groups[2].Value){$errors+='tow charge did not match measured cash delta'}}
if(-not$repair.Success){$errors+='missing separate paid workshop repair evidence'}else{if([int]$repair.Groups[2].Value -le 0 -or [int]$repair.Groups[3].Value-[int]$repair.Groups[4].Value -ne [int]$repair.Groups[2].Value){$errors+='repair charge did not match measured cash delta'};if([double]$repair.Groups[5].Value -lt .999 -or [double]$repair.Groups[6].Value -lt .999 -or [double]$repair.Groups[7].Value -lt .999){$errors+='workshop did not restore condition, tires and body'}}
if(-not$complete){$errors+='recovery choice runtime route did not complete'}
if($offer.Success -and $tow.Success -and $offer.Groups[1].Value -ne $tow.Groups[1].Value){$errors+='recovery offer/tow vehicle mismatch'}
if($tow.Success -and $repair.Success -and $tow.Groups[1].Value -ne $repair.Groups[1].Value){$errors+='tow/repair vehicle mismatch'}
if($offer.Success -and $tow.Success -and [int]$offer.Groups[3].Value -ne [int]$tow.Groups[2].Value){$errors+='authorized tow did not use the quoted price'}
if($ExpectedGitSha -and $smoke.git_sha -and $ExpectedGitSha -ne 'unknown' -and $smoke.git_sha -ne $ExpectedGitSha){$errors+='recovery/runtime SHA mismatch'}
$result=if($errors.Count){'FAIL'}else{'PASS'}
$out=Join-Path $PackageDirectory 'RECOVERY_CHOICE.json'
[ordered]@{schema='gtt.recovery-choice.v1';game='Grand Theft Tractor';result=$result;git_sha=$smoke.git_sha;vehicle=if($offer.Success){$offer.Groups[1].Value}else{''};offer_passed=$offer.Success;manual_choice_passed=$choice.Success;tow_preserves_damage_passed=$tow.Success;separate_repair_passed=$repair.Success;route_complete=$complete;tow_quote=if($offer.Success){[int]$offer.Groups[3].Value}else{0};repair_quote=if($tow.Success){[int]$tow.Groups[5].Value}else{0};choice_wait_seconds=if($choice.Success){[double]$choice.Groups[2].Value}else{0};errors=$errors;observed_utc=(Get-Date).ToUniversalTime().ToString('o')}|ConvertTo-Json -Depth 6|Set-Content -Encoding UTF8 $out
Write-Host "[GTT] Player-selectable recovery + tow-preserves-damage + separate repair evidence: $result -> $out";if($errors.Count){throw ('Recovery choice evidence failed: '+($errors -join '; '))}