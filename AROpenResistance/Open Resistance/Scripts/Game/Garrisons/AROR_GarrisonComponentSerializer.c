//! Per-location results, never seeds or entity refs. Existing location Persistence owns this section.
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
		context.WriteValue("version", 2);
		context.WriteValue("role", garrison.GetRosterRole());
		context.WriteValue("faction", garrison.GetRosterFaction());
		context.WriteValue("assetFaction", garrison.GetAssetFaction());
		context.WriteValue("level", garrison.GetLevel());
		context.WriteValue("count", slots.Count());
		foreach (int index, AROR_GarrisonSlot slot : slots)
		{
			string key = string.Format("slot%1_", index);
			context.WriteValue(key + "prefab", slot.m_sPrefab);
			context.WriteValue(key + "kind", slot.m_iKind);
			context.WriteValue(key + "group", slot.m_iGroup);
			context.WriteValue(key + "vehicle", slot.m_iVehicle);
			context.WriteValue(key + "seat", slot.m_iSeat);
			context.WriteValue(key + "lost", slot.m_bLost);
			context.WriteValue(key + "health", slot.m_fHealth);
			context.WriteValue(key + "placed", slot.m_bPlaced);
			context.WriteValue(key + "position", slot.m_vPosition);
			context.WriteValue(key + "angles", slot.m_vAngles);
		}
		return ESerializeResult.OK;
	}
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull LoadContext context)
	{
		int version, role, level, count;
		string faction, assetFaction;
		if (!context.ReadValue("version", version) || version < 1 || version > 2 || !context.ReadValue("role", role) || !context.ReadValue("faction", faction) || !context.ReadValue("assetFaction", assetFaction) || !context.ReadValue("level", level) || !context.ReadValue("count", count))
			return false;
		if (!AROR_GarrisonPolicy.IsEnemyRole(role) || level < 1 || level > 4 || count < 1 || count > 128 || faction.IsEmpty())
			return false;
		array<ref AROR_GarrisonSlot> slots = {};
		for (int index = 0; index < count; index++)
		{
			AROR_GarrisonSlot slot = new AROR_GarrisonSlot();
			string key = string.Format("slot%1_", index);
			if (!context.ReadValue(key + "prefab", slot.m_sPrefab) || !context.ReadValue(key + "kind", slot.m_iKind) || !context.ReadValue(key + "group", slot.m_iGroup) || !context.ReadValue(key + "vehicle", slot.m_iVehicle) || !context.ReadValue(key + "seat", slot.m_iSeat) || !context.ReadValue(key + "lost", slot.m_bLost) || !context.ReadValue(key + "health", slot.m_fHealth) || !context.ReadValue(key + "placed", slot.m_bPlaced) || !context.ReadValue(key + "position", slot.m_vPosition) || !context.ReadValue(key + "angles", slot.m_vAngles))
				return false;
			if (slot.m_sPrefab.IsEmpty() || slot.m_iKind < 0 || slot.m_iKind > 4 || slot.m_iVehicle >= count || slot.m_iVehicle < -1 || slot.m_iGroup < -1 || slot.m_iGroup > 32 || slot.m_fHealth < 0 || slot.m_fHealth > 1)
				return false;
			slots.Insert(slot);
			if (version == 1)
				slot.m_bPlaced = false; // Keep losses/health, replace obsolete scattered placement on next activation.
		}
		foreach (AROR_GarrisonSlot saved : slots)
		{
			if (saved.m_iKind != 2 && saved.m_iGroup < 0)
				return false;
			if (saved.m_iKind == 4)
			{
				if (saved.m_iVehicle < 0)
					return false;
				AROR_GarrisonSlot vehicle = slots[saved.m_iVehicle];
				if ((vehicle.m_iKind != 1 && vehicle.m_iKind != 3) || vehicle.m_iGroup != saved.m_iGroup || saved.m_iSeat < 0 || saved.m_iSeat > 32)
					return false;
			}
		}
		AROR_GarrisonComponent.Cast(component).RestoreRoster_S(role, faction, assetFaction, level, slots);
		return true;
	}
}
