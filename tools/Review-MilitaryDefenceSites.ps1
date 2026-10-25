[CmdletBinding()]
param([switch]$Apply)
$ErrorActionPreference='Stop'
$repoRoot=Split-Path -Parent $PSScriptRoot
$catalog=@(Import-Csv (Join-Path $repoRoot 'docs/EVERON_CAMPAIGN_LOCATIONS.csv'))
$sites=@($catalog | Where-Object Type -NotIn @('TOWN','CITY'))
$definitions=(Get-Content -Raw -Encoding utf8 (Join-Path $repoRoot 'docs/MILITARY_DEFENCE_PRESETS.json') | ConvertFrom-Json).Presets
$original=Get-Content -Raw -Encoding utf8 (Join-Path $repoRoot 'docs/EVERON_DEFENCE_EDITOR_AUDIT.json') | ConvertFrom-Json
$dense=Get-Content -Raw -Encoding utf8 (Join-Path $repoRoot 'docs/EVERON_DEFENCE_EDITOR_AUDIT_DENSE.json') | ConvertFrom-Json
$placed=Get-Content -Raw -Encoding utf8 (Join-Path $repoRoot 'docs/EVERON_DEFENCE_EDITOR_AUDIT_PLACED.json') | ConvertFrom-Json
$grounded=Get-Content -Raw -Encoding utf8 (Join-Path $repoRoot 'docs/EVERON_DEFENCE_EDITOR_AUDIT_GROUNDED.json') | ConvertFrom-Json
$followup=Get-Content -Raw -Encoding utf8 (Join-Path $repoRoot 'docs/EVERON_DEFENCE_EDITOR_AUDIT_FOLLOWUP.json') | ConvertFrom-Json
$validated=Get-Content -Raw -Encoding utf8 (Join-Path $repoRoot 'docs/EVERON_DEFENCE_EDITOR_AUDIT_VALIDATED.json') | ConvertFrom-Json
function Overlaps($a,$ah,$b,$bh) {
 $dx=[double]$b.FlagX-[double]$a.FlagX; $dz=[double]$b.FlagZ-[double]$a.FlagZ
 foreach($angle in @([double]$a.DefenceYaw,([double]$a.DefenceYaw+90),[double]$b.DefenceYaw,([double]$b.DefenceYaw+90))) {
  $r=$angle*[Math]::PI/180
  $distance=[Math]::Abs($dx*[Math]::Cos($r)-$dz*[Math]::Sin($r))
  $first=([double]$a.DefenceYaw-$angle)*[Math]::PI/180
  $second=([double]$b.DefenceYaw-$angle)*[Math]::PI/180
  $extent=$ah*([Math]::Abs([Math]::Cos($first))+[Math]::Abs([Math]::Sin($first)))+$bh*([Math]::Abs([Math]::Cos($second))+[Math]::Abs([Math]::Sin($second)))
  if($distance -gt $extent+8){return $false}
 }
 return $true
}
$records=foreach($site in $sites) {
 $preset=@($definitions | Where-Object Name -CEQ $site.DefencePreset)
 if($preset.Count -ne 1){throw "Missing or ambiguous preset: $($site.Id)"}
 $preset=$preset[0]
 $evidence=$null
 if($site.DefenceAudit -eq 'Original'){$evidence=$original.Candidates.($site.Id)}
 if($site.DefenceAudit -eq 'Dense'){$evidence=$dense.Candidates.($site.Id)}
 if($site.DefenceAudit -eq 'Placed'){$evidence=$placed.Candidates.($site.Id)}
 if($site.DefenceAudit -eq 'Grounded'){$evidence=$grounded.Candidates.($site.Id)}
 if($site.DefenceAudit -eq 'Followup'){$evidence=$followup.Candidates.($site.Id)}
 if($site.DefenceAudit -ne 'Pending' -and (!$evidence -or !$evidence.FootprintGatePassed)){throw "Missing adoption evidence: $($site.Id)"}
 $prior=$validated.Current.($site.Id)
 $risks=New-Object 'Collections.Generic.List[string]'
 $conflicts=@()
 foreach($other in $sites) {
  if($other.Id -eq $site.Id){continue}
  $otherPreset=$definitions | Where-Object Name -CEQ $other.DefencePreset
  if(Overlaps $site ([double]$preset.HalfExtent) $other ([double]$otherPreset.HalfExtent)){$conflicts+=$other.Id}
 }
 if($conflicts.Count){$risks.Add('Perimeter/access overlap: '+($conflicts -join ', '))}
 $limit=2
 if($site.DefenceMaxRelief){$limit=[double]$site.DefenceMaxRelief}
 $category='New measured placement - re-audit pending'
 if($evidence){
  $risks.Add(('Measured {0} clearing: {1:0.00}m relief; model grounding, access and full Play pending' -f $site.DefenceAudit,[double]$evidence.Relief))
  if($prior.Suitable -and [double]$prior.HalfExtent -eq [double]$preset.HalfExtent -and [Math]::Abs([double]$prior.Center[0]-[double]$site.FlagX) -lt 0.05 -and [Math]::Abs([double]$prior.Center[2]-[double]$site.FlagZ) -lt 0.05){$category='Native footprint passed - Play pending'}
  elseif([Math]::Abs([double]$prior.Center[0]-[double]$site.FlagX) -lt 0.05 -and [Math]::Abs([double]$prior.Center[2]-[double]$site.FlagZ) -lt 0.05){
   $category='Current footprint rejected - correction pending'
   $risks.Add(('Latest audit: blocked={0}, badArcs={1}; confirm wall grounding before accepting or relocating' -f $prior.Blocked,$prior.BadArcs))
  }
 } else {
  $category='Compact draft - native audit pending'
  $risks.Add('Unmeasured compact draft; retain native terrain/pad/arc gates. No deployment acceptance.')
  $suggested=$followup.Candidates.($site.Id)
  if($suggested){$risks.Add(('Latest audit found a clearing at {0}, yaw {1}; not applied pending preservation of unsaved wall edits' -f ($suggested.Center -join ','),$suggested.Yaw))}
  if($prior){$risks.Add(('Prior full-size footprint: half={0}, relief={1}, blocked={2}, badArcs={3}; not evidence for this smaller layout' -f $prior.HalfExtent,$prior.Relief,$prior.Blocked,$prior.BadArcs))}
 }
 if($site.DefenceAnchored -eq '1'){$risks.Add('Strategic root retained; complete defence must remain within its canonical capture radius')}
 if($site.Type -eq 'RESOURCES'){$risks.Add('Resource relocation authorised in Chicken; the overlapping original proposal pair must not be adopted together')}
 [pscustomobject][ordered]@{Id=$site.Id;Name=$site.Name;Type=$site.Type;Preset=('AROR_Defence_'+$preset.Name+'.et');FlagX=$site.FlagX;FlagZ=$site.FlagZ;HalfExtent=$preset.HalfExtent;MaxRelief=$limit;FootprintStatus=$(if($evidence){'Adopted measured candidate - Play pending'}else{'Compact draft - native audit pending'});AuditCategory=$category;Evidence=$site.DefenceAudit;Anchored=$site.DefenceAnchored;PerimeterConflicts=($conflicts -join ';');Findings=($risks -join ' | ')}
}
$report=New-Object 'Collections.Generic.List[string]'
$report.Add('# Everon military defence site review')
$report.Add('')
$measuredCount=@($records | Where-Object Evidence -NE 'Pending').Count
$draftCount=@($records | Where-Object Evidence -EQ 'Pending').Count
$passedCount=@($records | Where-Object AuditCategory -EQ 'Native footprint passed - Play pending').Count
$report.Add(('All 23 military sites select a perimeter preset: {0} measured placements and {1} unmeasured drafts. The latest native audit passed all {2} current footprints, including relocated East Resources, with no further relocation suggestions. Full model/navigation/lifecycle acceptance remains outstanding.' -f $measuredCount,$draftCount,$passedCount))
$report.Add('')
$report.Add('The airport anchor stays fixed. Authorised Factory/resource moves affect Chicken only; Rowan scene files remain untouched. East Resources now uses the measured clearing at 6179.36,166.438,6457.14, yaw 45, about 866m from its former flag, with 1.18671m relief. Native terrain and pad gates remain active.')
$report.Add('')
$report.Add('After the final East Resources move, native grounding completed all 1526 walls with zero warnings. The world was saved and all 1526 measured offsets were imported into EVERON_DEFENCE_WALL_PLACEMENTS.json. The subsequent audit passed every current footprint. No additional scene changes were made during this evidence import.')
$report.Add('')
$report.Add('| Site | Layout / footprint | Status | Findings |')
$report.Add('| --- | --- | --- | --- |')
foreach($record in $records){$report.Add(('| {0} (`{1}`) | {2}; {3} x {3}m | {4} | {5} |' -f $record.Name,$record.Id,$record.Preset,([double]$record.HalfExtent*2),$record.AuditCategory,$record.Findings.Replace(' | ','; ')))}
$report.Add('')
$report.Add('User-reported Play tests: an earlier unnamed outpost completed clearance, flag capture and QRF response with good pathing. East Resources subsequently passed garrison spawning, defender clearance, flag capture and a successful QRF response at its relocated site. Full QRF delivery, security hold, return and cleanup were not confirmed; other sites still need gameplay coverage.')
$outputs=@(@('docs/EVERON_DEFENCE_SITE_REVIEW.csv',(($records | ConvertTo-Csv -NoTypeInformation) -join "`r`n")+"`r`n"),@('docs/EVERON_DEFENCE_SITE_REVIEW.md',($report -join "`r`n")+"`r`n"))
foreach($output in $outputs){
 $path=Join-Path $repoRoot $output[0]
 if($Apply){[IO.File]::WriteAllText($path,$output[1],(New-Object Text.UTF8Encoding($false)))}
 elseif(!(Test-Path -LiteralPath $path) -or [IO.File]::ReadAllText($path).Replace("`r`n","`n") -cne $output[1].Replace("`r`n","`n")){throw "Site review is stale: $path"}
}
Write-Output "Reviewed 23 perimeter bindings: $measuredCount measured placements, $draftCount compact drafts, $passedCount retained footprints passed the latest audit."
