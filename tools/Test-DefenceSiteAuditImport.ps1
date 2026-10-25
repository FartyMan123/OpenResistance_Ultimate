param()
$ErrorActionPreference='Stop'
$importer=Join-Path $PSScriptRoot 'Import-DefenceSiteAudit.ps1'
$repoRoot=Split-Path -Parent $PSScriptRoot
$fixturePath=Join-Path $repoRoot ('.defence-audit-fixture-'+[guid]::NewGuid().ToString('N')+'.log')
$assertions=0
function Check([bool]$ok,[string]$message) {
 $script:assertions++
 if (!$ok) { throw $message }
}
try {
 # Synthetic parser fixtures, never native evidence or an adoption archive.
 $lines=@()
 for($i=0;$i -lt 23;$i++) {
  $lines+='SCRIPT : [AROR DEFENCE AUDIT] id=everon_fixture_'+$i+' current=<1,10,1> half=18 relief=3 blocked=0 badArcs=0 overlap=0 suitable=0'
 }
 $lines+='SCRIPT : [AROR DEFENCE AUDIT] id=everon_fixture_0 proposed=<26,10,1> yaw=45 half=18 relief=1 candidate=1; fixture only'
 $lines+='SCRIPT : [AROR DEFENCE AUDIT] current suitable=0 suggested relocations=1 reviewed=23. Fixture only.'
 $lines+='SCRIPT : [AROR DEFENCE AUDIT] id=everon_outpost_andres_beacon current=<1,10,1> half=18 relief=4.24518 blocked=0 badArcs=0 overlap=0 suitable=0'
 $lines+='SCRIPT : [AROR DEFENCE AUDIT] id=everon_outpost_andres_beacon proposed=<26,10,26> yaw=45 half=18 relief=1 candidate=55; fixture only'
 $lines+='SCRIPT : [AROR DEFENCE AUDIT] current suitable=0 suggested relocations=1 reviewed=1. Fixture only.'
 [IO.File]::WriteAllText($fixturePath,($lines -join "`r`n"),(New-Object Text.UTF8Encoding($false)))
 $full=@(& $importer -LogPath $fixturePath)
 $archive=$full[0] | ConvertFrom-Json
 Check (@($archive.Current.PSObject.Properties).Count -eq 23) 'Full import must ignore a later targeted summary'
 Check (@($archive.Candidates.PSObject.Properties).Count -eq 1) 'Full import candidate count'
 $target=@(& $importer -LogPath $fixturePath -ExpectedSites 1 -OutputFile 'docs/EVERON_DEFENCE_ANDRES_AUDIT.json')
 $archive=$target[0] | ConvertFrom-Json
 Check (@($archive.Current.PSObject.Properties).Count -eq 1) 'Target import must contain only its current site'
 Check ($archive.Candidates.everon_outpost_andres_beacon.Candidate -eq 55) 'Target import must use its own proposal'
 Check (!$archive.Candidates.everon_fixture_0) 'Target import must not inherit an earlier island proposal'
 $rejected=$false
 try { & $importer -LogPath $fixturePath -ExpectedSites 1 | Out-Null } catch { $rejected=$_.Exception.Message -like '*separate OutputFile*' }
 Check $rejected 'Target import must protect the island adoption archive'
 [IO.File]::WriteAllText($fixturePath,(($lines | Select-Object -First 22) -join "`r`n")+"`r`n"+$lines[24],(New-Object Text.UTF8Encoding($false)))
 $rejected=$false
 try { & $importer -LogPath $fixturePath | Out-Null } catch { $rejected=$_.Exception.Message -like '*Incomplete current-site*' }
 Check $rejected 'Incomplete island audit must reject'
 $dense=@('SCRIPT : [AROR DEFENCE AUDIT] begin reviewed=23 grid=25 radius=800 orientations=8; fixture only')
 for($i=0;$i -lt 23;$i++) {
  $limit=2
  if($i -eq 0) { $limit=3 }
  $dense+='SCRIPT : [AROR DEFENCE AUDIT] id=everon_dense_'+$i+' current=<1,10,1> half=18 relief=3 blocked=0 badArcs=0 overlap=0 suitable=0 limit='+$limit
 }
 $dense+='SCRIPT : [AROR DEFENCE AUDIT] id=everon_dense_0 proposed=<26,10,1> yaw=45 half=18 relief=2.7 candidate=1; fixture only'
 $dense+='SCRIPT : [AROR DEFENCE AUDIT] current suitable=0 suggested relocations=1 reviewed=23. Fixture only.'
 [IO.File]::WriteAllText($fixturePath,(($lines+$dense) -join "`r`n"),(New-Object Text.UTF8Encoding($false)))
 $full=@(& $importer -LogPath $fixturePath)
 $archive=$full[0] | ConvertFrom-Json
 Check ($archive.Current.everon_dense_0.MaxRelief -eq 3 -and $archive.Candidates.everon_dense_0.Relief -eq 2.7) 'Dense import must honour recorded site-specific allowance'
 Check (!$archive.Current.everon_fixture_0 -and @($archive.Current.PSObject.Properties).Count -eq 23) 'Dense run boundary must exclude earlier full-run records'
 $invalid=($dense -join "`r`n").Replace('relief=2.7 candidate','relief=3.1 candidate')
 [IO.File]::WriteAllText($fixturePath,$invalid,(New-Object Text.UTF8Encoding($false)))
 $rejected=$false
 try { & $importer -LogPath $fixturePath | Out-Null } catch { $rejected=$_.Exception.Message -like '*exceeds the terrain limit*' }
 Check $rejected 'Dense candidate above its own configured allowance must reject'
 $partial=@($dense | Select-Object -First 23)+$dense[25]
 [IO.File]::WriteAllText($fixturePath,(($lines+$partial) -join "`r`n"),(New-Object Text.UTF8Encoding($false)))
 $rejected=$false
 try { & $importer -LogPath $fixturePath | Out-Null } catch { $rejected=$_.Exception.Message -like '*Incomplete current-site*' }
 Check $rejected 'Incomplete dense run must not borrow sites from the previous completed audit'
 Write-Output "$assertions audit-import fixture checks pass. No placement evidence was created."
} finally {
 if(Test-Path -LiteralPath $fixturePath) { Remove-Item -LiteralPath $fixturePath }
}
