//! One base-game faction to import and the preset file it writes
class AROR_BaseFactionImport
{
	string m_sFactionKey;	//!< Base-game faction key, e.g. "US"
	string m_sFile;			//!< Preset file name in Configs/FactionPresets/BaseGame/
	string m_sPresetId;
	string m_sPresetName;
	ref array<int> m_aUsableRoles = {};

	//------------------------------------------------------------------------------------------------
	void AROR_BaseFactionImport(string factionKey, string file, string presetId, string presetName, notnull array<int> usableRoles)
	{
		m_sFactionKey = factionKey;
		m_sFile = file;
		m_sPresetId = presetId;
		m_sPresetName = presetName;
		m_aUsableRoles.Copy(usableRoles);
	}
}

//! Entries collected for one preset, already in .conf text form
class AROR_PresetImportResult
{
	ref array<string> m_aUnits = {};
	ref array<string> m_aVehicles = {};
	ref array<string> m_aGear = {};
	ref array<string> m_aEquipment = {};
	ref array<string> m_aWeapons = {};
	ref array<string> m_aAccessories = {};
	ref array<string> m_aAmmunition = {};
	ref array<string> m_aVehicleAmmunition = {};
	ref set<int> m_UnitTypesUsed = new set<int>();
	ref array<string> m_aSkipped = {};
}

//! Imports the base game's US, USSR, FIA and CIV factions into the core faction presets
//! (Configs/FactionPresets/BaseGame/*.conf). Reads each faction's entity catalogs through our faction manager prefab
//! (which inherits the base-game factions), sorts every entry into the preset types using the base game's own tags
//! (Game Master labels for characters and vehicles, arsenal item type/mode for items) plus a few file-name hints, and
//! rewrites the preset files. Prices, readiness and import requirements live in the Base Game content pack, which this
//! never touches; add new prefabs there by hand. Anything that cannot be placed is listed in the log.
//! Re-run after game updates; it overwrites the four base preset files.
[WorkbenchPluginAttribute(name: "Import base faction presets", description: "Fill the base-game faction presets (US, USSR, FIA, CIV) from the game's faction catalogs.", wbModules: {"WorldEditor", "ResourceManager"}, category: "Open Resistance")]
class AROR_ImportBaseFactionPresetsPlugin : WorkbenchPlugin
{
	protected static const ResourceName FACTION_MANAGER = "{906843134C8EF19C}Prefabs/Managers/AROR_FactionManager.et";
	protected static const string OUTPUT_FOLDER = "$OpenResistance:Configs/FactionPresets/BaseGame/";
	protected static const string FALLBACK_FOLDER = "$profile:AROR_FactionPresets/";

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		if (!GetGame())
		{
			Print("[AROR] Import base faction presets: run this from the World Editor (Plugins > Open Resistance)", LogLevel.ERROR);
			return;
		}

		array<ref AROR_BaseFactionImport> imports = {};
		imports.Insert(new AROR_BaseFactionImport("FIA", "AROR_Preset_FIA.conf", "FIA", "FIA", {AROR_EFactionRole.RESISTANCE}));
		imports.Insert(new AROR_BaseFactionImport("US", "AROR_Preset_US_Army.conf", "US_ARMY", "US Army", {AROR_EFactionRole.OCCUPIERS, AROR_EFactionRole.INVADERS}));
		imports.Insert(new AROR_BaseFactionImport("USSR", "AROR_Preset_USSR_Army.conf", "USSR_ARMY", "Soviet Army", {AROR_EFactionRole.OCCUPIERS, AROR_EFactionRole.INVADERS}));
		imports.Insert(new AROR_BaseFactionImport("CIV", "AROR_Preset_Everon_Civilians.conf", "EVERON_CIV", "Everon Civilians", {AROR_EFactionRole.CIVILIANS}));

		Resource resource = BaseContainerTools.LoadContainer(FACTION_MANAGER);
		if (!resource || !resource.IsValid())
		{
			Print(string.Format("[AROR] Import base faction presets: cannot load %1", FACTION_MANAGER), LogLevel.ERROR);
			return;
		}

		BaseContainerList factions = resource.GetResource().ToBaseContainer().GetObjectArray("Factions");
		if (!factions)
		{
			Print("[AROR] Import base faction presets: the faction manager has no Factions list", LogLevel.ERROR);
			return;
		}

