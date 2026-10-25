[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$LogPath,[switch]$Apply)
$ErrorActionPreference='Stop'
$repoRoot=Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'CampaignGroundingData.ps1')
$layerPath=Join-Path $repoRoot 'AROpenResistance/Open Resistance/Worlds/AROR_Everon/AROR_Everon_Chicken_Layers/MilitaryInstallations.layer'
$saved=Read-CampaignLayerLocations ([IO.File]::ReadAllText($layerPath))
$rows=@(Import-Csv (Join-Path $repoRoot 'docs/EVERON_CAMPAIGN_LOCATIONS.csv') | Where-Object Type -NotIn @('TOWN','CITY'))
if($saved.Count -ne $rows.Count){throw 'Saved scene does not contain the complete military catalogue'}
$stream=[IO.File]::Open($LogPath,[IO.FileMode]::Open,[IO.FileAccess]::Read,([IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete))
$reader=New-Object IO.StreamReader($stream)
try{$log=$reader.ReadToEnd()}finally{$reader.Dispose()}
$summaries=[regex]::Matches($log,'(?m)^([^\r\n]*\[AROR DEFENCE GROUND\] found=(\d+) grounded=(\d+) warnings=(\d+)[^\r\n]*)')
if(!$summaries.Count){throw 'No completed grounding summary'}
$summary=$summaries[$summaries.Count-1]
$entries=@([regex]::Matches($log.Substring(0,$summary.Index),'\[AROR DEFENCE GROUND\] id=(everon_\w+) source=\S+ local=<([^>]+)> terrain=([\d.eE+\-]+)') | Select-Object -Last ([int]$summary.Groups[3].Value))
if($entries.Count -ne [int]$summary.Groups[3].Value){throw 'Incomplete grounding log'}
$measurements=@{}
foreach($entry in $entries){$id=$entry.Groups[1].Value;if(!$measurements.ContainsKey($id)){$measurements[$id]=@()};$measurements[$id]+=,@($entry.Groups[2].Value.Split(',') | ForEach-Object {[double]$_})}
$sites=[ordered]@{};$wallCount=0;$matchedCount=0
foreach($row in $rows){
 $site=$saved[$row.Id]
 if(!$site){throw "Missing saved site: $($row.Id)"}
 $root=@([double]$row.X,[double]$row.Y,[double]$row.Z)
 foreach($axis in 0..2){if([Math]::Abs($site.Origin[$axis]-$root[$axis]) -gt 0.002){throw "Saved root differs from catalogue: $($row.Id)"}}
 $yaw=[double]$row.DefenceYaw
 if([Math]::Abs((($site.Yaw-$yaw+540)%360)-180) -gt 0.001){throw "Saved yaw differs from catalogue: $($row.Id)"}
 $r=$yaw*[Math]::PI/180;$c=[Math]::Cos($r);$s=[Math]::Sin($r)
 $dx=[double]$row.FlagX-$root[0];$dz=[double]$row.FlagZ-$root[2]
 $delta=@(($dx*$c-$dz*$s),([double]$row.FlagY-0.905946-$root[1]),($dx*$s+$dz*$c))
 $prefab=Join-Path $repoRoot ('AROpenResistance/Open Resistance/Prefabs/Compositions/Locations/AROR_Defence_'+$row.DefencePreset+'.et')
 $walls=[ordered]@{};$unmeasured=@()
 foreach($wall in [regex]::Matches([IO.File]::ReadAllText($prefab),'(?ms)^  GenericEntity AROR_\w+_Wall_[^\r\n]+\r?\n(.*?)^  \}')){
  $id=[regex]::Match($wall.Groups[1].Value,'ID "([A-F0-9]{16})"').Groups[1].Value
  $p=[regex]::Match($wall.Groups[1].Value,'(?m)^   coords ([\d.\-]+) ([\d.\-]+) ([\d.\-]+)')
  $expected=@(1..3 | ForEach-Object {[double]$p.Groups[$_].Value+$delta[$_-1]})
  $position=$expected
  if($site.Children.ContainsKey($id) -and $null -ne $site.Children[$id]){$position=$site.Children[$id]}
  if([Math]::Abs($position[0]-$expected[0]) -gt 0.003 -or [Math]::Abs($position[2]-$expected[2]) -gt 0.003){throw "Unexpected horizontal wall edit: $($row.Id)/$id"}
  $matches=@($measurements[$row.Id] | Where-Object {[Math]::Abs($_[0]-$position[0]) -lt 0.02 -and [Math]::Abs($_[2]-$position[2]) -lt 0.02})
  if($matches.Count -gt 1){throw "Ambiguous grounding measurement: $($row.Id)/$id"}
  if($matches.Count -eq 1){
   if([Math]::Abs($matches[0][1]-$position[1]) -gt 0.002){throw "Grounded wall edit not saved: $($row.Id)/$id"}
   $matchedCount++
  }else{$unmeasured+=$id}
  # Convert saved rounded root coordinates into canonical catalogue-local space.
  $wx=$site.Origin[0]+$position[0]*$c+$position[2]*$s
  $wz=$site.Origin[2]-$position[0]*$s+$position[2]*$c
  $walls[$id]=@([Math]::Round((($wx-$root[0])*$c-($wz-$root[2])*$s),6),[Math]::Round(($site.Origin[1]+$position[1]-$root[1]),6),[Math]::Round((($wx-$root[0])*$s+($wz-$root[2])*$c),6))
  $wallCount++
 }
 $sites[$row.Id]=[ordered]@{Pose=(Get-DefencePoseKey $row);Walls=$walls;Unmeasured=$unmeasured}
}
if($wallCount -ne [int]$summary.Groups[2].Value -or $matchedCount -ne [int]$summary.Groups[3].Value){throw 'Saved wall coverage differs from completed grounding log'}
$archive=[ordered]@{Version=1;Source=((Split-Path -Leaf (Split-Path -Parent $LogPath))+'/script.log');Summary=$summary.Groups[1].Value;Note='Saved editor wall positions. Unmeasured walls are retained source geometry, not accepted terrain measurements. Poses invalidate offsets after relocation or rotation.';Sites=$sites}
$output=($archive | ConvertTo-Json -Depth 8)+"`r`n"
if($Apply){[IO.File]::WriteAllText((Join-Path $repoRoot 'docs/EVERON_DEFENCE_WALL_PLACEMENTS.json'),$output,(New-Object Text.UTF8Encoding($false)))}
Write-Output "Reconciled $wallCount saved walls at $($sites.Count) sites: $matchedCount measured, $($wallCount-$matchedCount) unmeasured. No scene files changed."
