//! Versioned logical roster results. Layout and current faction assets are scenario data.
class AROR_GarrisonComponentSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType() { return AROR_GarrisonComponent; }
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull SaveContext context)
	{
		AROR_GarrisonComponent garrison = AROR_GarrisonComponent.Cast(component);
		if (!garrison.HasRoster())
			return ESerializeResult.DEFAULT;
		garrison.Observe_S(false);
		array<ref AROR_GarrisonSlot> slots = {};
		garrison.GetSlots(slots);
		context.WriteValue("version", 3);
		context.WriteValue("role", garrison.GetRosterRole());
		context.WriteValue("level", garrison.GetLevel());
		context.WriteValue("quarantined", garrison.IsRestoreQuarantined());
		context.WriteValue("count", slots.Count());
		foreach (int index, AROR_GarrisonSlot slot : slots)
		{
			string key = string.Format("slot%1_", index);
			context.WriteValue(key + "kind", slot.m_iKind);
			context.WriteValue(key + "variant", slot.m_iVariant);
			context.WriteValue(key + "crewRecorded", slot.m_bCrewRecorded);
			context.WriteValue(key + "group", slot.m_iGroup);
			context.WriteValue(key + "vehicle", slot.m_iVehicle);
			context.WriteValue(key + "seat", slot.m_iSeat);
			context.WriteValue(key + "lost", slot.m_bLost);
			context.WriteValue(key + "health", slot.m_fHealth);
		}
		return ESerializeResult.OK;
	}

	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull LoadContext context)
	{
		AROR_GarrisonComponent garrison = AROR_GarrisonComponent.Cast(component);
		int version, role, count;
		if (!context.ReadValue("version", version) || version < 1 || version > 3 || !context.ReadValue("role", role) || !AROR_GarrisonPolicy.IsEnemyRole(role) || !context.ReadValue("count", count) || count < 0 || count > 128)
		{
			garrison.QuarantineRoster_S("invalid or unsupported header");
			return true;
		}
		bool quarantined;
		context.ReadValue("quarantined", quarantined);
		if (quarantined || count == 0)
		{
			garrison.QuarantineRoster_S("saved quarantine or empty roster");
			return true;
		}
		int level = 1;
		context.ReadValue("level", level);
		level = Math.Clamp(level, 1, 4);
		array<ref AROR_GarrisonSlot> slots = {};
		int damaged;
		for (int index = 0; index < count; index++)
		{
			AROR_GarrisonSlot slot = new AROR_GarrisonSlot();
			string key = string.Format("slot%1_", index);
			bool invalid;
			if (!context.ReadValue(key + "kind", slot.m_iKind) || slot.m_iKind < 0 || slot.m_iKind > 4)
			{
				slot.m_iKind = 0;
				invalid = true;
			}
			context.ReadValue(key + "group", slot.m_iGroup);
			context.ReadValue(key + "vehicle", slot.m_iVehicle);
			context.ReadValue(key + "seat", slot.m_iSeat);
			context.ReadValue(key + "crewRecorded", slot.m_bCrewRecorded);
			if (!context.ReadValue(key + "lost", slot.m_bLost))
				invalid = true; // A missing casualty flag must not create a living defender.
			if (!context.ReadValue(key + "health", slot.m_fHealth) || slot.m_fHealth < 0 || slot.m_fHealth > 1 || slot.m_fHealth != slot.m_fHealth)
			{
				slot.m_fHealth = 0;
				invalid = true;
			}
			if (slot.m_iKind != 2 && (slot.m_iGroup < 0 || slot.m_iGroup > 127))
			{
				slot.m_iGroup = 0;
				invalid = true;
			}
			if (version < 3)
			{
				context.ReadValue(key + "prefab", slot.m_sPrefab);
				slot.m_bLegacyAsset = true; // Optional hint only; old asset paths never gate loading.
			}
			else
				context.ReadValue(key + "variant", slot.m_iVariant);
			slot.m_iVariant = Math.Max(0, slot.m_iVariant);
			if (invalid)
			{
				slot.m_bLost = true;
				damaged++;
			}
			if (slot.m_fHealth == 0)
				slot.m_bLost = true;
			// No saved transforms are used, including legacy placed/position/angles fields.
			slot.m_bPlaced = false;
			slots.Insert(slot);
		}
		foreach (AROR_GarrisonSlot saved : slots)
		{
			if (version < 3 && damaged > 0 && (saved.m_iKind == 1 || saved.m_iKind == 3))
				saved.m_bCrewRecorded = true; // Ambiguous legacy damage cannot grant missing replacement crew.
			if (saved.m_iKind != 4)
			{
				saved.m_iVehicle = -1;
				saved.m_iSeat = -1;
				continue;
			}
			bool orphan = saved.m_iVehicle < 0 || saved.m_iVehicle >= count || saved.m_iSeat < 0 || saved.m_iSeat > 32;
			if (!orphan)
			{
				AROR_GarrisonSlot vehicle = slots[saved.m_iVehicle];
				orphan = (vehicle.m_iKind != 1 && vehicle.m_iKind != 3) || vehicle.m_bLost || vehicle.m_iGroup != saved.m_iGroup;
			}
			if (orphan)
			{
				saved.m_iVehicle = -1;
				saved.m_iSeat = -1; // Surviving crew remain defenders on foot.
			}
		}
		if (damaged > 0)
			Print(string.Format("[AROR GARRISON] %1 damaged saved slots quarantined; other casualties/health preserved", damaged), LogLevel.WARNING);
		garrison.RestoreRoster_S(role, string.Empty, string.Empty, level, slots);
		return true;
	}
}
