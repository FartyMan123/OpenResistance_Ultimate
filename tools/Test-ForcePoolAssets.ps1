param(
	[Parameter(Mandatory = $true)]
	[string]$ResourceDatabasePath
)

$ErrorActionPreference = 'Stop'
$repoPath = Split-Path -Parent $PSScriptRoot
$manifest = Get-Content -LiteralPath (Join-Path $repoPath 'docs/FORCE_ASSETS.json') -Raw | ConvertFrom-Json
$resourceBytes = [System.IO.File]::ReadAllBytes($ResourceDatabasePath)
$resourceText = [System.Text.Encoding]::GetEncoding(28591).GetString($resourceBytes)
$verified = 0
foreach ($entry in $manifest.resources.PSObject.Properties) {
	$resourceIndex = $resourceText.IndexOf($entry.Name + [char]0)
	if ($resourceIndex -lt 0) { throw "Installed resource missing: $($entry.Name)" }
	$resourceGuid = [System.BitConverter]::ToUInt64($resourceBytes, $resourceIndex + $entry.Name.Length + 7).ToString('X16')
	$actual = '{' + $resourceGuid + '}' + $entry.Name
	if ($actual -cne $entry.Value) { throw "GUID mismatch: $($entry.Name)" }
	$verified++
}

$layerPath = Join-Path $repoPath 'AROpenResistance/Open Resistance/Worlds/AROR_Everon/AROR_Everon_Chicken_Layers/Managers.layer'
$layerText = [System.IO.File]::ReadAllText($layerPath)
$navigation = @{
	'5584F30E67F617AD' = 'worlds/GameMaster/Navmeshes/GM_Eden.nmn'
	'5584F30EEFEE1223' = 'worlds/GameMaster/Navmeshes/GM_Eden_vehicles.nmn'
	'5C8C9B750D124A63' = 'worlds/MP/Navmeshes/LowResEden.nmn'
}
foreach ($componentId in $navigation.Keys) {
	$pattern = 'NavmeshWorldComponent\s+"\{' + $componentId + '\}"\s*\{[\s\S]*?NavmeshFile\s+"([^"]+)"'
	$binding = [regex]::Match($layerText, $pattern)
	$expected = $manifest.resources.PSObject.Properties[$navigation[$componentId]]
	if (!$binding.Success -or !$expected -or $binding.Groups[1].Value -cne $expected.Value) { throw "Missing/incorrect Chicken navigation binding: $componentId" }
}
$profiles = [regex]::Matches($layerText, '(?ms)^\s{4}AROR_ForceProfile\s*\{(.*?)^\s{4}\}')
if ($profiles.Count -ne 8) { throw "Expected eight Chicken profiles, found $($profiles.Count)" }
$profileIds = @()
$airborneCount = 0
$roleFactions = @{'1' = 'AROR_OCCUPIERS'; '2' = 'AROR_INVADERS'}
$assetFactions = @{'1' = 'US'; '2' = 'USSR'}
foreach ($profile in $profiles) {
	$body = $profile.Groups[1].Value
	$roleId = [regex]::Match($body, 'm_eRole\s+(\d+)').Groups[1].Value
	$factionKey = [regex]::Match($body, 'm_sFactionKey\s+"([^"]+)"').Groups[1].Value
	$assetKey = [regex]::Match($body, 'm_sAssetFactionKey\s+"([^"]+)"').Groups[1].Value
	if (!$roleFactions.ContainsKey($roleId) -or $factionKey -cne $roleFactions[$roleId] -or $assetKey -cne $assetFactions[$roleId]) { throw 'Chicken profile must match core campaign and native asset factions' }
	$profileId = [regex]::Match($body, 'm_sId\s+"([^"]+)"').Groups[1].Value
	if (!$profileId -or $profileIds -contains $profileId) { throw "Empty/duplicate profile ID: $profileId" }
	$profileIds += $profileId
	if ($body -match 'm_eType\s+3\b') {
		$airborneCount++
		if ($body -notmatch 'm_bEnabled\s+0\b') { throw "Airborne profile must stay disabled until flight adapter exists: $profileId" }
	}
	foreach ($reference in [regex]::Matches($body, '\{[0-9A-F]{16}\}(Prefabs/[^"\r\n]+)')) {
		$manifestEntry = $manifest.resources.PSObject.Properties[$reference.Groups[1].Value]
		if (!$manifestEntry -or $manifestEntry.Value -cne $reference.Value) { throw "Profile resource absent from verified manifest: $($reference.Value)" }
	}
}
if ($airborneCount -ne 2) { throw 'Expected one disabled airborne profile per faction' }
if ($layerText -match 'm_sResistanceFactionKey|m_sOccupiersFactionKey|m_sInvadersFactionKey|m_sAffiliatedFaction\s+"FIA"') { throw 'Chicken must inherit shared role mappings and the Resistance loadout' }
$defenderPath = Join-Path $repoPath 'AROpenResistance/Open Resistance/Worlds/AROR_Everon/AROR_Everon_Chicken_Layers/CivilianLocations.layer'
$defenderText = [IO.File]::ReadAllText($defenderPath)
if ([regex]::Matches($defenderText, 'm_bInitializeFromDefendingRole\s+1').Count -ne 2 -or $defenderText -match 'm_sInitialFactionKey') { throw 'Both Chicken defenders must initialize from the shared defending role' }
$maxGroups = [int][regex]::Match($layerText, 'm_iMaxGroups\s+(\d+)').Groups[1].Value
$maxMembers = [int][regex]::Match($layerText, 'm_iMaxMembers\s+(\d+)').Groups[1].Value
$maxVehicles = [int][regex]::Match($layerText, 'm_iMaxVehicles\s+(\d+)').Groups[1].Value
foreach ($profile in $profiles) {
	$body = $profile.Groups[1].Value
	$type = [int][regex]::Match($body, 'm_eType\s+(\d+)').Groups[1].Value
	if ($type -eq 3 -or $body -match 'm_bEnabled\s+0\b') { continue }
	$groupsPerPackage = 2
	if ($type -eq 2 -or $type -eq 4) { $groupsPerPackage = 1 }
	$membersPerPackage = [int][regex]::Match($body, 'm_iPersonnelBudget\s+(\d+)').Groups[1].Value
	foreach ($level in 1..5) {
		$vehicles = $level
		if ($type -eq 4) { $vehicles = 0 }
		if ($groupsPerPackage * $level -gt $maxGroups -or $membersPerPackage * $level -gt $maxMembers -or $vehicles -gt $maxVehicles) { throw "Prepared profile cannot fit response level $level" }
	}
}
$levels = [regex]::Matches($layerText, '(?ms)^\s{4}AROR_ResponseLevel\s*\{(.*?)^\s{4}\}')
if ($levels.Count -ne 5) { throw 'Expected all five response levels in Chicken' }
$levelIds = @()
foreach ($level in $levels) {
	$levelId = [int][regex]::Match($level.Groups[1].Value, 'm_iLevel\s+(\d+)').Groups[1].Value
	if ($levelId -lt 1 -or $levelId -gt 5 -or $levelIds -contains $levelId) { throw 'Invalid or duplicate response level' }
	$levelIds += $levelId
	foreach ($weight in @('m_iAttackWeight','m_iQRFWeight')) {
		$weightMatch = [regex]::Match($level.Groups[1].Value, $weight + '\s+(\d+)')
		if (!$weightMatch.Success -or [int]$weightMatch.Groups[1].Value -lt 1) { throw "Preset level $levelId must be selectable for both attacks and QRFs" }
	}
}
Write-Output "Verified $verified installed resource GUIDs, three Chicken navigation bindings, eight unique profiles, two disabled airborne entries and all five selectable response levels. This is configuration validation, not Enforce compilation or gameplay testing."
