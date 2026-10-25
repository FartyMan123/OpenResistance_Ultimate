param()
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'Set-MilitaryDefencePresets.ps1')
& (Join-Path $PSScriptRoot 'Review-MilitaryDefenceSites.ps1')
$repoRoot = Split-Path -Parent $PSScriptRoot
$definition = Get-Content -Encoding utf8 -Raw (Join-Path $repoRoot 'docs/MILITARY_DEFENCE_PRESETS.json') | ConvertFrom-Json
$addonRoot = Join-Path $repoRoot 'AROpenResistance/Open Resistance'
$allGuids = @()
foreach ($preset in $definition.Presets) {
 $path = 'Prefabs/Compositions/Locations/AROR_Defence_'+$preset.Name+'.et'
 $text = [IO.File]::ReadAllText((Join-Path $addonRoot $path))
 $meta = [IO.File]::ReadAllText((Join-Path $addonRoot ($path+'.meta')))
 if (!$meta.Contains('{'+$preset.Guid+'}'+$path) -or $allGuids -contains $preset.Guid) { throw 'Invalid/duplicate preset resource GUID' }
 $allGuids += $preset.Guid
 $ids = @([regex]::Matches($text,'(?m)^\s+ID "([A-F0-9]{16})"') | ForEach-Object { $_.Groups[1].Value })
 if (@($ids | Select-Object -Unique).Count -ne $ids.Count) { throw "Duplicate entity ID: $path" }
 $positions = @([regex]::Matches($text,'(?m)^  GenericEntity AROR_\w+_Defence_(\d+)_(\d+) \{'))
 if (!$positions.Count -or !$text.Contains($definition.WallResource)) { throw "Missing markers or walls: $path" }
 if ($preset.SourceName -eq 'Outpost') {
  if ($positions.Count -ne 3 -or @($positions | Where-Object { $_.Groups[1].Value -in @('2','3') }).Count) { throw 'Outpost must contain two infantry positions, one MG, no hull positions' }
 }
 $pads=@()
 foreach ($position in $positions) {
  $block = $text.Substring($position.Index)
  $coords = [regex]::Match($block,'(?m)^   coords ([\d.\-]+) 0 ([\d.\-]+)')
  if (!$coords.Success -or [Math]::Abs([double]$coords.Groups[1].Value) -gt ([double]$preset.HalfExtent-2) -or [Math]::Abs([double]$coords.Groups[2].Value) -gt ([double]$preset.HalfExtent-2)) { throw "Defence marker outside wall clearance: $path" }
  $pads+=[pscustomobject]@{Kind=[int]$position.Groups[1].Value;X=[double]$coords.Groups[1].Value;Z=[double]$coords.Groups[2].Value}
 }
 for($a=0;$a -lt $pads.Count;$a++) { for($b=$a+1;$b -lt $pads.Count;$b++) {
  if($pads[$a].Kind -eq 0 -and $pads[$b].Kind -eq 0){continue}
  $minimum=12
  if($pads[$a].Kind -eq 0 -or $pads[$b].Kind -eq 0){$minimum=7}
  $distance=[Math]::Sqrt([Math]::Pow(($pads[$a].X-$pads[$b].X),2)+[Math]::Pow(($pads[$a].Z-$pads[$b].Z),2))
  if($distance -lt $minimum){throw "Role pads conflict with runtime clearance in $path"}
 } }
 $walls = [regex]::Matches($text,'(?ms)^  GenericEntity AROR_\w+_Wall_(\d+)_\d+[^\r\n]+\r?\n(.*?)^  \}')
 if ($walls.Count -lt 20) { throw "Missing physical perimeter wall children: $path" }
 foreach ($wall in $walls) {
  $wallBody=$wall.Groups[2].Value
  if ($wallBody -notmatch '(?m)^    Hierarchy \{' -or $wallBody -notmatch '(?m)^    AROR_DefenceWallComponent \{') { throw "Detached or unidentifiable wall child in $path" }
  $wallCoords=[regex]::Match($wallBody,'(?m)^   coords ([\d.\-]+) 0 ([\d.\-]+)')
  if (!$wallCoords.Success) { throw "Missing wall transform in $path" }
  if ($wall.Groups[1].Value -eq '2' -and [Math]::Abs([double]$wallCoords.Groups[1].Value) -lt 6.25) { throw "South entrance blocked in $path" }
 }
}
$review = @(Import-Csv (Join-Path $repoRoot 'docs/EVERON_DEFENCE_SITE_REVIEW.csv'))
if ($review.Count -ne 23 -or @($review | Where-Object FootprintStatus -EQ 'Adopted measured candidate - Play pending').Count -ne 23 -or @($review | Where-Object FootprintStatus -EQ 'Compact draft - native audit pending').Count -ne 0) { throw 'All 23 military sites must have measured placement evidence' }
if (@($review | Where-Object PerimeterConflicts).Count) { throw 'Perimeters or access gaps overlap in the authored plan' }
if(@($review | Where-Object AuditCategory -EQ 'Native footprint passed - Play pending').Count -ne 23){throw 'Preserve the latest native footprint evidence for all 23 sites'}
Write-Output 'Eleven presets and all 23 measured site bindings pass; all 23 native footprints passed. Navigation and full Play acceptance remain outstanding.'