		foreach (AROR_BaseFactionImport factionImport : imports)
		{
			BaseContainer faction = FindFaction(factions, factionImport.m_sFactionKey);
			if (!faction)
			{
				Print(string.Format("[AROR] Import base faction presets: faction '%1' not found; '%2' left unchanged", factionImport.m_sFactionKey, factionImport.m_sFile), LogLevel.WARNING);
				continue;
			}

			AROR_PresetImportResult result = new AROR_PresetImportResult();
			ReadFaction(faction, result);
			WritePreset(factionImport, result);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected BaseContainer FindFaction(BaseContainerList factions, string factionKey)
	{
		for (int i = 0; i < factions.Count(); i++)
		{
			BaseContainer faction = factions.Get(i);
			string key;
			if (faction && faction.Get("FactionKey", key) && key == factionKey)
				return faction;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Every catalog of the faction: characters, vehicles, static weapons and inventory items
	protected void ReadFaction(BaseContainer faction, AROR_PresetImportResult result)
	{
		BaseContainerList catalogs = faction.GetObjectArray("m_aEntityCatalogs");
		if (!catalogs)
			return;

		for (int i = 0; i < catalogs.Count(); i++)
		{
			SCR_EntityCatalog catalog = SCR_EntityCatalog.Cast(BaseContainerTools.CreateInstanceFromContainer(catalogs.Get(i)));
			if (!catalog)
				continue;

			// Merges multi-lists, drops disabled entries and loads each entry's labels and data
			catalog.InitCatalog();

			array<SCR_EntityCatalogEntry> entries = {};
			catalog.GetEntityList(entries);

			EEntityCatalogType catalogType = catalog.GetCatalogType();
			foreach (SCR_EntityCatalogEntry entry : entries)
			{
				switch (catalogType)
				{
					case EEntityCatalogType.CHARACTER:
						AddUnit(entry, result);
						break;
					case EEntityCatalogType.VEHICLE:
						AddVehicle(entry, result);
						break;
					case EEntityCatalogType.WEAPONS_TRIPOD:
						AddEntry(result.m_aVehicles, "AROR_FactionPresetVehicle", AROR_EVehicleType, AROR_EVehicleType.STATIC_MG, entry.GetPrefab());
						break;
					case EEntityCatalogType.ITEM:
						AddItem(entry, result);
						break;
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	// Characters
	//------------------------------------------------------------------------------------------------
	protected void AddUnit(SCR_EntityCatalogEntry entry, AROR_PresetImportResult result)
	{
		int type = UnitType(entry);
		if (type < 0)
		{
			result.m_aSkipped.Insert("unit (no matching type): " + entry.GetPrefab());
			return;
		}

		// One unit per type: the first match wins
		if (!result.m_UnitTypesUsed.Insert(type))
		{
			result.m_aSkipped.Insert(string.Format("unit (%1 already set): %2", typename.EnumToString(AROR_EUnitType, type), entry.GetPrefab()));
			return;
		}

		AddEntry(result.m_aUnits, "AROR_FactionPresetUnit", AROR_EUnitType, type, entry.GetPrefab());
	}

	//------------------------------------------------------------------------------------------------
	protected int UnitType(SCR_EntityCatalogEntry entry)
	{
		string name = LowerName(entry.GetPrefab());

		// Roles the labels cannot tell apart are recognised by file name
		if (name.Contains("pilot") || entry.HasEditableEntityLabel(EEditableEntityLabel.TRAIT_HELI_CREW))
			return AROR_EUnitType.PILOT;
		if (name.Contains("crew") || entry.HasEditableEntityLabel(EEditableEntityLabel.TRAIT_VEHICLE_CREW))
			return AROR_EUnitType.CREW;
		if (name.Contains("officer"))
			return AROR_EUnitType.OFFICER;
		if (name.Contains("sniper"))
			return AROR_EUnitType.SNIPER;
		if (name.Contains("_aa.") || name.Contains("_aa_") || name.Contains("antiair"))
			return AROR_EUnitType.AA;
		if (name.Contains("_ar.") || name.Contains("_ar_") || name.Contains("autorifle"))
			return AROR_EUnitType.AUTORIFLEMAN;
		if (entry.HasEditableEntityLabel(EEditableEntityLabel.FACTION_CIV) || name.Contains("civilian"))
			return AROR_EUnitType.CIVILIAN;

		if (entry.HasEditableEntityLabel(EEditableEntityLabel.ROLE_LEADER))
			return AROR_EUnitType.LEADER;
		if (entry.HasEditableEntityLabel(EEditableEntityLabel.ROLE_MEDIC))
			return AROR_EUnitType.MEDIC;
		if (entry.HasEditableEntityLabel(EEditableEntityLabel.ROLE_ANTITANK))
			return AROR_EUnitType.AT;
		if (entry.HasEditableEntityLabel(EEditableEntityLabel.ROLE_MACHINEGUNNER))
			return AROR_EUnitType.MACHINEGUNNER;
		if (entry.HasEditableEntityLabel(EEditableEntityLabel.ROLE_GRENADIER))
			return AROR_EUnitType.GRENADIER;
		if (entry.HasEditableEntityLabel(EEditableEntityLabel.ROLE_SHARPSHOOTER))
			return AROR_EUnitType.MARKSMAN;
		if (entry.HasEditableEntityLabel(EEditableEntityLabel.ROLE_SAPPER))
			return AROR_EUnitType.ENGINEER;
		if (entry.HasEditableEntityLabel(EEditableEntityLabel.ROLE_RADIOOPERATOR))
			return AROR_EUnitType.RADIO_OPERATOR;
		if (entry.HasEditableEntityLabel(EEditableEntityLabel.ROLE_RIFLEMAN))
			return AROR_EUnitType.RIFLEMAN;

		return -1;
	}

	//------------------------------------------------------------------------------------------------
	// Vehicles
	//------------------------------------------------------------------------------------------------
	protected void AddVehicle(SCR_EntityCatalogEntry entry, AROR_PresetImportResult result)
	{
		int type = VehicleType(entry);
		if (type < 0)
		{
			result.m_aSkipped.Insert("vehicle (no matching type): " + entry.GetPrefab());
			return;
		}

		AddEntry(result.m_aVehicles, "AROR_FactionPresetVehicle", AROR_EVehicleType, type, entry.GetPrefab());
	}

	//------------------------------------------------------------------------------------------------
	protected int VehicleType(SCR_EntityCatalogEntry entry)
	{
		string name = LowerName(entry.GetPrefab());

		if (name.Contains("boat"))
			return AROR_EVehicleType.BOAT;

		if (entry.HasEditableEntityLabel(EEditableEntityLabel.VEHICLE_TURRET))
			return AROR_EVehicleType.STATIC_MG;

		if (entry.HasEditableEntityLabel(EEditableEntityLabel.VEHICLE_HELICOPTER))
		{
			if (entry.HasEditableEntityLabel(EEditableEntityLabel.TRAIT_ARMED))
				return AROR_EVehicleType.HELI_ATTACK;

			return AROR_EVehicleType.HELI_TRANSPORT;
		}

		if (entry.HasEditableEntityLabel(EEditableEntityLabel.VEHICLE_APC))
		{
			// Tracked/armoured fighting vehicles carrying an autocannon are treated as IFVs by name
			if (name.Contains("bmp") || name.Contains("ifv"))
				return AROR_EVehicleType.IFV;

			return AROR_EVehicleType.APC;
		}

		if (name.Contains("tank") || name.Contains("t72") || name.Contains("m1a"))
			return AROR_EVehicleType.TANK;

		if (entry.HasEditableEntityLabel(EEditableEntityLabel.VEHICLE_TRUCK))
		{
			if (entry.HasEditableEntityLabel(EEditableEntityLabel.TRAIT_REFUELING))
				return AROR_EVehicleType.FUEL_TRUCK;
			if (entry.HasEditableEntityLabel(EEditableEntityLabel.TRAIT_MEDICAL))
				return AROR_EVehicleType.AMBULANCE;
			if (entry.HasEditableEntityLabel(EEditableEntityLabel.TRAIT_SUPPLYSTORAGE_SMALL) || entry.HasEditableEntityLabel(EEditableEntityLabel.TRAIT_SUPPLYSTORAGE_LARGE)
				|| entry.HasEditableEntityLabel(EEditableEntityLabel.TRAIT_LOGISTICS))
				return AROR_EVehicleType.SUPPLY_TRUCK;

			return AROR_EVehicleType.MOTOR_TRUCK;
		}

		if (entry.HasEditableEntityLabel(EEditableEntityLabel.VEHICLE_CAR))
		{
			if (entry.HasEditableEntityLabel(EEditableEntityLabel.TRAIT_MEDICAL))
				return AROR_EVehicleType.AMBULANCE;
			if (entry.HasEditableEntityLabel(EEditableEntityLabel.TRAIT_ARMED))
				return AROR_EVehicleType.ARMED_CAR;

			return AROR_EVehicleType.MOTOR_CAR;
		}

		return -1;
	}

	//------------------------------------------------------------------------------------------------
	// Items: weapons, ammunition, accessories, gear and equipment, from the arsenal data
	//------------------------------------------------------------------------------------------------
	protected void AddItem(SCR_EntityCatalogEntry entry, AROR_PresetImportResult result)
	{
		SCR_ArsenalItem arsenal = SCR_ArsenalItem.Cast(entry.GetEntityDataOfType(SCR_ArsenalItem));
		if (!arsenal)
		{
			result.m_aSkipped.Insert("item (no arsenal data): " + entry.GetPrefab());
			return;
		}

		ResourceName prefab = entry.GetPrefab();
		string name = LowerName(prefab);
		SCR_EArsenalItemType itemType = arsenal.GetItemType();
		SCR_EArsenalItemMode itemMode = arsenal.GetItemMode();

		// Munitions loaded onto aircraft and vehicles as items (e.g. helicopter rocket pods)
		if (itemMode == SCR_EArsenalItemMode.PYLON || itemType == SCR_EArsenalItemType.HELICOPTER || itemType == SCR_EArsenalItemType.VEHICLE)
		{
			int vehicleAmmunition = AROR_EVehicleAmmunitionType.ROCKET;
			if (name.Contains("missile") || name.Contains("atgm"))
				vehicleAmmunition = AROR_EVehicleAmmunitionType.MISSILE;
			else if (name.Contains("bomb"))
				vehicleAmmunition = AROR_EVehicleAmmunitionType.BOMB;

			AddEntry(result.m_aVehicleAmmunition, "AROR_FactionPresetVehicleAmmunition", AROR_EVehicleAmmunitionType, vehicleAmmunition, prefab);
			return;
		}

		if (itemMode == SCR_EArsenalItemMode.AMMUNITION)
		{
			int ammo = AmmunitionType(itemType, name);
			AddEntry(result.m_aAmmunition, "AROR_FactionPresetAmmunition", AROR_EAmmunitionType, ammo, prefab);
			return;
		}

		if (itemMode == SCR_EArsenalItemMode.ATTACHMENT || itemType == SCR_EArsenalItemType.WEAPON_ATTACHMENT)
		{
			int accessory = AccessoryType(name);
			if (accessory < 0)
				result.m_aSkipped.Insert("accessory (no matching type): " + prefab);
			else
				AddEntry(result.m_aAccessories, "AROR_FactionPresetWeaponAccessory", AROR_EWeaponAccessoryType, accessory, prefab);

			return;
		}

		int weapon = WeaponType(itemType);
		if (weapon >= 0)
		{
			AddEntry(result.m_aWeapons, "AROR_FactionPresetWeapon", AROR_EWeaponType, weapon, prefab);
			return;
		}

		int gear = GearType(itemType, name);
		if (gear >= 0)
		{
			AddEntry(result.m_aGear, "AROR_FactionPresetGear", AROR_EGearType, gear, prefab);
			return;
		}

		int equipment = EquipmentType(itemType, name);
		if (equipment >= 0)
		{
			AddEntry(result.m_aEquipment, "AROR_FactionPresetEquipment", AROR_EEquipmentType, equipment, prefab);
			return;
		}

		result.m_aSkipped.Insert(string.Format("item (%1 has no matching type): %2", typename.EnumToString(SCR_EArsenalItemType, itemType), prefab));
	}

	//------------------------------------------------------------------------------------------------
	protected int WeaponType(SCR_EArsenalItemType itemType)
	{
		switch (itemType)
		{
			case SCR_EArsenalItemType.PISTOL:
				return AROR_EWeaponType.PISTOL;
			case SCR_EArsenalItemType.RIFLE:
				return AROR_EWeaponType.RIFLE;
			case SCR_EArsenalItemType.MACHINE_GUN:
				return AROR_EWeaponType.MACHINEGUN;
			case SCR_EArsenalItemType.SNIPER_RIFLE:
				return AROR_EWeaponType.SNIPER_RIFLE;
			case SCR_EArsenalItemType.ROCKET_LAUNCHER:
				return AROR_EWeaponType.LAUNCHER_AT;
			case SCR_EArsenalItemType.LETHAL_THROWABLE:
			case SCR_EArsenalItemType.NON_LETHAL_THROWABLE:
				return AROR_EWeaponType.GRENADE;
		}

		return -1;
	}

	//------------------------------------------------------------------------------------------------
	protected int AmmunitionType(SCR_EArsenalItemType itemType, string name)
	{
		if (itemType == SCR_EArsenalItemType.ROCKET_LAUNCHER)
			return AROR_EAmmunitionType.ROCKET;
		if (itemType == SCR_EArsenalItemType.MORTARS)
			return AROR_EAmmunitionType.SHELL;
		if (name.Contains("40mm") || name.Contains("vog") || name.Contains("gp25") || name.Contains("m203") || name.Contains("grenade"))
			return AROR_EAmmunitionType.GRENADE_ROUND;
		if (itemType == SCR_EArsenalItemType.MACHINE_GUN || name.Contains("belt") || name.Contains("box"))
			return AROR_EAmmunitionType.BELT;

		return AROR_EAmmunitionType.MAGAZINE;
	}

	//------------------------------------------------------------------------------------------------
	protected int AccessoryType(string name)
	{
		if (name.Contains("suppressor") || name.Contains("silencer") || name.Contains("pbs"))
			return AROR_EWeaponAccessoryType.SUPPRESSOR;
		if (name.Contains("bayonet"))
			return AROR_EWeaponAccessoryType.BAYONET;
		if (name.Contains("ugl") || name.Contains("m203") || name.Contains("gp25") || name.Contains("bipod"))
			return AROR_EWeaponAccessoryType.UNDERBARREL;
		if (name.Contains("flashlight") || name.Contains("light"))
			return AROR_EWeaponAccessoryType.LIGHT;
		if (name.Contains("optic") || name.Contains("scope") || name.Contains("sight") || name.Contains("artii") || name.Contains("pso") || name.Contains("kobra") || name.Contains("_pu"))
			return AROR_EWeaponAccessoryType.OPTIC;

		return -1;
	}

	//------------------------------------------------------------------------------------------------
	protected int GearType(SCR_EArsenalItemType itemType, string name)
	{
		if (itemType == SCR_EArsenalItemType.HEADWEAR)
		{
			if (name.Contains("helmet") || name.Contains("pasgt") || name.Contains("ssh") || name.Contains("6b"))
				return AROR_EGearType.HELMET;
			if (name.Contains("glasses") || name.Contains("goggles") || name.Contains("balaclava") || name.Contains("mask"))
				return AROR_EGearType.FACEWEAR;

			return AROR_EGearType.HEADGEAR;
		}

		// Reforger splits clothing into jacket, trousers, boots and gloves; all count as uniform
		if (itemType == SCR_EArsenalItemType.TORSO || itemType == SCR_EArsenalItemType.LEGS
			|| itemType == SCR_EArsenalItemType.FOOTWEAR || itemType == SCR_EArsenalItemType.HANDWEAR)
			return AROR_EGearType.UNIFORM;

		if (itemType == SCR_EArsenalItemType.VEST_AND_WAIST)
			return AROR_EGearType.VEST;

		if (itemType == SCR_EArsenalItemType.BACKPACK || itemType == SCR_EArsenalItemType.RADIO_BACKPACK)
			return AROR_EGearType.BACKPACK;

		return -1;
	}

	//------------------------------------------------------------------------------------------------
	protected int EquipmentType(SCR_EArsenalItemType itemType, string name)
	{
		if (itemType == SCR_EArsenalItemType.HEAL)
			return AROR_EEquipmentType.MEDICAL;
		if (itemType == SCR_EArsenalItemType.EXPLOSIVES)
			return AROR_EEquipmentType.EXPLOSIVE;
		if (itemType != SCR_EArsenalItemType.EQUIPMENT)
			return -1;

		if (name.Contains("binocular"))
			return AROR_EEquipmentType.BINOCULARS;
		if (name.Contains("radio"))
			return AROR_EEquipmentType.RADIO;
		if (name.Contains("map"))
			return AROR_EEquipmentType.MAP;
		if (name.Contains("compass"))
			return AROR_EEquipmentType.COMPASS;
		if (name.Contains("watch"))
			return AROR_EEquipmentType.WATCH;
		if (name.Contains("flashlight") || name.Contains("torch"))
			return AROR_EEquipmentType.FLASHLIGHT;

		return AROR_EEquipmentType.TOOL;
	}

	//------------------------------------------------------------------------------------------------
	// Output
	//------------------------------------------------------------------------------------------------
	//! Types are written by enum name (e.g. "MOTOR_TRUCK"), like Workbench does, so the files are readable and do
	//! not silently change meaning if an enum is ever reordered
	protected void AddEntry(array<string> lines, string className, typename enumType, int type, ResourceName prefab)
	{
		lines.Insert(string.Format("  %1 {\n   m_sPrefab \"%2\"\n   m_eType %3\n  }", className, prefab, typename.EnumToString(enumType, type)));
	}

	//------------------------------------------------------------------------------------------------
	protected void WritePreset(AROR_BaseFactionImport factionImport, AROR_PresetImportResult result)
	{
		string path = OUTPUT_FOLDER + factionImport.m_sFile;
		FileHandle file = FileIO.OpenFile(path, FileMode.WRITE);
		if (!file)
		{
			FileIO.MakeDirectory(FALLBACK_FOLDER);
			path = FALLBACK_FOLDER + factionImport.m_sFile;
			file = FileIO.OpenFile(path, FileMode.WRITE);
			if (!file)
			{
				Print(string.Format("[AROR] Import base faction presets: cannot write %1", factionImport.m_sFile), LogLevel.ERROR);
				return;
			}

			Print(string.Format("[AROR] Import base faction presets: the addon folder is read-only here, so %1 was written to your profile folder; copy it into Configs/FactionPresets/BaseGame/", factionImport.m_sFile), LogLevel.WARNING);
		}

		file.WriteLine("AROR_FactionPreset {");
		file.WriteLine(string.Format(" m_sId \"%1\"", factionImport.m_sPresetId));
		file.WriteLine(string.Format(" m_sName \"%1\"", factionImport.m_sPresetName));
		file.WriteLine(" m_aUsableRoles {");
		foreach (int role : factionImport.m_aUsableRoles)
		{
			file.WriteLine(string.Format("  %1", typename.EnumToString(AROR_EFactionRole, role)));
		}
		file.WriteLine(" }");

		WriteList(file, "m_aUnits", result.m_aUnits);
		WriteList(file, "m_aVehicles", result.m_aVehicles);
		WriteList(file, "m_aGear", result.m_aGear);
		WriteList(file, "m_aEquipment", result.m_aEquipment);
		WriteList(file, "m_aWeapons", result.m_aWeapons);
		WriteList(file, "m_aWeaponAccessories", result.m_aAccessories);
		WriteList(file, "m_aAmmunition", result.m_aAmmunition);
		WriteList(file, "m_aVehicleAmmunition", result.m_aVehicleAmmunition);
		file.WriteLine("}");
		file.Close();

		Print(string.Format("[AROR] Import base faction presets: %1 (%2) -> %3: %4 units, %5 vehicles, %6 gear, %7 equipment, %8 weapons, %9 accessories",
			factionImport.m_sPresetId, factionImport.m_sFactionKey, path, result.m_aUnits.Count(), result.m_aVehicles.Count(), result.m_aGear.Count(),
			result.m_aEquipment.Count(), result.m_aWeapons.Count(), result.m_aAccessories.Count()), LogLevel.NORMAL);
		Print(string.Format("[AROR] Import base faction presets: %1: %2 ammunition, %3 vehicle ammunition, %4 skipped", factionImport.m_sPresetId, result.m_aAmmunition.Count(), result.m_aVehicleAmmunition.Count(), result.m_aSkipped.Count()), LogLevel.NORMAL);

		foreach (string skipped : result.m_aSkipped)
		{
			Print(string.Format("[AROR]   %1 skipped %2", factionImport.m_sPresetId, skipped), LogLevel.NORMAL);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void WriteList(FileHandle file, string listName, array<string> lines)
	{
		if (lines.IsEmpty())
			return;

		file.WriteLine(string.Format(" %1 {", listName));
		foreach (string line : lines)
		{
			file.WriteLine(line);
		}
		file.WriteLine(" }");
	}

	//------------------------------------------------------------------------------------------------
	//! Lower-case file name of a prefab, for name hints
	protected static string LowerName(ResourceName prefab)
	{
		string path = prefab;
		int slash = path.LastIndexOf("/");
		if (slash >= 0)
			path = path.Substring(slash + 1, path.Length() - slash - 1);

		path.ToLower();
		return path;
	}
}
