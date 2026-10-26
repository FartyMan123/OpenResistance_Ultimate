//! One thing a faction preset makes available: a prefab and its base cost (see AROR_FactionPreset).
//! Each category below adds its own type. Presets only list what exists; the systems using them decide what to pick.
[BaseContainerProps(insertable: false)]
class AROR_FactionPresetEntry
{
	[Attribute("", UIWidgets.ResourcePickerThumbnail, desc: "Prefab", params: "et")]
	protected ResourceName m_sPrefab;

	[Attribute("0", desc: "Base cost in money", params: "0 inf")]
	protected int m_iCost;

	[Attribute("1", desc: "War level at which this asset becomes available", params: "1 5 1")]
	protected int m_iMinimumWarLevel;

	//------------------------------------------------------------------------------------------------
	ResourceName GetPrefab()
	{
		return m_sPrefab;
	}

	//------------------------------------------------------------------------------------------------
	int GetCost()
	{
		return m_iCost;
	}

	//------------------------------------------------------------------------------------------------
	int GetMinimumWarLevel()
	{
		return m_iMinimumWarLevel;
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
		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Type name for logs, e.g. "RIFLEMAN"
	string GetTypeName()
	{
		return string.Empty;
	}
}

//! AI character. A preset has at most one unit per type.
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(AROR_EUnitType, "m_eType")]
class AROR_FactionPresetUnit : AROR_FactionPresetEntry
{
	[Attribute("0", UIWidgets.ComboBox, desc: "Unit type (one unit per type in a preset)", enums: ParamEnumArray.FromEnum(AROR_EUnitType))]
	protected AROR_EUnitType m_eType;

	//------------------------------------------------------------------------------------------------
	override int GetTypeValue()
	{
		return m_eType;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsKnownType()
	{
		return m_eType >= 0 && m_eType <= AROR_EUnitType.CIVILIAN;
	}

	//------------------------------------------------------------------------------------------------
	override string GetTypeName()
	{
		return typename.EnumToString(AROR_EUnitType, m_eType);
	}
}

//! Empty vehicle (no crew)
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(AROR_EVehicleType, "m_eType")]
class AROR_FactionPresetVehicle : AROR_FactionPresetEntry
{
	[Attribute("0", UIWidgets.ComboBox, desc: "Vehicle type", enums: ParamEnumArray.FromEnum(AROR_EVehicleType))]
	protected AROR_EVehicleType m_eType;

	//------------------------------------------------------------------------------------------------
	override int GetTypeValue()
	{
		return m_eType;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsKnownType()
	{
		return m_eType >= 0 && m_eType <= AROR_EVehicleType.STATIC_ARTILLERY;
	}

	//------------------------------------------------------------------------------------------------
	override string GetTypeName()
	{
		return typename.EnumToString(AROR_EVehicleType, m_eType);
	}
}

//! Clothing and carried kit
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(AROR_EGearType, "m_eType")]
class AROR_FactionPresetGear : AROR_FactionPresetEntry
{
	[Attribute("0", UIWidgets.ComboBox, desc: "Gear type", enums: ParamEnumArray.FromEnum(AROR_EGearType))]
	protected AROR_EGearType m_eType;

	//------------------------------------------------------------------------------------------------
	override int GetTypeValue()
	{
		return m_eType;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsKnownType()
	{
		return m_eType >= 0 && m_eType <= AROR_EGearType.FACEWEAR;
	}

	//------------------------------------------------------------------------------------------------
	override string GetTypeName()
	{
		return typename.EnumToString(AROR_EGearType, m_eType);
	}
}

//! Medical supplies, binoculars, radios and other equipment
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(AROR_EEquipmentType, "m_eType")]
class AROR_FactionPresetEquipment : AROR_FactionPresetEntry
{
	[Attribute("0", UIWidgets.ComboBox, desc: "Equipment type", enums: ParamEnumArray.FromEnum(AROR_EEquipmentType))]
	protected AROR_EEquipmentType m_eType;

	//------------------------------------------------------------------------------------------------
	override int GetTypeValue()
	{
		return m_eType;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsKnownType()
	{
		return m_eType >= 0 && m_eType <= AROR_EEquipmentType.EXPLOSIVE;
	}

	//------------------------------------------------------------------------------------------------
	override string GetTypeName()
	{
		return typename.EnumToString(AROR_EEquipmentType, m_eType);
	}
}

//! Hand-held weapon
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(AROR_EWeaponType, "m_eType")]
class AROR_FactionPresetWeapon : AROR_FactionPresetEntry
{
	[Attribute("0", UIWidgets.ComboBox, desc: "Weapon type", enums: ParamEnumArray.FromEnum(AROR_EWeaponType))]
	protected AROR_EWeaponType m_eType;

	//------------------------------------------------------------------------------------------------
	override int GetTypeValue()
	{
		return m_eType;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsKnownType()
	{
		return m_eType >= 0 && m_eType <= AROR_EWeaponType.GRENADE;
	}

	//------------------------------------------------------------------------------------------------
	override string GetTypeName()
	{
		return typename.EnumToString(AROR_EWeaponType, m_eType);
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(AROR_EWeaponAccessoryType, "m_eType")]
class AROR_FactionPresetWeaponAccessory : AROR_FactionPresetEntry
{
	[Attribute("0", UIWidgets.ComboBox, desc: "Weapon accessory type", enums: ParamEnumArray.FromEnum(AROR_EWeaponAccessoryType))]
	protected AROR_EWeaponAccessoryType m_eType;

	//------------------------------------------------------------------------------------------------
	override int GetTypeValue()
	{
		return m_eType;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsKnownType()
	{
		return m_eType >= 0 && m_eType <= AROR_EWeaponAccessoryType.LIGHT;
	}

	//------------------------------------------------------------------------------------------------
	override string GetTypeName()
	{
		return typename.EnumToString(AROR_EWeaponAccessoryType, m_eType);
	}
}

//! Ammunition for hand-held weapons
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(AROR_EAmmunitionType, "m_eType")]
class AROR_FactionPresetAmmunition : AROR_FactionPresetEntry
{
	[Attribute("0", UIWidgets.ComboBox, desc: "Ammunition type", enums: ParamEnumArray.FromEnum(AROR_EAmmunitionType))]
	protected AROR_EAmmunitionType m_eType;

	//------------------------------------------------------------------------------------------------
	override int GetTypeValue()
	{
		return m_eType;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsKnownType()
	{
		return m_eType >= 0 && m_eType <= AROR_EAmmunitionType.GRENADE_ROUND;
	}

	//------------------------------------------------------------------------------------------------
	override string GetTypeName()
	{
		return typename.EnumToString(AROR_EAmmunitionType, m_eType);
	}
}

//! Munition loaded onto a vehicle or aircraft as an item (e.g. a helicopter rocket pod)
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(AROR_EVehicleAmmunitionType, "m_eType")]
class AROR_FactionPresetVehicleAmmunition : AROR_FactionPresetEntry
{
	[Attribute("0", UIWidgets.ComboBox, desc: "Vehicle ammunition type", enums: ParamEnumArray.FromEnum(AROR_EVehicleAmmunitionType))]
	protected AROR_EVehicleAmmunitionType m_eType;

	//------------------------------------------------------------------------------------------------
	override int GetTypeValue()
	{
		return m_eType;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsKnownType()
	{
		return m_eType >= 0 && m_eType <= AROR_EVehicleAmmunitionType.BOMB;
	}

	//------------------------------------------------------------------------------------------------
	override string GetTypeName()
	{
		return typename.EnumToString(AROR_EVehicleAmmunitionType, m_eType);
	}
}
