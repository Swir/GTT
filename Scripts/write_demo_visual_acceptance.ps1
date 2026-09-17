param([Parameter(Mandatory=$true)][string]$VisualEvidencePath,[Parameter(Mandatory=$true)][string]$OutputPath,[Parameter(Mandatory=$true)][string]$ExpectedGitSha,[Parameter(Mandatory=$true)][string]$Reviewer,[Parameter(Mandatory=$true)][string]$Notes,[switch]$Approved)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$VisualEvidencePath=[IO.Path]::GetFullPath($VisualEvidencePath);$OutputPath=[IO.Path]::GetFullPath($OutputPath)
if(-not $Approved){throw 'Visual acceptance cannot be written without explicit -Approved attestation.'}
if(-not(Test-Path $VisualEvidencePath -PathType Leaf)){throw "Visual evidence manifest missing: $VisualEvidencePath"}
if(-not $ExpectedGitSha -or $ExpectedGitSha.Length -lt 7){throw 'ExpectedGitSha is required.'}
if(-not $Reviewer.Trim()){throw 'Reviewer identity is required.'}
if($Notes.Trim().Length -lt 20){throw 'Visual review notes must contain at least 20 characters.'}
$visual=Get-Content -Raw $VisualEvidencePath|ConvertFrom-Json
if($visual.schema -ne 'gtt.demo-visual-evidence.v1'){throw "Visual evidence schema mismatch: $($visual.schema)"}
if($visual.result -ne 'PASS'){throw 'Rendered visual evidence did not PASS.'}
if($visual.git_sha -ne $ExpectedGitSha){throw "Visual evidence SHA mismatch: evidence=$($visual.git_sha), expected=$ExpectedGitSha"}
if(-not $visual.human_review_required){throw 'Visual evidence manifest does not require human review as expected.'}
if([int]$visual.scene_count -lt 5 -or $visual.screenshots.Count -lt 5){throw 'At least five reviewed rendered scenes are required.'}
$evidenceHash=(Get-FileHash -Algorithm SHA256 -Path $VisualEvidencePath).Hash.ToLowerInvariant();$screenshotHashes=@()
foreach($shot in $visual.screenshots){if(-not $shot.sha256){throw "Screenshot hash missing for scene $($shot.scene)."};$screenshotHashes += [ordered]@{scene=$shot.scene;sha256=$shot.sha256}}
$parent=Split-Path -Parent $OutputPath;if($parent){New-Item -ItemType Directory -Path $parent -Force|Out-Null}
$acceptance=[ordered]@{schema='gtt.demo-visual-acceptance.v1';game='Grand Theft Tractor';result='PASS';git_sha=$ExpectedGitSha;version=$visual.version;reviewer=$Reviewer.Trim();notes=$Notes.Trim();reviewed_utc=(Get-Date).ToUniversalTime().ToString('o');visual_evidence_manifest_sha256=$evidenceHash;screenshot_hashes=$screenshotHashes;exact_candidate_review=$true}
$acceptance|ConvertTo-Json -Depth 8|Set-Content -Encoding UTF8 $OutputPath
Write-Host "[GTT] Human visual acceptance attestation written for exact candidate $ExpectedGitSha."
