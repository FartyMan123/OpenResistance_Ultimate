[CmdletBinding()]
param([switch]$Apply)
# Generates the Fortress Vostok stronghold composition (Prefabs/Compositions/Locations/AROR_Defence_Fortress_Vostok.et):
# a 72 m high-sandbag fortress with six crewed gun pits (four walls plus two corner bastions), level-4 garrison markers,
# three manned hull pads and a parked hull, two bunkers, USSR stores and an outer hedgehog/wire belt.
# Without -Apply it only checks that the files on disk match.
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$addonRoot = Join-Path $repoRoot 'AROpenResistance/Open Resistance'
$culture = [Globalization.CultureInfo]::InvariantCulture
$utf8 = New-Object Text.UTF8Encoding($false)
. (Join-Path $PSScriptRoot 'GunEmbrasures.ps1')
function Number($value) { ([double]$value).ToString('0.######',$culture) }
function EntityId([string]$name) {
 $digest = [Security.Cryptography.SHA256]::Create().ComputeHash([Text.Encoding]::UTF8.GetBytes("FortressVostok/$name"))
 ([BitConverter]::ToString($digest).Replace('-','')).Substring(0,16)
}

$name = 'Fortress_Vostok'
$guid = '7C1E5A0D93B4F268' # Hand-authored resource GUID, recorded in the sibling .meta
$resourcePath = "Prefabs/Compositions/Locations/AROR_Defence_$name.et"
$half = 36
$pitch = 2.4
$high = '{BE16EE8FAA315FE2}Prefabs/Props/Military/Sandbags/Sandbag_01_long_high_burlap.et'
$low = '{21D83950DF36F250}Prefabs/Props/Military/Sandbags/Sandbag_01_long_burlap.et'
$bunker = '{5821AF9613353C80}Prefabs/Props/Military/Sandbags/Sandbag_01_bunker_burlap_camonet.et'
$hedgehog = '{AE28D47CF26E91EA}Prefabs/Props/Military/Fortification/CzechHedgehog_01_rusty.et'
$wire = '{BE7B5BAFE1A16A50}Prefabs/Props/Military/Fortification/BarbedTape_Coil.et'
$stores = @(
 @('Weapons','{8C7C140EEB0ECF36}Prefabs/Props/Military/AmmoBoxes/USSR/EquipmentBoxWooden_Weapon_01_USSR.et'),
 @('Ammunition','{A48EDE9CBA71B5A3}Prefabs/Props/Military/AmmoBoxes/USSR/EquipmentBoxWooden_Ammunition_01_USSR.et'),
 @('Equipment','{CD62E36CFDCB8FD8}Prefabs/Props/Military/AmmoBoxes/USSR/EquipmentBoxWooden_Equipment_01_USSR.et'),
 @('Medical','{70837F44CD03FD05}Prefabs/Props/Military/AmmoBoxes/USSR/EquipmentBoxWooden_Medical_01_USSR.et'))

# Gun pits: x, z, facing. Walls get low sandbags across each pit's field of fire.
$guns = @(@(0,32,0),@(-32,6,270),@(32,6,90),@(-10,-32,180),@(-30,30,315),@(30,30,45))
# Low wall spans per side (0 north, 1 east, 2 south, 3 west): from, to along the wall
$lowSpans = @{ 0 = @(@(-6,6),@(-36,-24),@(24,36)); 1 = @(@(0,12),@(24,36)); 2 = @(@(-16,-4)); 3 = @(@(0,12),@(24,36)) }
$entrance = @(6,18) # South gate, wide enough for the manned hulls
$teams = @(@(-20,20,315),@(20,20,45),@(-22,-20,225),@(22,-20,135),@(-12,8,270),@(12,8,90),@(-24,-4,270),@(24,-4,90))
$hulls = @(@(12,-16),@(12,-4),@(-12,-4))
$parked = @(-14,-18)

$lines = New-Object 'Collections.Generic.List[string]'
function Child([string]$entity, [string]$prefab, $x, $z, $yaw, [string[]]$components) {
 $header = '  GenericEntity {0}' -f $entity
 if ($prefab) { $header += (' : "{0}"' -f $prefab) }
 $lines.Add($header + ' {')
 $lines.Add(('   ID "{0}"' -f (EntityId $entity)))
 $lines.Add('   components {')
 $lines.Add('    Hierarchy {')
 $lines.Add('    }')
 foreach ($component in $components) { $lines.Add($component) }
 $lines.Add('   }')
 $lines.Add(('   coords {0} 0 {1}' -f (Number $x),(Number $z)))
 if ($yaw -ne 0) { $lines.Add(('   angles 0 {0} 0' -f (Number $yaw))) }
 $lines.Add('  }')
}
$wall = @('    AROR_DefenceWallComponent {','    }')

