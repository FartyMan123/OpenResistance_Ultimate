//! Saves Resistance Garage unique vehicle snapshots (AROR_ResistanceGarageComponent on the game mode).
//! Full cargo trees and weapon ammo are flattened to parallel arrays. Nothing is written while empty.
class AROR_ResistanceGarageComponentSerializer : ScriptedComponentSerializer
{
	protected static const int VERSION = 1;

	//------------------------------------------------------------------------------------------------
	override static typename GetTargetType()
	{
		return AROR_ResistanceGarageComponent;
	}

	//------------------------------------------------------------------------------------------------
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull SaveContext context)
	{
		const AROR_ResistanceGarageComponent garage = AROR_ResistanceGarageComponent.Cast(component);
		if (garage.IsEmpty())
			return ESerializeResult.DEFAULT;

		array<ref AROR_GarageVehicleEntry> entries = {};
		garage.GetFullEntries_S(entries);

		array<int> entryIds = {};
		array<ResourceName> prefabs = {};
		array<float> healths = {};
		array<float> fuelFractions = {};

		array<int> fuelEntryIds = {};
		array<float> fuelAmounts = {};

		array<int> weaponEntryIds = {};
		array<int> weaponIndices = {};
		array<ResourceName> weaponMagPrefabs = {};
		array<int> weaponAmmoCounts = {};

		array<int> itemEntryIds = {};
		array<int> itemNodeIndices = {};
		array<int> itemParentIndices = {};
		array<ResourceName> itemPrefabs = {};
		array<int> itemAmmoCounts = {};
		array<float> itemHealths = {};

		foreach (AROR_GarageVehicleEntry entry : entries)
		{
			if (!entry)
				continue;

			entryIds.Insert(entry.m_iId);
			prefabs.Insert(entry.m_sPrefab);
			healths.Insert(entry.m_fHealthScaled);
			fuelFractions.Insert(entry.m_fFuelNormalized);

			foreach (float amount : entry.m_aFuelAmounts)
			{
				fuelEntryIds.Insert(entry.m_iId);
				fuelAmounts.Insert(amount);
			}

			foreach (AROR_GarageWeaponAmmoSnapshot ammo : entry.m_aWeaponAmmo)
			{
				if (!ammo)
					continue;

				weaponEntryIds.Insert(entry.m_iId);
				weaponIndices.Insert(ammo.m_iWeaponIndex);
				weaponMagPrefabs.Insert(ammo.m_sMagazinePrefab);
				weaponAmmoCounts.Insert(ammo.m_iAmmoCount);
			}

			array<int> nextNode = {};
			nextNode.Insert(0);
			foreach (AROR_GarageItemSnapshot root : entry.m_aCargoRoots)
			{
				FlattenItem(entry.m_iId, root, -1, nextNode, itemEntryIds, itemNodeIndices, itemParentIndices, itemPrefabs, itemAmmoCounts, itemHealths);
			}
		}

		context.WriteValue("version", VERSION);
		context.WriteValue("nextId", garage.GetNextId_S());
		context.WriteValue("entryIds", entryIds);
		context.WriteValue("prefabs", prefabs);
		context.WriteValue("healths", healths);
		context.WriteValue("fuelFractions", fuelFractions);
		context.WriteValue("fuelEntryIds", fuelEntryIds);
		context.WriteValue("fuelAmounts", fuelAmounts);
		context.WriteValue("weaponEntryIds", weaponEntryIds);
		context.WriteValue("weaponIndices", weaponIndices);
		context.WriteValue("weaponMagPrefabs", weaponMagPrefabs);
		context.WriteValue("weaponAmmoCounts", weaponAmmoCounts);
		context.WriteValue("itemEntryIds", itemEntryIds);
		context.WriteValue("itemNodeIndices", itemNodeIndices);
		context.WriteValue("itemParentIndices", itemParentIndices);
		context.WriteValue("itemPrefabs", itemPrefabs);
		context.WriteValue("itemAmmoCounts", itemAmmoCounts);
		context.WriteValue("itemHealths", itemHealths);
		return ESerializeResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull LoadContext context)
	{
		AROR_ResistanceGarageComponent garage = AROR_ResistanceGarageComponent.Cast(component);

		int version;
		context.ReadValue("version", version);
		if (version != VERSION)
			return true;

		int nextId = 1;
		context.ReadValue("nextId", nextId);

		array<int> entryIds = {};
		array<ResourceName> prefabs = {};
		array<float> healths = {};
		array<float> fuelFractions = {};
		array<int> fuelEntryIds = {};
		array<float> fuelAmounts = {};
		array<int> weaponEntryIds = {};
		array<int> weaponIndices = {};
		array<ResourceName> weaponMagPrefabs = {};
		array<int> weaponAmmoCounts = {};
		array<int> itemEntryIds = {};
		array<int> itemNodeIndices = {};
		array<int> itemParentIndices = {};
		array<ResourceName> itemPrefabs = {};
		array<int> itemAmmoCounts = {};
		array<float> itemHealths = {};

		if (!context.ReadValue("entryIds", entryIds) || !context.ReadValue("prefabs", prefabs))
			return true;

		context.ReadValue("healths", healths);
		context.ReadValue("fuelFractions", fuelFractions);
		context.ReadValue("fuelEntryIds", fuelEntryIds);
		context.ReadValue("fuelAmounts", fuelAmounts);
		context.ReadValue("weaponEntryIds", weaponEntryIds);
		context.ReadValue("weaponIndices", weaponIndices);
		context.ReadValue("weaponMagPrefabs", weaponMagPrefabs);
		context.ReadValue("weaponAmmoCounts", weaponAmmoCounts);
		context.ReadValue("itemEntryIds", itemEntryIds);
		context.ReadValue("itemNodeIndices", itemNodeIndices);
		context.ReadValue("itemParentIndices", itemParentIndices);
		context.ReadValue("itemPrefabs", itemPrefabs);
		context.ReadValue("itemAmmoCounts", itemAmmoCounts);
		context.ReadValue("itemHealths", itemHealths);

		map<int, ref AROR_GarageVehicleEntry> byId = new map<int, ref AROR_GarageVehicleEntry>();
		array<ref AROR_GarageVehicleEntry> entries = {};

		int count = Math.Min(entryIds.Count(), prefabs.Count());
		for (int i = 0; i < count; i++)
		{
			if (entryIds[i] <= 0 || prefabs[i].IsEmpty())
				continue;

			AROR_GarageVehicleEntry entry = new AROR_GarageVehicleEntry();
			entry.m_iId = entryIds[i];
			entry.m_sPrefab = prefabs[i];
			if (i < healths.Count())
				entry.m_fHealthScaled = healths[i];
			if (i < fuelFractions.Count())
				entry.m_fFuelNormalized = fuelFractions[i];

			byId.Set(entry.m_iId, entry);
			entries.Insert(entry);
		}

		int fuelCount = Math.Min(fuelEntryIds.Count(), fuelAmounts.Count());
		for (int i = 0; i < fuelCount; i++)
		{
			AROR_GarageVehicleEntry entry;
			if (!byId.Find(fuelEntryIds[i], entry) || !entry)
				continue;

			entry.m_aFuelAmounts.Insert(fuelAmounts[i]);
		}

		int weaponCount = Math.Min(Math.Min(weaponEntryIds.Count(), weaponIndices.Count()), Math.Min(weaponMagPrefabs.Count(), weaponAmmoCounts.Count()));
		for (int i = 0; i < weaponCount; i++)
		{
			AROR_GarageVehicleEntry entry;
			if (!byId.Find(weaponEntryIds[i], entry) || !entry)
				continue;

			AROR_GarageWeaponAmmoSnapshot ammo = new AROR_GarageWeaponAmmoSnapshot();
			ammo.m_iWeaponIndex = weaponIndices[i];
			ammo.m_sMagazinePrefab = weaponMagPrefabs[i];
			ammo.m_iAmmoCount = weaponAmmoCounts[i];
			entry.m_aWeaponAmmo.Insert(ammo);
		}

		RebuildCargo(byId, itemEntryIds, itemNodeIndices, itemParentIndices, itemPrefabs, itemAmmoCounts, itemHealths);

		garage.RestoreEntries_S(entries, nextId);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static void FlattenItem(int entryId, AROR_GarageItemSnapshot item, int parentIndex, notnull array<int> nextNode, notnull array<int> itemEntryIds, notnull array<int> itemNodeIndices, notnull array<int> itemParentIndices, notnull array<ResourceName> itemPrefabs, notnull array<int> itemAmmoCounts, notnull array<float> itemHealths)
	{
		if (!item || item.m_sPrefab.IsEmpty() || nextNode.IsEmpty())
			return;

		int nodeIndex = nextNode[0];
		nextNode[0] = nodeIndex + 1;

		itemEntryIds.Insert(entryId);
		itemNodeIndices.Insert(nodeIndex);
		itemParentIndices.Insert(parentIndex);
		itemPrefabs.Insert(item.m_sPrefab);
		itemAmmoCounts.Insert(item.m_iAmmoCount);
		itemHealths.Insert(item.m_fHealthScaled);

		foreach (AROR_GarageItemSnapshot child : item.m_aChildren)
		{
			FlattenItem(entryId, child, nodeIndex, nextNode, itemEntryIds, itemNodeIndices, itemParentIndices, itemPrefabs, itemAmmoCounts, itemHealths);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void RebuildCargo(notnull map<int, ref AROR_GarageVehicleEntry> byId, notnull array<int> itemEntryIds, notnull array<int> itemNodeIndices, notnull array<int> itemParentIndices, notnull array<ResourceName> itemPrefabs, notnull array<int> itemAmmoCounts, notnull array<float> itemHealths)
	{
		int count = Math.Min(Math.Min(itemEntryIds.Count(), itemNodeIndices.Count()), Math.Min(itemParentIndices.Count(), itemPrefabs.Count()));
		count = Math.Min(count, Math.Min(itemAmmoCounts.Count(), itemHealths.Count()));
		if (count <= 0)
			return;

		// Group row indices per entry
		map<int, ref array<int>> rowsByEntry = new map<int, ref array<int>>();
		for (int i = 0; i < count; i++)
		{
			int entryId = itemEntryIds[i];
			array<int> rows;
			if (!rowsByEntry.Find(entryId, rows) || !rows)
			{
				rows = {};
				rowsByEntry.Set(entryId, rows);
			}

			rows.Insert(i);
		}

		foreach (int entryId, array<int> rows : rowsByEntry)
		{
			AROR_GarageVehicleEntry entry;
			if (!byId.Find(entryId, entry) || !entry || !rows)
				continue;

			int maxNode = -1;
			foreach (int row : rows)
			{
				if (itemNodeIndices[row] > maxNode)
					maxNode = itemNodeIndices[row];
			}

			array<ref AROR_GarageItemSnapshot> nodes = {};
			for (int n = 0; n <= maxNode; n++)
				nodes.Insert(null);

			foreach (int row : rows)
			{
				int nodeIndex = itemNodeIndices[row];
				if (nodeIndex < 0 || nodeIndex > maxNode || itemPrefabs[row].IsEmpty())
					continue;

				AROR_GarageItemSnapshot snapshot = new AROR_GarageItemSnapshot();
				snapshot.m_sPrefab = itemPrefabs[row];
				snapshot.m_iAmmoCount = itemAmmoCounts[row];
				snapshot.m_fHealthScaled = itemHealths[row];
				nodes[nodeIndex] = snapshot;
			}

			foreach (int row : rows)
			{
				int nodeIndex = itemNodeIndices[row];
				int parentIndex = itemParentIndices[row];
				if (nodeIndex < 0 || nodeIndex > maxNode || !nodes[nodeIndex])
					continue;

				if (parentIndex < 0)
				{
					entry.m_aCargoRoots.Insert(nodes[nodeIndex]);
					continue;
				}

				if (parentIndex > maxNode || !nodes[parentIndex])
					continue;

				nodes[parentIndex].m_aChildren.Insert(nodes[nodeIndex]);
			}
		}
	}
}
