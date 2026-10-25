//! A faction preset (see ARCHITECTURE.md -> Factions): everything an army makes available to the game, in seven
//! categories, each entry with a base cost. A preset only lists what exists; garrisons, QRFs, shops and other systems
//! decide what to use. One `.conf` per preset, listed in AROR_FactionPresetRegistry; extender mods add their own.
[BaseContainerProps(configRoot: true)]
class AROR_FactionPreset
{
	[Attribute("", desc: "Unique ID, e.g. US_ARMY. Saved with campaigns: never change it once released.")]
	protected string m_sId;

	[Attribute("", desc: "Name shown to the admin when choosing presets")]
	protected string m_sName;

	[Attribute("", UIWidgets.ComboBox, desc: "Roles this preset can be chosen for", enums: ParamEnumArray.FromEnum(AROR_EFactionRole))]
	protected ref array<AROR_EFactionRole> m_aUsableRoles;

	[Attribute(desc: "AI units: at most one per unit type", category: "Units")]
	protected ref array<ref AROR_FactionPresetUnit> m_aUnits;

	[Attribute(desc: "Empty vehicles (no crew)", category: "Vehicles")]
	protected ref array<ref AROR_FactionPresetVehicle> m_aVehicles;

	[Attribute(desc: "Clothing and carried kit", category: "Gear")]
	protected ref array<ref AROR_FactionPresetGear> m_aGear;

	[Attribute(desc: "Medical supplies, binoculars, radios and other equipment", category: "Equipment")]
	protected ref array<ref AROR_FactionPresetEquipment> m_aEquipment;

	[Attribute(desc: "Weapons", category: "Weapons")]
	protected ref array<ref AROR_FactionPresetWeapon> m_aWeapons;

	[Attribute(desc: "Weapon accessories", category: "Weapon accessories")]
	protected ref array<ref AROR_FactionPresetWeaponAccessory> m_aWeaponAccessories;

	[Attribute(desc: "Ammunition", category: "Ammunition")]
	protected ref array<ref AROR_FactionPresetAmmunition> m_aAmmunition;

	//------------------------------------------------------------------------------------------------
	string GetId()
	{
		return m_sId;
	}

	//------------------------------------------------------------------------------------------------
	string GetName()
	{
		return m_sName;
	}

	//------------------------------------------------------------------------------------------------
	bool IsUsableAs(AROR_EFactionRole role)
	{
		return m_aUsableRoles && m_aUsableRoles.Contains(role);
	}

