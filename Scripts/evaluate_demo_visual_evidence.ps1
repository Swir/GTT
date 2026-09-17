param([Parameter(Mandatory=$true)][string]$PackageDirectory,[string]$RuntimeLog="",[string]$ExpectedGitSha=$env:GITHUB_SHA)
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest
$PackageDirectory=[IO.Path]::GetFullPath($PackageDirectory)
if(-not $RuntimeLog){$RuntimeLog=Join-Path $PackageDirectory 'GTT_VISUAL_RUNTIME.log'}
$RuntimeLog=[IO.Path]::GetFullPath($RuntimeLog)
$outputPath=Join-Path $PackageDirectory 'DEMO_VISUAL_EVIDENCE.json'
$visualRuntimePath=Join-Path $PackageDirectory 'VISUAL_RUNTIME_SMOKE.json'
$visualDirectory=Join-Path $PackageDirectory 'DemoVisualEvidence'
$expectedScenes=@('world_gameplay','law_pressure','native_vehicle','loaded_trailer','hud_overview')
$blockers=New-Object System.Collections.Generic.List[string]
$records=New-Object System.Collections.Generic.List[object]
function Add-Blocker([string]$Text){if(-not $blockers.Contains($Text)){[void]$blockers.Add($Text)}}
if(-not(Test-Path $visualRuntimePath -PathType Leaf)){Add-Blocker 'VISUAL_RUNTIME_SMOKE.json missing.'}
if(-not(Test-Path $RuntimeLog -PathType Leaf)){Add-Blocker 'Rendered runtime log missing.'}
if(-not(Test-Path $visualDirectory -PathType Container)){Add-Blocker 'DemoVisualEvidence directory missing.'}
$visualRuntime=$null
if(Test-Path $visualRuntimePath -PathType Leaf){try{$visualRuntime=Get-Content -Raw $visualRuntimePath|ConvertFrom-Json}catch{Add-Blocker 'VISUAL_RUNTIME_SMOKE.json is not valid JSON.'}}
if($visualRuntime){if($visualRuntime.schema -ne 'gtt.visual-runtime-smoke.v1'){Add-Blocker 'Visual runtime schema mismatch.'};if($visualRuntime.result -ne 'PASS'){Add-Blocker 'Rendered visual runtime did not PASS.'};if($visualRuntime.null_rhi -ne $false){Add-Blocker 'Visual runtime used NullRHI; rendered acceptance evidence is invalid.'};if($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown' -and $visualRuntime.git_sha -ne $ExpectedGitSha){Add-Blocker 'Visual runtime SHA mismatch.'};if([int]$visualRuntime.survived_seconds -lt 178){Add-Blocker 'Visual runtime ended before the final deterministic capture window.'}}
$log=''
if(Test-Path $RuntimeLog -PathType Leaf){$log=Get-Content -Raw $RuntimeLog;if($log -match 'DEMO_VISUAL_CAPTURE_FAIL'){Add-Blocker 'Runtime reported DEMO_VISUAL_CAPTURE_FAIL.'};if($log -notmatch 'DEMO_VISUAL_CAPTURE_PLAN\s+scenes=5'){Add-Blocker 'Visual capture plan marker missing.'};if($log -notmatch 'DEMO_VISUAL_CAPTURE_COMPLETE\s+scenes=5'){Add-Blocker 'Visual capture completion marker missing.'}}
$systemDrawingReady=$false
try{Add-Type -AssemblyName System.Drawing -ErrorAction Stop;$systemDrawingReady=$true}catch{Add-Blocker 'System.Drawing unavailable; screenshot pixels cannot be validated.'}
$hashes=New-Object System.Collections.Generic.HashSet[string]
foreach($scene in $expectedScenes){
    $path=Join-Path $visualDirectory "GTT_visual_$scene.png"
    if($log -and $log -notmatch ("DEMO_VISUAL_CAPTURE_WRITTEN\s+scene="+[regex]::Escape($scene))){Add-Blocker "Runtime write marker missing for scene '$scene'."}
    if(-not(Test-Path $path -PathType Leaf)){Add-Blocker "Screenshot missing for scene '$scene'.";continue}
    $item=Get-Item $path;$hash=(Get-FileHash -Algorithm SHA256 -Path $path).Hash.ToLowerInvariant();[void]$hashes.Add($hash)
    $width=0;$height=0;$minLuma=255.0;$maxLuma=0.0;$meanLuma=0.0;$sampleCount=0;$bins=New-Object System.Collections.Generic.HashSet[string]
    if($item.Length -lt 20000){Add-Blocker "Screenshot '$scene' is suspiciously small ($($item.Length) bytes)."}
    if($systemDrawingReady){$bitmap=$null;try{$bitmap=[System.Drawing.Bitmap]::new($path);$width=$bitmap.Width;$height=$bitmap.Height;if($width -lt 1280 -or $height -lt 720){Add-Blocker "Screenshot '$scene' is below 1280x720 ($width x $height)."};$stepX=[Math]::Max(1,[int]($width/24));$stepY=[Math]::Max(1,[int]($height/14));for($y=[int]($stepY/2);$y -lt $height;$y+=$stepY){for($x=[int]($stepX/2);$x -lt $width;$x+=$stepX){$c=$bitmap.GetPixel($x,$y);$luma=0.2126*$c.R+0.7152*$c.G+0.0722*$c.B;$minLuma=[Math]::Min($minLuma,$luma);$maxLuma=[Math]::Max($maxLuma,$luma);$meanLuma+=$luma;$sampleCount++;$bin="{0}-{1}-{2}" -f [int]($c.R/32),[int]($c.G/32),[int]($c.B/32);[void]$bins.Add($bin)}};if($sampleCount -gt 0){$meanLuma/=$sampleCount};if(($maxLuma-$minLuma)-lt 24.0){Add-Blocker "Screenshot '$scene' is visually flat (luminance range < 24)."};if($meanLuma -lt 4.0){Add-Blocker "Screenshot '$scene' is effectively black."};if($meanLuma -gt 251.0){Add-Blocker "Screenshot '$scene' is effectively white."};if($bins.Count -lt 12){Add-Blocker "Screenshot '$scene' has insufficient sampled color variation."}}catch{Add-Blocker "Screenshot '$scene' could not be decoded as a rendered image: $($_.Exception.Message)"}finally{if($bitmap){$bitmap.Dispose()}}}
    [void]$records.Add([ordered]@{scene=$scene;file="DemoVisualEvidence/GTT_visual_$scene.png";bytes=$item.Length;width=$width;height=$height;sha256=$hash;sampled_luminance_min=[Math]::Round($minLuma,2);sampled_luminance_max=[Math]::Round($maxLuma,2);sampled_luminance_mean=[Math]::Round($meanLuma,2);sampled_color_bins=$bins.Count})
}
if($records.Count -ne $expectedScenes.Count){Add-Blocker "Expected five screenshot records, found $($records.Count)."}
if($hashes.Count -lt 4){Add-Blocker 'Visual evidence does not contain enough distinct rendered frames.'}
$result=if($blockers.Count -eq 0){'PASS'}else{'FAIL'}
$manifest=[ordered]@{schema='gtt.demo-visual-evidence.v1';game='Grand Theft Tractor';result=$result;git_sha=if($visualRuntime){$visualRuntime.git_sha}else{$ExpectedGitSha};version=if($visualRuntime){$visualRuntime.version}else{'unknown'};rendered_runtime=if($visualRuntime -and $visualRuntime.result -eq 'PASS'){'PASS'}else{'FAIL'};null_rhi=if($visualRuntime){$visualRuntime.null_rhi}else{$null};expected_resolution='1280x720';scene_count=$records.Count;unique_frame_hashes=$hashes.Count;screenshots=$records;human_review_required=$true;human_visual_acceptance='NOT_PERFORMED';blockers=$blockers;evaluated_utc=(Get-Date).ToUniversalTime().ToString('o')}
$manifest|ConvertTo-Json -Depth 8|Set-Content -Encoding UTF8 $outputPath
if($result -ne 'PASS'){Write-Host '[GTT] Demo visual evidence gate: FAIL';foreach($blocker in $blockers){Write-Host " - $blocker"};exit 1}
Write-Host '[GTT] Demo visual evidence gate: PASS (5 rendered scenes; human aesthetic review still required).'
