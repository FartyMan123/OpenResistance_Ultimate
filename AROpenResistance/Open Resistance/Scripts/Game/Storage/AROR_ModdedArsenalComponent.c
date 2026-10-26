//! Resistance Storage arsenals list every stocked prefab (qty > 0) instead of the faction catalog.
modded class SCR_ArsenalComponent
{
	//------------------------------------------------------------------------------------------------
	override bool GetAvailablePrefabs(out notnull array<ResourceName> availablePrefabs)
	{
		if (!AROR_ResistanceArsenalComponent.IsResistanceArsenal(GetOwner()))
			return super.GetAvailablePrefabs(availablePrefabs);

		AROR_ResistanceStorageComponent storage = AROR_ResistanceStorageComponent.GetInstance();
		if (!storage)
			return false;

		array<ResourceName> prefabs = {};
		array<int> quantities = {};
		storage.GetEntries(prefabs, quantities);

		int count = Math.Min(prefabs.Count(), quantities.Count());
		for (int i = 0; i < count; i++)
		{
			if (quantities[i] > 0 && !prefabs[i].IsEmpty())
				availablePrefabs.Insert(prefabs[i]);
		}

		return !availablePrefabs.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	override void RefreshArsenal(bool init = false, SCR_Faction faction = null)
	{
		if (!AROR_ResistanceArsenalComponent.IsResistanceArsenal(GetOwner()))
		{
			super.RefreshArsenal(init, faction);
			return;
		}

		ClearArsenal();

		array<ResourceName> arsenalPrefabs = {};
		GetAvailablePrefabs(arsenalPrefabs);

		// Always notify (including empty) so the arsenal inventory clears when stock is gone
		ScriptInvokerArsenalUpdated onUpdated = GetOnArsenalUpdated();
		if (onUpdated)
			onUpdated.Invoke(arsenalPrefabs);

		// Same client sync as vanilla RefreshArsenal; clients rebuild via GetAvailablePrefabs (stock replicates)
		if (!init && Replication.IsServer())
			Rpc(RPC_OnArsenalUpdated, GetSupportedArsenalItemTypes(), GetSupportedArsenalItemModes());
	}
}