	//------------------------------------------------------------------------------------------------
	// Units
	//------------------------------------------------------------------------------------------------
	//! The unit of this type, or null if the preset has none
	AROR_FactionPresetUnit GetUnit(AROR_EUnitType type)
	{
		if (!m_aUnits)
			return null;

		foreach (AROR_FactionPresetUnit unit : m_aUnits)
		{
			if (unit && unit.GetUnitType() == type)
				return unit;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Adds every unit to outUnits; returns how many were added
	int GetUnits(notnull array<AROR_FactionPresetUnit> outUnits)
	{
		int added;
		if (m_aUnits)
		{
			foreach (AROR_FactionPresetUnit entry : m_aUnits)
			{
				if (entry)
				{
					outUnits.Insert(entry);
					added++;
				}
			}
		}

		return added;
	}

	//------------------------------------------------------------------------------------------------
	// Vehicles, gear, equipment, weapons, accessories, ammunition: many entries per type.
	// Each getter adds the entries of one type to the given array and returns how many were added.
	//------------------------------------------------------------------------------------------------
	int GetVehicles(AROR_EVehicleType type, notnull array<AROR_FactionPresetVehicle> outVehicles)
	{
		int added;
		if (m_aVehicles)
		{
			foreach (AROR_FactionPresetVehicle entry : m_aVehicles)
			{
				if (entry && entry.GetVehicleType() == type)
				{
					outVehicles.Insert(entry);
					added++;
				}
			}
		}

		return added;
	}

	//------------------------------------------------------------------------------------------------
	int GetGear(AROR_EGearType type, notnull array<AROR_FactionPresetGear> outGear)
	{
		int added;
		if (m_aGear)
		{
			foreach (AROR_FactionPresetGear entry : m_aGear)
			{
				if (entry && entry.GetGearType() == type)
				{
					outGear.Insert(entry);
					added++;
				}
			}
		}

		return added;
	}

	//------------------------------------------------------------------------------------------------
	int GetEquipment(AROR_EEquipmentType type, notnull array<AROR_FactionPresetEquipment> outEquipment)
	{
		int added;
		if (m_aEquipment)
		{
			foreach (AROR_FactionPresetEquipment entry : m_aEquipment)
			{
				if (entry && entry.GetEquipmentType() == type)
				{
					outEquipment.Insert(entry);
					added++;
				}
			}
		}

		return added;
	}

	//------------------------------------------------------------------------------------------------
	int GetWeapons(AROR_EWeaponType type, notnull array<AROR_FactionPresetWeapon> outWeapons)
	{
		int added;
		if (m_aWeapons)
		{
			foreach (AROR_FactionPresetWeapon entry : m_aWeapons)
			{
				if (entry && entry.GetWeaponType() == type)
				{
					outWeapons.Insert(entry);
					added++;
				}
			}
		}

		return added;
	}

	//------------------------------------------------------------------------------------------------
	int GetWeaponAccessories(AROR_EWeaponAccessoryType type, notnull array<AROR_FactionPresetWeaponAccessory> outAccessories)
	{
		int added;
		if (m_aWeaponAccessories)
		{
			foreach (AROR_FactionPresetWeaponAccessory entry : m_aWeaponAccessories)
			{
				if (entry && entry.GetAccessoryType() == type)
				{
					outAccessories.Insert(entry);
					added++;
				}
			}
		}

		return added;
	}

	//------------------------------------------------------------------------------------------------
	int GetAmmunition(AROR_EAmmunitionType type, notnull array<AROR_FactionPresetAmmunition> outAmmunition)
	{
		int added;
		if (m_aAmmunition)
		{
			foreach (AROR_FactionPresetAmmunition entry : m_aAmmunition)
			{
				if (entry && entry.GetAmmunitionType() == type)
				{
					outAmmunition.Insert(entry);
					added++;
				}
			}
		}

		return added;
	}

	//------------------------------------------------------------------------------------------------
	// All entries of a category (e.g. for a shop listing everything)
	//------------------------------------------------------------------------------------------------
	int GetAllVehicles(notnull array<AROR_FactionPresetVehicle> outVehicles)
	{
		int added;
		if (m_aVehicles)
		{
			foreach (AROR_FactionPresetVehicle entry : m_aVehicles)
			{
				if (entry)
				{
					outVehicles.Insert(entry);
					added++;
				}
			}
		}

		return added;
	}

	//------------------------------------------------------------------------------------------------
	int GetAllGear(notnull array<AROR_FactionPresetGear> outGear)
	{
		int added;
		if (m_aGear)
		{
			foreach (AROR_FactionPresetGear entry : m_aGear)
			{
				if (entry)
				{
					outGear.Insert(entry);
					added++;
				}
			}
		}

		return added;
	}

	//------------------------------------------------------------------------------------------------
	int GetAllEquipment(notnull array<AROR_FactionPresetEquipment> outEquipment)
	{
		int added;
		if (m_aEquipment)
		{
			foreach (AROR_FactionPresetEquipment entry : m_aEquipment)
			{
				if (entry)
				{
					outEquipment.Insert(entry);
					added++;
				}
			}
		}

		return added;
	}

	//------------------------------------------------------------------------------------------------
	int GetAllWeapons(notnull array<AROR_FactionPresetWeapon> outWeapons)
	{
		int added;
		if (m_aWeapons)
		{
			foreach (AROR_FactionPresetWeapon entry : m_aWeapons)
			{
				if (entry)
				{
					outWeapons.Insert(entry);
					added++;
				}
			}
		}

		return added;
	}

	//------------------------------------------------------------------------------------------------
	int GetAllWeaponAccessories(notnull array<AROR_FactionPresetWeaponAccessory> outAccessories)
	{
		int added;
		if (m_aWeaponAccessories)
		{
			foreach (AROR_FactionPresetWeaponAccessory entry : m_aWeaponAccessories)
			{
				if (entry)
				{
					outAccessories.Insert(entry);
					added++;
				}
			}
		}

		return added;
	}

	//------------------------------------------------------------------------------------------------
	int GetAllAmmunition(notnull array<AROR_FactionPresetAmmunition> outAmmunition)
	{
		int added;
		if (m_aAmmunition)
		{
			foreach (AROR_FactionPresetAmmunition entry : m_aAmmunition)
			{
				if (entry)
				{
					outAmmunition.Insert(entry);
					added++;
				}
			}
		}

		return added;
	}

	//------------------------------------------------------------------------------------------------
	//! Logs configuration mistakes (missing ID, unknown or duplicate types, empty prefabs) and drops entries with unknown types.
	//! Returns false if the preset is unusable.
	bool Validate(string source)
	{
		if (m_sId.IsEmpty())
		{
			Print(string.Format("[AROR] Faction preset '%1' has no ID and is ignored", source), LogLevel.ERROR);
			return false;
		}

		if (!m_aUsableRoles || m_aUsableRoles.IsEmpty())
			Print(string.Format("[AROR] Faction preset '%1' has no usable roles", m_sId), LogLevel.WARNING);

		// Only the core types are used; entries with any other type (e.g. from a modded enum) are dropped
		if (m_aUnits)
		{
			for (int i = m_aUnits.Count() - 1; i >= 0; i--)
			{
				AROR_FactionPresetUnit entry = m_aUnits[i];
				if (entry && entry.GetUnitType() > AROR_EUnitType.CIVILIAN)
				{
					Print(string.Format("[AROR] Faction preset '%1' has a unit entry with an unknown type (%2); it is ignored", m_sId, entry.GetUnitType()), LogLevel.WARNING);
					m_aUnits.Remove(i);
				}
			}
		}

		if (m_aVehicles)
		{
			for (int i = m_aVehicles.Count() - 1; i >= 0; i--)
			{
				AROR_FactionPresetVehicle entry = m_aVehicles[i];
				if (entry && entry.GetVehicleType() > AROR_EVehicleType.STATIC_WEAPON)
				{
					Print(string.Format("[AROR] Faction preset '%1' has a vehicle entry with an unknown type (%2); it is ignored", m_sId, entry.GetVehicleType()), LogLevel.WARNING);
					m_aVehicles.Remove(i);
				}
			}
		}

		if (m_aGear)
		{
			for (int i = m_aGear.Count() - 1; i >= 0; i--)
			{
				AROR_FactionPresetGear entry = m_aGear[i];
				if (entry && entry.GetGearType() > AROR_EGearType.FACEWEAR)
				{
					Print(string.Format("[AROR] Faction preset '%1' has a gear entry with an unknown type (%2); it is ignored", m_sId, entry.GetGearType()), LogLevel.WARNING);
					m_aGear.Remove(i);
				}
			}
		}

		if (m_aEquipment)
		{
			for (int i = m_aEquipment.Count() - 1; i >= 0; i--)
			{
				AROR_FactionPresetEquipment entry = m_aEquipment[i];
				if (entry && entry.GetEquipmentType() > AROR_EEquipmentType.EXPLOSIVE)
				{
					Print(string.Format("[AROR] Faction preset '%1' has a equipment entry with an unknown type (%2); it is ignored", m_sId, entry.GetEquipmentType()), LogLevel.WARNING);
					m_aEquipment.Remove(i);
				}
			}
		}

		if (m_aWeapons)
		{
			for (int i = m_aWeapons.Count() - 1; i >= 0; i--)
			{
				AROR_FactionPresetWeapon entry = m_aWeapons[i];
				if (entry && entry.GetWeaponType() > AROR_EWeaponType.GRENADE)
				{
					Print(string.Format("[AROR] Faction preset '%1' has a weapon entry with an unknown type (%2); it is ignored", m_sId, entry.GetWeaponType()), LogLevel.WARNING);
					m_aWeapons.Remove(i);
				}
			}
		}

		if (m_aWeaponAccessories)
		{
			for (int i = m_aWeaponAccessories.Count() - 1; i >= 0; i--)
			{
				AROR_FactionPresetWeaponAccessory entry = m_aWeaponAccessories[i];
				if (entry && entry.GetAccessoryType() > AROR_EWeaponAccessoryType.LIGHT)
				{
					Print(string.Format("[AROR] Faction preset '%1' has a weapon accessory entry with an unknown type (%2); it is ignored", m_sId, entry.GetAccessoryType()), LogLevel.WARNING);
					m_aWeaponAccessories.Remove(i);
				}
			}
		}

		if (m_aAmmunition)
		{
			for (int i = m_aAmmunition.Count() - 1; i >= 0; i--)
			{
				AROR_FactionPresetAmmunition entry = m_aAmmunition[i];
				if (entry && entry.GetAmmunitionType() > AROR_EAmmunitionType.GRENADE_ROUND)
				{
					Print(string.Format("[AROR] Faction preset '%1' has a ammunition entry with an unknown type (%2); it is ignored", m_sId, entry.GetAmmunitionType()), LogLevel.WARNING);
					m_aAmmunition.Remove(i);
				}
			}
		}


		if (m_aUnits)
		{
			set<AROR_EUnitType> seen = new set<AROR_EUnitType>();
			foreach (AROR_FactionPresetUnit unit : m_aUnits)
			{
				if (unit && !seen.Insert(unit.GetUnitType()))
					Print(string.Format("[AROR] Faction preset '%1' lists unit type %2 more than once; only the first is used", m_sId, typename.EnumToString(AROR_EUnitType, unit.GetUnitType())), LogLevel.WARNING);
			}
		}

		int empty;
		if (m_aUnits)
		{
			foreach (AROR_FactionPresetUnit entry : m_aUnits)
			{
				if (!entry || entry.GetPrefab().IsEmpty())
					empty++;
			}
		}

		if (m_aVehicles)
		{
			foreach (AROR_FactionPresetVehicle entry : m_aVehicles)
			{
				if (!entry || entry.GetPrefab().IsEmpty())
					empty++;
			}
		}

		if (m_aGear)
		{
			foreach (AROR_FactionPresetGear entry : m_aGear)
			{
				if (!entry || entry.GetPrefab().IsEmpty())
					empty++;
			}
		}

		if (m_aEquipment)
		{
			foreach (AROR_FactionPresetEquipment entry : m_aEquipment)
			{
				if (!entry || entry.GetPrefab().IsEmpty())
					empty++;
			}
		}

		if (m_aWeapons)
		{
			foreach (AROR_FactionPresetWeapon entry : m_aWeapons)
			{
				if (!entry || entry.GetPrefab().IsEmpty())
					empty++;
			}
		}

		if (m_aWeaponAccessories)
		{
			foreach (AROR_FactionPresetWeaponAccessory entry : m_aWeaponAccessories)
			{
				if (!entry || entry.GetPrefab().IsEmpty())
					empty++;
			}
		}

		if (m_aAmmunition)
		{
			foreach (AROR_FactionPresetAmmunition entry : m_aAmmunition)
			{
				if (!entry || entry.GetPrefab().IsEmpty())
					empty++;
			}
		}

		if (empty > 0)
			Print(string.Format("[AROR] Faction preset '%1' has %2 entries without a prefab", m_sId, empty), LogLevel.WARNING);

		return true;
	}
}
