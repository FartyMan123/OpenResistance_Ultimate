param(
	[string]$WorkbenchPath = 'D:\SteamLibrary\steamapps\common\Arma Reforger Tools\Workbench\ArmaReforgerWorkbenchSteamDiag.exe',
	[string]$GameAddonsPath,
	[string]$BaseGameAddonsPath,
	[switch]$PrintOnly
)

# Opens the Open Resistance - WCS extender in Workbench, resolving the repository core and the WCS/RHS addons
# downloaded by the game, without registering them globally.

$ErrorActionPreference = 'Stop'
$repoPath = Split-Path -Parent $PSScriptRoot
$projectPath = Join-Path $repoPath 'OptionalAddons/OpenResistanceWCS/addon.gproj'
$corePath = Join-Path $repoPath 'AROpenResistance/Open Resistance/addon.gproj'
$coreAddonsPath = Join-Path $repoPath 'AROpenResistance'
$coreGuid = '6AA282336705EED2'

if (!$BaseGameAddonsPath) {
	$toolsInstallPath = Split-Path -Parent (Split-Path -Parent $WorkbenchPath)
	$steamCommonPath = Split-Path -Parent $toolsInstallPath
	$BaseGameAddonsPath = Join-Path $steamCommonPath 'Arma Reforger/addons'
}
$baseGameProject = Join-Path $BaseGameAddonsPath 'data/ArmaReforger.gproj'

foreach ($requiredFile in @($WorkbenchPath, $projectPath, $corePath, $baseGameProject)) {
	if (!(Test-Path -LiteralPath $requiredFile -PathType Leaf)) { throw "Required file not found: $requiredFile" }
}
if (![IO.File]::ReadAllText($corePath).Contains($coreGuid)) { throw 'The repository core project has an unexpected GUID' }

# Every dependency of the extender (and the core's own, e.g. REAPER) except the core and the base game must be
# installed in the game's addon folder
$projectText = [IO.File]::ReadAllText($projectPath) + [IO.File]::ReadAllText($corePath)
$dependencies = [regex]::Matches(($projectText -split 'Dependencies')[1..2] -join ' ', '"([0-9A-F]{16})"') |
	ForEach-Object { $_.Groups[1].Value } |
	Where-Object { $_ -ne $coreGuid -and $_ -ne '58D0FB3206B6F859' } |
	Select-Object -Unique

if (!$GameAddonsPath) {
	$documentCandidates = @([Environment]::GetFolderPath('MyDocuments'))
	foreach ($oneDriveRoot in @($env:OneDrive, $env:OneDriveConsumer, $env:OneDriveCommercial, (Join-Path $env:USERPROFILE 'OneDrive'))) {
		if ($oneDriveRoot) { $documentCandidates += Join-Path $oneDriveRoot 'Documents' }
	}
	foreach ($documentsPath in ($documentCandidates | Select-Object -Unique)) {
		$addonsCandidate = Join-Path $documentsPath 'My Games/ArmaReforger/addons'
		if (Test-Path -LiteralPath $addonsCandidate -PathType Container) { $GameAddonsPath = $addonsCandidate; break }
	}
	if (!$GameAddonsPath) { throw 'The game addon folder was not found in Documents/OneDrive. Supply -GameAddonsPath explicitly.' }
}

$installed = Get-ChildItem -LiteralPath $GameAddonsPath -Directory
$missing = @()
foreach ($guid in $dependencies) {
	$folder = $installed | Where-Object Name -Like "*$guid" | Select-Object -First 1
	if (!$folder -or !(Test-Path -LiteralPath (Join-Path $folder.FullName 'addon.gproj') -PathType Leaf)) { $missing += $guid }
}
if ($missing) { throw ("Not installed in {0}: {1}. Subscribe to them in the game and let them download." -f $GameAddonsPath, ($missing -join ', ')) }

$searchPaths = $BaseGameAddonsPath + ',' + $coreAddonsPath + ',' + $GameAddonsPath
if ($projectPath.Contains('"') -or $searchPaths.Contains('"')) { throw 'Unexpected quote in project/search path' }
$launchArguments = @('-gproj', ('"{0}"' -f $projectPath), '-addonsDir', ('"{0}"' -f $searchPaths))
Write-Output "Project: $projectPath"
Write-Output "Core: $corePath"
Write-Output "Dependencies found: $($dependencies.Count)"
Write-Output "Search roots: $searchPaths"
if ($PrintOnly) { return }

# This launches the visible interactive editor, not a build or automated Workbench action.
Start-Process -FilePath $WorkbenchPath -WorkingDirectory (Split-Path -Parent $WorkbenchPath) -ArgumentList $launchArguments -WindowStyle Normal
