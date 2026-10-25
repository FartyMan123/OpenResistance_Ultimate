//! One thing a faction preset makes available: a prefab and its base cost (see AROR_FactionPreset).
//! Each category below adds its own type. Presets only list what exists; the systems using them decide what to pick.
[BaseContainerProps(insertable: false)]
class AROR_FactionPresetEntry
{
	[Attribute("", UIWidgets.ResourcePickerThumbnail, desc: "Prefab", params: "et")]
	protected ResourceName m_sPrefab;

	[Attribute("0", desc: "Base cost in money", params: "0 inf")]
	protected int m_iCost;

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
}

//! AI character. A preset has at most one unit per type.
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(AROR_EUnitType, "m_eType")]
class AROR_FactionPresetUnit : AROR_FactionPresetEntry
{
	[Attribute("0", UIWidgets.ComboBox, desc: "Unit type (one unit per type in a preset)", enums: ParamEnumArray.FromEnum(AROR_EUnitType))]
	protected AROR_EUnitType m_eType;

	//------------------------------------------------------------------------------------------------
	AROR_EUnitType GetUnitType()
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
	AROR_EVehicleType GetVehicleType()
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
	AROR_EGearType GetGearType()
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
	AROR_EEquipmentType GetEquipmentType()
	{
		return m_eType;
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(AROR_EWeaponType, "m_eType")]
class AROR_FactionPresetWeapon : AROR_FactionPresetEntry
{
	[Attribute("0", UIWidgets.ComboBox, desc: "Weapon type", enums: ParamEnumArray.FromEnum(AROR_EWeaponType))]
	protected AROR_EWeaponType m_eType;

	//------------------------------------------------------------------------------------------------
	AROR_EWeaponType GetWeaponType()
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
	AROR_EWeaponAccessoryType GetAccessoryType()
	{
		return m_eType;
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(AROR_EAmmunitionType, "m_eType")]
class AROR_FactionPresetAmmunition : AROR_FactionPresetEntry
{
	[Attribute("0", UIWidgets.ComboBox, desc: "Ammunition type", enums: ParamEnumArray.FromEnum(AROR_EAmmunitionType))]
	protected AROR_EAmmunitionType m_eType;

	//------------------------------------------------------------------------------------------------
	AROR_EAmmunitionType GetAmmunitionType()
	{
		return m_eType;
	}
}
