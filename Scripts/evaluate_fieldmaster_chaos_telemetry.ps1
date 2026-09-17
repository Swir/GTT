param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [Parameter(Mandatory=$true)][string]$RuntimeLog,
    [string]$ExpectedGitSha=$env:GITHUB_SHA,
    [int]$MinimumSamples=3
)

$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$PackageDirectory=[IO.Path]::GetFullPath($PackageDirectory)
$RuntimeLog=[IO.Path]::GetFullPath($RuntimeLog)
if(-not(Test-Path $PackageDirectory -PathType Container)){throw "Package directory missing: $PackageDirectory"}
if(-not(Test-Path $RuntimeLog -PathType Leaf)){throw "Runtime log missing: $RuntimeLog"}
if($MinimumSamples -lt 3){throw 'MinimumSamples must be at least 3.'}

$buildPath=Join-Path $PackageDirectory 'BUILD_INFO.json'
if(-not(Test-Path $buildPath)){throw "BUILD_INFO.json missing: $buildPath"}
$build=Get-Content -Raw $buildPath|ConvertFrom-Json
if($build.platform -ne 'Win64'){throw "Telemetry evidence requires Win64 package, got $($build.platform)."}
if($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown' -and $build.git_sha -ne $ExpectedGitSha){throw "Build SHA mismatch: package=$($build.git_sha), expected=$ExpectedGitSha"}

$number='[-+]?[0-9]+(?:\.[0-9]+)?'
$pattern="FIELDMASTER_CHAOS_TELEMETRY result=OBSERVED config_valid=(?<config>[01]) active=(?<active>[01]) gear=(?<gear>-?[0-9]+) target_gear=(?<target>-?[0-9]+) rpm=(?<rpm>$number) max_rpm=(?<maxrpm>$number) speed_kmh=(?<speed>$number) requested_throttle=(?<requested>$number) throttle=(?<throttle>$number) steering=(?<steering>$number) drive_health=(?<health>$number) steering_grip=(?<steergrip>$number) terrain_grip=(?<terrain>$number) valid_wheels=(?<valid>[0-9]+) contacts=(?<contacts>[0-9]+) suspension_samples=(?<suspcount>[0-9]+) slipping=(?<slipping>[0-9]+) skidding=(?<skidding>[0-9]+) suspension_min=(?<suspmin>$number) suspension_max=(?<suspmax>$number) spring_force=(?<spring>$number) max_slip=(?<slip>$number) drive_torque=(?<drive>$number) brake_torque=(?<brake>$number)"
$culture=[Globalization.CultureInfo]::InvariantCulture
function Num([System.Text.RegularExpressions.Match]$m,[string]$name){return [double]::Parse($m.Groups[$name].Value,$culture)}

$samples=@()
foreach($line in Get-Content $RuntimeLog){
    $m=[regex]::Match($line,$pattern)
    if(-not $m.Success){continue}
    $sample=[ordered]@{
        config_valid=[int]$m.Groups['config'].Value
        active=[int]$m.Groups['active'].Value
        gear=[int]$m.Groups['gear'].Value
        target_gear=[int]$m.Groups['target'].Value
        rpm=Num $m 'rpm'
        max_rpm=Num $m 'maxrpm'
        speed_kmh=Num $m 'speed'
        requested_throttle=Num $m 'requested'
        throttle=Num $m 'throttle'
        steering=Num $m 'steering'
        drive_health=Num $m 'health'
        steering_grip=Num $m 'steergrip'
        terrain_grip=Num $m 'terrain'
        valid_wheels=[int]$m.Groups['valid'].Value
        contacts=[int]$m.Groups['contacts'].Value
        suspension_samples=[int]$m.Groups['suspcount'].Value
        slipping=[int]$m.Groups['slipping'].Value
        skidding=[int]$m.Groups['skidding'].Value
        suspension_min=Num $m 'suspmin'
        suspension_max=Num $m 'suspmax'
        spring_force=Num $m 'spring'
        max_slip=Num $m 'slip'
        drive_torque=Num $m 'drive'
        brake_torque=Num $m 'brake'
    }
    $samples += [pscustomobject]$sample
}

if($samples.Count -lt $MinimumSamples){throw "Only $($samples.Count) Fieldmaster telemetry samples found; need at least $MinimumSamples."}
if(@($samples|Where-Object{$_.config_valid -ne 1}).Count -gt 0){throw 'Fieldmaster emitted telemetry while dedicated configuration was invalid.'}
if(@($samples|Where-Object{$_.active -eq 1}).Count -eq 0){throw 'Fieldmaster Chaos movement was never active.'}

foreach($s in $samples){
    foreach($value in @($s.rpm,$s.max_rpm,$s.speed_kmh,$s.requested_throttle,$s.throttle,$s.steering,$s.drive_health,$s.steering_grip,$s.terrain_grip,$s.suspension_min,$s.suspension_max,$s.spring_force,$s.max_slip,$s.drive_torque,$s.brake_torque)){
        if([double]::IsNaN($value) -or [double]::IsInfinity($value)){throw 'Non-finite value found in Fieldmaster Chaos telemetry.'}
    }
    if($s.valid_wheels -lt 0 -or $s.valid_wheels -gt 4 -or $s.contacts -lt 0 -or $s.contacts -gt 4 -or $s.suspension_samples -lt 0 -or $s.suspension_samples -gt 4){throw 'Invalid Fieldmaster wheel/contact counters.'}
    if($s.throttle -lt -0.001 -or $s.throttle -gt 1.001){throw 'Effective Fieldmaster throttle escaped 0..1.'}
    if([math]::Abs($s.steering) -gt 1.001){throw 'Effective Fieldmaster steering escaped -1..1.'}
    foreach($bounded in @($s.drive_health,$s.steering_grip,$s.terrain_grip)){if($bounded -lt -0.001 -or $bounded -gt 1.001){throw 'Fieldmaster drive/grip factor escaped 0..1.'}}
    if($s.suspension_samples -gt 0 -and ($s.suspension_min -lt -0.001 -or $s.suspension_max -gt 1.001 -or $s.suspension_min -gt $s.suspension_max)){throw 'Invalid normalized Fieldmaster suspension telemetry.'}
}

$driven=@($samples|Where-Object{$_.active -eq 1 -and [math]::Abs($_.requested_throttle) -ge 0.25 -and $_.throttle -ge 0.15 -and $_.gear -ne 0 -and [math]::Abs($_.speed_kmh) -ge 0.25 -and $_.rpm -gt 0 -and $_.max_rpm -gt 0})
if($driven.Count -eq 0){throw 'No telemetry sample proved live dedicated Fieldmaster drivetrain motion (gear/RPM/throttle/speed).'}
$grounded=@($samples|Where-Object{$_.valid_wheels -eq 4 -and $_.contacts -ge 2 -and $_.suspension_samples -eq 4 -and $_.spring_force -gt 0})
if($grounded.Count -eq 0){throw 'No telemetry sample proved four valid Chaos wheels with ground contact and live suspension force.'}
$torque=@($samples|Where-Object{$_.drive_torque -gt 0.1})
if($torque.Count -eq 0){throw 'No telemetry sample proved non-zero Chaos wheel drive torque.'}

$maxSpeed=($samples|ForEach-Object{[math]::Abs($_.speed_kmh)}|Measure-Object -Maximum).Maximum
$maxRpm=($samples|ForEach-Object{$_.rpm}|Measure-Object -Maximum).Maximum
$maxDrive=($samples|ForEach-Object{$_.drive_torque}|Measure-Object -Maximum).Maximum
$maxSpring=($samples|ForEach-Object{$_.spring_force}|Measure-Object -Maximum).Maximum
$minContacts=($grounded|ForEach-Object{$_.contacts}|Measure-Object -Minimum).Minimum
$gears=@($samples|ForEach-Object{$_.gear}|Sort-Object -Unique)

$evidence=[ordered]@{
    schema='gtt.fieldmaster-chaos-telemetry.v1'
    game='Grand Theft Tractor'
    result='PASS'
    version=$build.version
    platform=$build.platform
    git_sha=$build.git_sha
    sample_count=$samples.Count
    driven_sample_count=$driven.Count
    grounded_sample_count=$grounded.Count
    torque_sample_count=$torque.Count
    observed_gears=$gears
    max_abs_speed_kmh=[math]::Round([double]$maxSpeed,3)
    max_engine_rpm=[math]::Round([double]$maxRpm,1)
    max_total_drive_torque=[math]::Round([double]$maxDrive,1)
    max_total_spring_force=[math]::Round([double]$maxSpring,1)
    minimum_grounded_contacts=[int]$minContacts
    dedicated_movement='PASS'
    drivetrain='PASS'
    authored_wheels='PASS'
    suspension_runtime='PASS'
    source='GTT_RUNTIME.log'
    evaluated_utc=(Get-Date).ToUniversalTime().ToString('o')
}
$outPath=Join-Path $PackageDirectory 'FIELDMASTER_CHAOS_TELEMETRY.json'
$evidence|ConvertTo-Json -Depth 6|Set-Content -Encoding UTF8 $outPath
Write-Host "[GTT] Fieldmaster Native Chaos telemetry: PASS ($($samples.Count) samples, max speed $([math]::Round([double]$maxSpeed,2)) km/h, max drive torque $([math]::Round([double]$maxDrive,1)))."
Write-Host "[GTT] Evidence: $outPath"
