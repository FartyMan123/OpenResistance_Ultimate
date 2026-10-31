//! One thing a faction preset makes available: a prefab of a type (see AROR_FactionPreset).
//! Each category below adds its own type. Prices, readiness and import requirements are not here: they belong to
//! content packs (see AROR_ContentPack). Presets only list what exists; the systems using them decide what to pick.
[BaseContainerProps(insertable: false)]
class AROR_FactionPresetEntry
{
	[Attribute("", UIWidgets.ResourcePickerThumbnail, desc: "Prefab", params: "et")]
	protected ResourceName m_sPrefab;

	//------------------------------------------------------------------------------------------------
	//! The prefab to use: the listed one, or its replacement from an enabled content pack (e.g. a weapon mod's variant)
	ResourceName GetPrefab()
	{
		return AROR_ContentPacksComponent.Resolve(m_sPrefab);
	}

	//------------------------------------------------------------------------------------------------
	//! The prefab exactly as listed in the preset, ignoring content pack replacements
	ResourceName GetListedPrefab()
	{
		return m_sPrefab;
	}

	//------------------------------------------------------------------------------------------------
	AROR_EFactionPresetCategory GetCategory()
	{
		return -1;
	}

	//------------------------------------------------------------------------------------------------
	//! The entry's type within its category, as the int value of that category's type enum
	int GetTypeValue()
	{
		return -1;
	}

	//------------------------------------------------------------------------------------------------
	//! False for types outside the core list (e.g. added by a modded enum); such entries are ignored
	bool IsKnownType()
	{
		return AROR_PresetTypes.IsKnownType(GetCategory(), GetTypeValue());
	}

	//------------------------------------------------------------------------------------------------
	//! Type name for logs, e.g. "RIFLEMAN"
	string GetTypeName()
	{
		return AROR_PresetTypes.GetTypeName(GetCategory(), GetTypeValue());
	}
}

//! AI character. A preset has at most one unit per type.
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(AROR_EUnitType, "m_eType")]
class AROR_FactionPresetUnit : AROR_FactionPresetEntry
{
	[Attribute("0", UIWidgets.ComboBox, desc: "Unit type (one unit per type in a preset)", enums: ParamEnumArray.FromEnum(AROR_EUnitType))]
	protected AROR_EUnitType m_eType;

	//------------------------------------------------------------------------------------------------
	override AROR_EFactionPresetCategory GetCategory()
	{
		return AROR_EFactionPresetCategory.UNITS;
	}

	//------------------------------------------------------------------------------------------------
	override int GetTypeValue()
	{
		return m_eType;
	}
}

//! Empty vehicle (no crew)
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(AROR_EVehicleType, "m_eType")]
class AROR_FactionPresetVehicle : AROR_FactionPresetEntry
{
	[Attribute("0", UIWidgets.ComboBox, desc: "Vehicle type", enums: ParamEnumArray.FromEnum(AROR_EVehicleType))]
	protected AROR_EVehicleType m_eType;

	//------------------------------------------------------------------------------------------------
	override AROR_EFactionPresetCategory GetCategory()
	{
		return AROR_EFactionPresetCategory.VEHICLES;
	}

	//------------------------------------------------------------------------------------------------
	override int GetTypeValue()
	{
		return m_eType;
	}
}

//! Clothing and carried kit
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(AROR_EGearType, "m_eType")]
class AROR_FactionPresetGear : AROR_FactionPresetEntry
{
	[Attribute("0", UIWidgets.ComboBox, desc: "Gear type", enums: ParamEnumArray.FromEnum(AROR_EGearType))]
	protected AROR_EGearType m_eType;

	//------------------------------------------------------------------------------------------------
	override AROR_EFactionPresetCategory GetCategory()
	{
		return AROR_EFactionPresetCategory.GEAR;
	}

	//------------------------------------------------------------------------------------------------
	override int GetTypeValue()
	{
		return m_eType;
	}
}

//! Medical supplies, binoculars, radios and other equipment
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(AROR_EEquipmentType, "m_eType")]
class AROR_FactionPresetEquipment : AROR_FactionPresetEntry
{
	[Attribute("0", UIWidgets.ComboBox, desc: "Equipment type", enums: ParamEnumArray.FromEnum(AROR_EEquipmentType))]
	protected AROR_EEquipmentType m_eType;

	//------------------------------------------------------------------------------------------------
	override AROR_EFactionPresetCategory GetCategory()
	{
		return AROR_EFactionPresetCategory.EQUIPMENT;
	}

	//------------------------------------------------------------------------------------------------
	override int GetTypeValue()
	{
		return m_eType;
	}
}

//! Hand-held weapon
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(AROR_EWeaponType, "m_eType")]
class AROR_FactionPresetWeapon : AROR_FactionPresetEntry
{
	[Attribute("0", UIWidgets.ComboBox, desc: "Weapon type", enums: ParamEnumArray.FromEnum(AROR_EWeaponType))]
	protected AROR_EWeaponType m_eType;

	//------------------------------------------------------------------------------------------------
	override AROR_EFactionPresetCategory GetCategory()
	{
		return AROR_EFactionPresetCategory.WEAPONS;
	}

	//------------------------------------------------------------------------------------------------
	override int GetTypeValue()
	{
		return m_eType;
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(AROR_EWeaponAccessoryType, "m_eType")]
class AROR_FactionPresetWeaponAccessory : AROR_FactionPresetEntry
{
	[Attribute("0", UIWidgets.ComboBox, desc: "Weapon accessory type", enums: ParamEnumArray.FromEnum(AROR_EWeaponAccessoryType))]
	protected AROR_EWeaponAccessoryType m_eType;

	//------------------------------------------------------------------------------------------------
	override AROR_EFactionPresetCategory GetCategory()
	{
		return AROR_EFactionPresetCategory.WEAPON_ACCESSORIES;
	}

	//------------------------------------------------------------------------------------------------
	override int GetTypeValue()
	{
		return m_eType;
	}
}

//! Ammunition, including munitions loaded onto vehicles and aircraft as items (e.g. a helicopter rocket pod)
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(AROR_EAmmunitionType, "m_eType")]
class AROR_FactionPresetAmmunition : AROR_FactionPresetEntry
{
	[Attribute("0", UIWidgets.ComboBox, desc: "Ammunition type", enums: ParamEnumArray.FromEnum(AROR_EAmmunitionType))]
	protected AROR_EAmmunitionType m_eType;

	//------------------------------------------------------------------------------------------------
	override AROR_EFactionPresetCategory GetCategory()
	{
		return AROR_EFactionPresetCategory.AMMUNITION;
	}

	//------------------------------------------------------------------------------------------------
	override int GetTypeValue()
	{
		return m_eType;
	}
}