$lines.Add('GenericEntity : "{118F89378FB3D213}Prefabs/Locations/AROR_Location_MilitaryBase.et" {')
$lines.Add((' ID "{0}"' -f (EntityId 'root')))
$lines.Add(' components {')
$lines.Add('  AROR_LocationComponent "{6AA28237301A1CF9}" {')
$lines.Add('   m_sName "Fortress"')
$lines.Add('   m_fRadius 200')
$lines.Add('   m_bGroundOnInit 0')
$lines.Add('  }')
$lines.Add('  AROR_GarrisonComponent "{66D30571F98F4BE3}" {')
$lines.Add('   m_iDefenceLevel 4')
$lines.Add('   m_iStaticWeaponCountOverride 6')
$lines.Add(('   m_fDefenceHalfExtent {0}' -f $half))
$lines.Add('   m_fMaxDefenceRelief 3')
$lines.Add('   m_fEditorSearchRadius 600')
$lines.Add('   m_fDefenceHoldRadius 6')
$lines.Add('  }')
$lines.Add('  AROR_StrongholdComponent {')
$lines.Add('  }')
$lines.Add(' }')
$lines.Add(' coords 0 0 0')
$lines.Add(' {')
# Inherited capture flag at the centre, keeping the model-base and interaction offsets of the other compositions
$lines.Add('  GenericEntity {')
$lines.Add('   ID "6AACA42C3DE4F4A5"')
$lines.Add('   components {')
$lines.Add('    AROR_CaptureFlagPlacementComponent "{F9A38254AA9B454D}" {')
$lines.Add('     m_bAutoPlace 0')
$lines.Add('    }')
$lines.Add('    ActionsManagerComponent "{6AAE5E8DE935135E}" {')
$lines.Add('     ActionContexts {')
$lines.Add('      UserActionContext "{6AAE5E8DEECDFE8B}" {')
$lines.Add('       Position PointInfo "{6AAE5E8DE4DBC0C4}" {')
$lines.Add('        Offset 0 0.694054 0')
$lines.Add('       }')
$lines.Add('      }')
$lines.Add('     }')
$lines.Add('    }')
$lines.Add('   }')
$lines.Add('   coords 0 0.905946 0')
$lines.Add('  }')

# Perimeter: high sandbags, low across gun fields of fire, open at the south gate
$segments = [int][Math]::Ceiling((2 * $half) / $pitch)
for ($side = 0; $side -lt 4; $side++) {
 for ($segment = 0; $segment -lt $segments; $segment++) {
  $along = -$half + ($segment + 0.5) * (2 * $half / $segments)
  if ($side -eq 2 -and $along -gt $entrance[0] -and $along -lt $entrance[1]) { continue }
  $prefab = $high
  foreach ($span in $lowSpans[$side]) { if ($along -ge $span[0] -and $along -le $span[1]) { $prefab = $low } }
  $x = $along; $z = $half; $yaw = 0
  if ($side -eq 1) { $x = $half; $z = $along; $yaw = 90 }
  if ($side -eq 2) { $z = -$half }
  if ($side -eq 3) { $x = -$half; $z = $along; $yaw = 90 }
  # Open embrasure: tripod guns fire from below even a low sandbag wall
  if (Test-InEmbrasure $x $z $guns) { continue }
  Child "AROR_Fortress_Vostok_Perimeter_${side}_$segment" $prefab $x $z $yaw $wall
 }
}

# Gun pits: a low traverse either side of each gun, parallel to its line of fire
for ($gun = 0; $gun -lt $guns.Count; $gun++) {
 $yaw = $guns[$gun][2]; $angle = $yaw * [Math]::PI / 180
 $rightX = [Math]::Cos($angle); $rightZ = -[Math]::Sin($angle)
 foreach ($side in @(-1,1)) {
  $x = $guns[$gun][0] + $rightX * 3.5 * $side
  $z = $guns[$gun][1] + $rightZ * 3.5 * $side
  $label = 'L'; if ($side -gt 0) { $label = 'R' }
  Child "AROR_Fortress_Vostok_Gun_${gun}_Traverse_$label" $low $x $z ($yaw + 90) $wall
 }
}

# Command and barracks bunkers, gate guard posts and the gate obstacles
Child 'AROR_Fortress_Vostok_Command_Bunker' $bunker 0 20 180 $wall
Child 'AROR_Fortress_Vostok_Barracks_Bunker' $bunker -24 -28 0 $wall
Child 'AROR_Fortress_Vostok_Gate_Guard_Left' $low ($entrance[0] - 2) (-$half + 3) 90 $wall
Child 'AROR_Fortress_Vostok_Gate_Guard_Right' $low ($entrance[1] + 2) (-$half + 3) 90 $wall
Child 'AROR_Fortress_Vostok_Gate_Obstacle_Left' $hedgehog ($entrance[0] - 2) (-$half - 6) 15 $wall
Child 'AROR_Fortress_Vostok_Gate_Obstacle_Right' $hedgehog ($entrance[1] + 2) (-$half - 6) -15 $wall

