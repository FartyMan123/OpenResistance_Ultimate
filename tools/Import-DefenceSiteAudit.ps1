[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$LogPath,[string]$OutputFile='docs/EVERON_DEFENCE_EDITOR_AUDIT.json',[ValidateSet(1,23)][int]$ExpectedSites=23,[switch]$Apply)
$ErrorActionPreference='Stop'
$repoRoot=Split-Path -Parent $PSScriptRoot
$stream=[IO.File]::Open($LogPath,[IO.FileMode]::Open,[IO.FileAccess]::Read,([IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete))
$reader=New-Object IO.StreamReader($stream)
try { $log=$reader.ReadToEnd() } finally { $reader.Dispose() }
$summaries=[regex]::Matches($log,('(?m)^([^\r\n]*\[AROR DEFENCE AUDIT\] current suitable=(\d+) suggested relocations=(\d+) reviewed='+$ExpectedSites+'\.[^\r\n]*)'))
if (!$summaries.Count) { throw "No completed $ExpectedSites-site defence audit found" }
$summary=$summaries[$summaries.Count-1]
$before=$log.Substring(0,$summary.Index)
$starts=[regex]::Matches($before,('(?m)^([^\r\n]*\[AROR DEFENCE AUDIT\] begin reviewed='+$ExpectedSites+'\b[^\r\n]*)'))
# New runs explicitly delimit their own records; incomplete reruns must never
# borrow measurements from a previous completed run. Keep legacy log support.
if ($starts.Count) { $runStart=$starts[$starts.Count-1].Index } else { $runStart=0 }
$before=$before.Substring($runStart)
$current=[regex]::Matches($before,'(?m)^([^\r\n]*\[AROR DEFENCE AUDIT\] id=(everon_\w+) current=<([^>]+)> half=([\d.]+) relief=([\d.eE+\-]+) blocked=(\d+) badArcs=(\d+) overlap=([01]) suitable=([01])[^\r\n]*)')
if ($current.Count -lt $ExpectedSites) { throw 'Incomplete current-site records' }
$current=@($current | Select-Object -Last $ExpectedSites)
if (@($current | ForEach-Object {$_.Groups[2].Value} | Select-Object -Unique).Count -ne $ExpectedSites) { throw 'Audit contains duplicate military IDs' }
if ($ExpectedSites -eq 1 -and $OutputFile -eq 'docs/EVERON_DEFENCE_EDITOR_AUDIT.json') { throw 'Use a separate OutputFile for a targeted audit; preserve the island-wide adoption evidence' }
$run=$log.Substring($runStart+$current[0].Index,$summary.Index-$runStart-$current[0].Index)
function ParseNumber([string]$value) { [double]::Parse($value,[Globalization.CultureInfo]::InvariantCulture) }
function ParseVector([string]$value) { @($value.Split(',') | ForEach-Object { ParseNumber $_ }) }
$sites=[ordered]@{}
foreach ($entry in $current) {
 $limit=2
 $configured=[regex]::Match($entry.Groups[1].Value,'\blimit=([\d.]+)')
 if ($configured.Success) { $limit=ParseNumber $configured.Groups[1].Value }
 if ($limit -lt 0.25 -or $limit -gt 4) { throw 'Invalid recorded terrain allowance' }
 $sites[$entry.Groups[2].Value]=[ordered]@{ Center=(ParseVector $entry.Groups[3].Value); HalfExtent=(ParseNumber $entry.Groups[4].Value); Relief=(ParseNumber $entry.Groups[5].Value); MaxRelief=$limit; Blocked=[int]$entry.Groups[6].Value; BadArcs=[int]$entry.Groups[7].Value; Overlap=($entry.Groups[8].Value -eq '1'); Suitable=($entry.Groups[9].Value -eq '1'); SourceLine=$entry.Groups[1].Value }
}
$candidates=[ordered]@{}
foreach ($entry in [regex]::Matches($run,'(?m)^([^\r\n]*\[AROR DEFENCE AUDIT\] id=(everon_\w+) proposed=<([^>]+)> yaw=([\d.\-]+) half=([\d.]+) relief=([\d.eE+\-]+) candidate=(\d+)[^\r\n]*)')) {
 $id=$entry.Groups[2].Value
 if (!$sites.Contains($id) -or $candidates.Contains($id)) { throw "Invalid duplicate candidate: $id" }
 $relief=ParseNumber $entry.Groups[6].Value
 if ($relief -gt $sites[$id].MaxRelief) { throw "Candidate exceeds the terrain limit: $id" }
 $candidates[$id]=[ordered]@{ Center=(ParseVector $entry.Groups[3].Value); Yaw=(ParseNumber $entry.Groups[4].Value); HalfExtent=(ParseNumber $entry.Groups[5].Value); Relief=$relief; Candidate=[int]$entry.Groups[7].Value; FootprintGatePassed=$true; VisualAndPlayAccepted=$false; SourceLine=$entry.Groups[1].Value }
}
if ($candidates.Count -ne [int]$summary.Groups[3].Value) { throw 'Candidate count differs from audit summary' }
$archive=[ordered]@{ Version=1; Source=((Split-Path -Leaf (Split-Path -Parent $LogPath))+'/'+(Split-Path -Leaf $LogPath)); Method='Completed native read-only defence footprint audit. Proposed centres passed terrain, wall strips, role pads, MG rays and conservative neighbour checks. Candidates are independent; actual models, driving/navmesh and visual acceptance remain unverified. Obstruction counts are not measured when terrain fails early.'; Summary=$summary.Groups[1].Value; Current=$sites; Candidates=$candidates }
$output=($archive | ConvertTo-Json -Depth 8)+"`r`n"
$path=Join-Path $repoRoot $OutputFile
if ($Apply) { [IO.File]::WriteAllText($path,$output,(New-Object Text.UTF8Encoding($false))) }
else { Write-Output $output }
Write-Output ('Completed audit: '+$sites.Count+' sites, '+$candidates.Count+' measured relocation candidates; visual/Play acceptance pending.')
