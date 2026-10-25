param([string]$ResourceDatabasePath = 'D:/SteamLibrary/steamapps/common/Arma Reforger/addons/data/resourceDatabase.rdb')
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$addonRoot = Join-Path $repoRoot 'AROpenResistance/Open Resistance'
$bytes = [IO.File]::ReadAllBytes($ResourceDatabasePath)
$database = [Text.Encoding]::GetEncoding(28591).GetString($bytes)
$paths = @('Prefabs/GameModes/AROR_GameMode_Base.et','Prefabs/Groups/AROR_GarrisonGroup.et')
$paths += @(Get-ChildItem -LiteralPath (Join-Path $addonRoot 'Prefabs/Compositions/Locations') -Filter 'AROR_Defence_*.et' | ForEach-Object { $_.FullName.Substring($addonRoot.Length + 1) })
$paths += @(Get-ChildItem -LiteralPath (Join-Path $addonRoot 'Scripts/Game/Garrisons') -Filter '*.c' | ForEach-Object { $_.FullName.Substring($addonRoot.Length + 1) })
$references = @{}
foreach ($path in $paths) {
 $text = [IO.File]::ReadAllText((Join-Path $addonRoot $path))
 foreach ($match in [regex]::Matches($text,'\{([A-F0-9]{16})\}(Prefabs/[^"\r\n]+\.et)')) { $references[$match.Groups[2].Value] = $match.Groups[1].Value }
}
$checked = 0
foreach ($path in $references.Keys) {
 $nativeIndex = $database.IndexOf($path + [char]0)
 if ($nativeIndex -ge 0) {
  $guid = [BitConverter]::ToUInt64($bytes,$nativeIndex + $path.Length + 7).ToString('X16')
  if ($guid -cne $references[$path]) { throw "Installed GUID mismatch: $path" }
 } else {
  $metaPath = Join-Path $addonRoot ($path + '.meta')
  if (!(Test-Path -LiteralPath $metaPath) -or ![IO.File]::ReadAllText($metaPath).Contains('{' + $references[$path] + '}' + $path)) { throw "Missing native asset or mismatched local metadata: $path" }
 }
 $checked++
}
foreach ($name in @('Airport','Factory','Port','Resources','MilitaryBase')) {
 if (![IO.File]::ReadAllText((Join-Path $addonRoot "Prefabs/Locations/AROR_Location_$name.et")).Contains('AROR_GarrisonComponent')) { throw "Missing military garrison: $name" }
}
foreach ($name in @('Town','City')) {
 $text = [IO.File]::ReadAllText((Join-Path $addonRoot "Prefabs/Locations/AROR_Location_$name.et"))
 if ($text.Contains('AROR_GarrisonComponent') -or $text.Contains('AROR_Prop_CaptureFlag.et')) { throw "Settlement/HQ must have no military garrison or pole: $name" }
}
$hq = [IO.File]::ReadAllText((Join-Path $addonRoot 'Prefabs/Locations/AROR_Location_HQ.et'))
if ($hq.Contains('AROR_GarrisonComponent') -or !$hq.Contains('m_bControlFixed 1')) { throw 'HQ must remain fixed and excluded from generated defences' }
$regional = [IO.File]::ReadAllText((Join-Path $addonRoot 'Scripts/Game/Settlements/AROR_RegionalSupportComponent.c'))
if ($regional -notmatch '\[Attribute\("0",[^\r\n]*\)\]\s+protected bool m_bRegionalSupport;') { throw 'Regional support must default off in the script' }
$sharedMode = [IO.File]::ReadAllText((Join-Path $addonRoot 'Prefabs/GameModes/AROR_GameMode_Base.et'))
if ($sharedMode -notmatch 'AROR_RegionalSupportComponent[^\r\n]*\{\s+m_bRegionalSupport 0\s+\}') { throw 'Shared game mode must explicitly disable regional support' }
$chicken = [IO.File]::ReadAllText((Join-Path $addonRoot 'Worlds/AROR_Everon/AROR_Everon_Chicken_Layers/Managers.layer'))
if ($chicken -notmatch 'AROR_RegionalSupportComponent[^\r\n]*\{\s+m_bRegionalSupport 1\s+\}') { throw 'Chicken must explicitly opt into regional support' }
Write-Output 'Verified regional support is disabled by default in script/shared prefab and explicitly enabled in Chicken.'
$switches = @{
 'Garrisons/AROR_GarrisonComponent.c' = 'm_bHasGarrison'
 'Garrisons/AROR_GarrisonManagerComponent.c' = 'm_bGarrisonsActive'
 'Settlements/AROR_RegionalSupportComponent.c' = 'm_bRegionalSupport'
 'Operations/AROR_OperationExecutorComponent.c' = 'm_bExecuteOperations'
 'Operations/AROR_QRFDefenseComponent.c' = 'm_bReportDamage'
}
foreach ($source in $switches.Keys) {
 $script = [IO.File]::ReadAllText((Join-Path $addonRoot ('Scripts/Game/'+$source)))
 if (!$script.Contains('protected bool '+$switches[$source]) -or $script.Contains('protected bool m_bEnabled;')) { throw "Component switch has not been renamed: $source" }
 foreach ($attribute in [regex]::Matches($script,'\[Attribute\([^\r\n]*\)\]')) {
  if ($attribute.Value -notmatch '\bdesc:') { throw "Missing designer tooltip: $source $($attribute.Value)" }
 }
}
if ($chicken -notmatch 'm_bExecuteOperations\s+1\b') { throw 'Chicken must use the renamed executor switch' }
Write-Output 'Verified purpose-specific component switches and designer descriptions.'
Write-Output "Verified $checked garrison/shared resource GUIDs, military component placement and settlement/HQ exclusions. Hand-authored group metadata still requires Workbench discovery; native seating/gameplay unverified."