# Outer belt: hedgehogs 7 m out, wire coils between them 4 m out, the gate approach left open
for ($side = 0; $side -lt 4; $side++) {
 for ($step = 0; $step -le 10; $step++) {
  $along = -40 + $step * 8
  foreach ($ring in @(@('Hedgehog',$hedgehog,($half + 7),0),@('Wire',$wire,($half + 4),4))) {
   $position = $along + $ring[3]
   if ($position -gt 40) { continue }
   if ($side -eq 2 -and $position -gt ($entrance[0] - 4) -and $position -lt ($entrance[1] + 4)) { continue }
   $x = $position; $z = $ring[2]; $yaw = 0
   if ($side -eq 1) { $x = $ring[2]; $z = $position; $yaw = 90 }
   if ($side -eq 2) { $z = -$ring[2] }
   if ($side -eq 3) { $x = -$ring[2]; $z = $position; $yaw = 90 }
   Child ("AROR_Fortress_Vostok_{0}_{1}_{2}" -f $ring[0],$side,$step) $ring[1] $x $z $yaw $wall
  }
 }
}

# USSR stores along the north wall
for ($store = 0; $store -lt $stores.Count; $store++) {
 $supply = @('    AROR_DefenceWallComponent {','    }','    AROR_OutpostSupplyComponent {',('     m_sCategory "{0}"' -f $stores[$store][0]),'    }')
 Child ("AROR_Fortress_Vostok_Loot_{0}" -f $stores[$store][0]) $stores[$store][1] (10 + $store * 3) 27 180 $supply
}

# Garrison markers. Kind: 0 infantry team, 1 gun, 2 parked hull, 3 manned hull, 4 crew staging (ordinal = group)
$positions = @()
for ($team = 0; $team -lt $teams.Count; $team++) { $positions += ,@(0,$team,$teams[$team][0],$teams[$team][1],$teams[$team][2]) }
for ($gun = 0; $gun -lt $guns.Count; $gun++) { $positions += ,@(1,$gun,$guns[$gun][0],$guns[$gun][1],$guns[$gun][2]) }
$positions += ,@(2,0,$parked[0],$parked[1],180)
for ($hull = 0; $hull -lt $hulls.Count; $hull++) { $positions += ,@(3,$hull,$hulls[$hull][0],$hulls[$hull][1],180) }
# Groups 8-13 are the gun crews and 14-16 the hull crews; stage them in a column clear of the hull pads
for ($group = 8; $group -le 16; $group++) { $positions += ,@(4,$group,0,(-6 - ($group - 8) * 2.5),180) }
foreach ($position in $positions) {
 $marker = @('    AROR_DefencePositionComponent {',('     m_iKind {0}' -f $position[0]),('     m_iOrdinal {0}' -f $position[1]),'    }')
 Child ("AROR_Fortress_Vostok_Defence_{0}_{1}" -f $position[0],$position[1]) '' $position[2] $position[3] $position[4] $marker
}
$lines.Add(' }')
$lines.Add('}')

$path = Join-Path $addonRoot $resourcePath
$text = ($lines -join "`r`n") + "`r`n"
$meta = 'MetaFileClass {' + "`r`n" + (' Name "{{{0}}}{1}"' -f $guid,$resourcePath) + "`r`n Configurations {`r`n"
foreach ($platform in @('PC','XBOX_ONE','XBOX_SERIES','PS4','PS5','HEADLESS')) {
 $parent = ''; if ($platform -ne 'PC') { $parent = ' : PC' }
 $meta += "  EntityTemplateResourceClass $platform$parent {`r`n  }`r`n"
}
$meta += " }`r`n}`r`n"
if ($Apply) {
 [IO.File]::WriteAllText($path,$text,$utf8)
 [IO.File]::WriteAllText($path + '.meta',$meta,$utf8)
 Write-Output "Wrote $resourcePath"
} else {
 foreach ($expected in @(@($path,$text),@(($path + '.meta'),$meta))) {
  if (!(Test-Path -LiteralPath $expected[0]) -or [IO.File]::ReadAllText($expected[0]).Replace("`r`n","`n") -cne $expected[1].Replace("`r`n","`n")) { throw "Fortress composition differs from this generator: $($expected[0])" }
 }
 Write-Output 'Fortress Vostok composition matches its generator. Native discovery, grounding, gun arcs and placement remain Workbench checks.'
}
