//! Server-side capture and restore of a world vehicle into / from an AROR_GarageVehicleEntry.
//! Belt-fed / muzzle-only guns without a magazine are skipped on both capture and restore.
class AROR_GarageSnapshotHelper
{
	//------------------------------------------------------------------------------------------------
	//! Short prefab leaf name for UI (strips GUID/path prefix when present)
	static string PrefabDisplayName(ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return "Unknown";

		string path = prefab;
		int slash = path.LastIndexOf("/");
		if (slash >= 0 && slash + 1 < path.Length())
			path = path.Substring(slash + 1, path.Length() - slash - 1);

		int dot = path.LastIndexOf(".");
		if (dot > 0)
			path = path.Substring(0, dot);

		return path;
	}

	//------------------------------------------------------------------------------------------------
	//! True when the entity is a Vehicle (any faction / prefab).
	static bool IsVehicle(IEntity entity)
	{
		return entity && Vehicle.Cast(entity);
	}

	//------------------------------------------------------------------------------------------------
	//! True when any compartment has a living occupant (player or AI).
	static bool HasLivingOccupant(IEntity vehicle)
	{
		if (!vehicle)
			return false;

		BaseCompartmentManagerComponent compartments = BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(BaseCompartmentManagerComponent));
		if (!compartments)
			return false;

		array<BaseCompartmentSlot> slots = {};
		compartments.GetCompartments(slots);
		foreach (BaseCompartmentSlot slot : slots)
		{
			if (!slot)
				continue;

			IEntity occupant = slot.GetOccupant();
			if (!occupant)
				continue;

			DamageManagerComponent damage = DamageManagerComponent.Cast(occupant.FindComponent(DamageManagerComponent));
			if (!damage || !damage.IsDestroyed())
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Builds a full entry snapshot. Caller assigns m_iId. Returns null on failure.
	static AROR_GarageVehicleEntry Capture(IEntity vehicle)
	{
		if (!IsVehicle(vehicle))
			return null;

		ResourceName prefab = SCR_ResourceNameUtils.GetPrefabName(vehicle);
		if (prefab.IsEmpty())
			return null;

		DamageManagerComponent damage = DamageManagerComponent.Cast(vehicle.FindComponent(DamageManagerComponent));
		if (damage && damage.IsDestroyed())
			return null;

		AROR_GarageVehicleEntry entry = new AROR_GarageVehicleEntry();
		entry.m_sPrefab = prefab;

		if (damage)
			entry.m_fHealthScaled = damage.GetHealthScaled();
		else
			entry.m_fHealthScaled = 1;

		CaptureFuel(vehicle, entry);
		CaptureCargo(vehicle, entry);
		CaptureWeaponAmmo(vehicle, entry);
		return entry;
	}

	//------------------------------------------------------------------------------------------------
	//! Applies a saved snapshot onto a freshly spawned vehicle (server). Clears default cargo first.
	static void Apply(IEntity vehicle, notnull AROR_GarageVehicleEntry entry)
	{
		if (!Replication.IsServer() || !vehicle)
			return;

		DamageManagerComponent damage = DamageManagerComponent.Cast(vehicle.FindComponent(DamageManagerComponent));
		if (damage)
			damage.SetHealthScaled(Math.Clamp(entry.m_fHealthScaled, 0, 1));

		ApplyFuel(vehicle, entry);
		ClearCargo(vehicle);
		ApplyCargo(vehicle, entry);
		ApplyWeaponAmmo(vehicle, entry);
	}

	//------------------------------------------------------------------------------------------------
	//! Flat cargo lines for the inventory dialog (depth-prefixed labels)
	static void FlattenCargoLabels(notnull AROR_GarageVehicleEntry entry, notnull array<string> outLabels)
	{
		outLabels.Clear();
		foreach (AROR_GarageItemSnapshot root : entry.m_aCargoRoots)
		{
			AppendCargoLabel(root, 0, outLabels);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void AppendCargoLabel(AROR_GarageItemSnapshot item, int depth, notnull array<string> outLabels)
	{
		if (!item || item.m_sPrefab.IsEmpty())
			return;

		string indent;
		for (int i = 0; i < depth; i++)
			indent += "  ";

		string name = PrefabDisplayName(item.m_sPrefab);
		string line = indent + name;
		if (item.m_iAmmoCount >= 0)
			line += string.Format(" (ammo %1)", item.m_iAmmoCount);
		if (item.m_fHealthScaled >= 0 && item.m_fHealthScaled < 0.999)
			line += string.Format(" (hp %1%%)", Math.Round(item.m_fHealthScaled * 100));

		outLabels.Insert(line);

		foreach (AROR_GarageItemSnapshot child : item.m_aChildren)
		{
			AppendCargoLabel(child, depth + 1, outLabels);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void CaptureFuel(IEntity vehicle, notnull AROR_GarageVehicleEntry entry)
	{
		FuelManagerComponent fuelManager = FuelManagerComponent.Cast(vehicle.FindComponent(FuelManagerComponent));
		if (!fuelManager)
		{
			entry.m_fFuelNormalized = 0;
			return;
		}

		array<BaseFuelNode> nodes = {};
		fuelManager.GetFuelNodesList(nodes);
		float total;
		float totalMax;
		foreach (BaseFuelNode node : nodes)
		{
			if (!node)
				continue;

			float amount = node.GetFuel();
			entry.m_aFuelAmounts.Insert(amount);
			total += amount;
			totalMax += node.GetMaxFuel();
		}

		if (totalMax > 0)
			entry.m_fFuelNormalized = Math.Clamp(total / totalMax, 0, 1);
		else
			entry.m_fFuelNormalized = 0;
	}

	//------------------------------------------------------------------------------------------------
	protected static void ApplyFuel(IEntity vehicle, notnull AROR_GarageVehicleEntry entry)
	{
		FuelManagerComponent fuelManager = FuelManagerComponent.Cast(vehicle.FindComponent(FuelManagerComponent));
		if (!fuelManager)
			return;

		array<BaseFuelNode> nodes = {};
		fuelManager.GetFuelNodesList(nodes);

		if (entry.m_aFuelAmounts.Count() == nodes.Count() && !entry.m_aFuelAmounts.IsEmpty())
		{
			int count = nodes.Count();
			for (int i = 0; i < count; i++)
			{
				BaseFuelNode node = nodes[i];
				if (node)
					node.SetFuel(entry.m_aFuelAmounts[i]);
			}

			return;
		}

		SCR_FuelManagerComponent scrFuel = SCR_FuelManagerComponent.Cast(fuelManager);
		if (scrFuel)
			scrFuel.SetTotalFuelPercentage(Math.Clamp(entry.m_fFuelNormalized, 0, 1));
	}

	//------------------------------------------------------------------------------------------------
	protected static void CaptureCargo(IEntity vehicle, notnull AROR_GarageVehicleEntry entry)
	{
		array<Managed> components = {};
		vehicle.FindComponents(BaseInventoryStorageComponent, components);
		foreach (Managed managed : components)
		{
			BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(managed);
			if (!storage || storage.GetOwner() != vehicle)
				continue;

			array<IEntity> nested = {};
			storage.GetAll(nested, false);
			foreach (IEntity child : nested)
			{
				AROR_GarageItemSnapshot snapshot = CaptureItemTree(child);
				if (snapshot)
					entry.m_aCargoRoots.Insert(snapshot);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void ClearCargo(IEntity vehicle)
	{
		InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(vehicle.FindComponent(InventoryStorageManagerComponent));
		if (!inventory)
			return;

		array<IEntity> items = {};
		inventory.GetItems(items);
		for (int i = items.Count() - 1; i >= 0; i--)
		{
			IEntity item = items[i];
			if (item)
				inventory.TryDeleteItem(item);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void ApplyCargo(IEntity vehicle, notnull AROR_GarageVehicleEntry entry)
	{
		InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(vehicle.FindComponent(InventoryStorageManagerComponent));
		if (!inventory)
			return;

		foreach (AROR_GarageItemSnapshot root : entry.m_aCargoRoots)
		{
			IEntity spawned = SpawnConfiguredItem(root);
			if (!spawned)
				continue;

			if (!inventory.TryInsertItem(spawned))
			{
				SCR_EntityHelper.DeleteEntityAndChildren(spawned);
				continue;
			}

			ApplyChildrenIntoItem(spawned, root);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void ApplyChildrenIntoItem(IEntity parentItem, notnull AROR_GarageItemSnapshot parentSnap)
	{
		if (parentSnap.m_aChildren.IsEmpty())
			return;

		InventoryStorageManagerComponent parentInventory = InventoryStorageManagerComponent.Cast(parentItem.FindComponent(InventoryStorageManagerComponent));
		array<Managed> storages = {};
		parentItem.FindComponents(BaseInventoryStorageComponent, storages);

		foreach (AROR_GarageItemSnapshot childSnap : parentSnap.m_aChildren)
		{
			IEntity child = SpawnConfiguredItem(childSnap);
			if (!child)
				continue;

			bool inserted;
			if (parentInventory)
				inserted = parentInventory.TryInsertItem(child);

			if (!inserted)
			{
				foreach (Managed managed : storages)
				{
					BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(managed);
					if (!storage)
						continue;

					// Prefer inserting via a manager on the vehicle if the item has none
					InventoryStorageManagerComponent vehicleInv = null;
					IEntity root = parentItem;
					while (root)
					{
						vehicleInv = InventoryStorageManagerComponent.Cast(root.FindComponent(InventoryStorageManagerComponent));
						if (vehicleInv)
							break;
						root = root.GetParent();
					}

					if (vehicleInv && vehicleInv.TryInsertItemInStorage(child, storage))
					{
						inserted = true;
						break;
					}
				}
			}

			if (!inserted)
			{
				SCR_EntityHelper.DeleteEntityAndChildren(child);
				continue;
			}

			ApplyChildrenIntoItem(child, childSnap);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static IEntity SpawnConfiguredItem(AROR_GarageItemSnapshot snapshot)
	{
		if (!snapshot || snapshot.m_sPrefab.IsEmpty())
			return null;

		Resource resource = Resource.Load(snapshot.m_sPrefab);
		if (!resource || !resource.IsValid())
			return null;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		IEntity item = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		if (!item)
			return null;

		if (snapshot.m_iAmmoCount >= 0)
		{
			BaseMagazineComponent magazine = BaseMagazineComponent.Cast(item.FindComponent(BaseMagazineComponent));
			if (magazine)
				magazine.SetAmmoCount(snapshot.m_iAmmoCount);
		}

		if (snapshot.m_fHealthScaled >= 0)
		{
			DamageManagerComponent damage = DamageManagerComponent.Cast(item.FindComponent(DamageManagerComponent));
			if (damage)
				damage.SetHealthScaled(Math.Clamp(snapshot.m_fHealthScaled, 0, 1));
		}

		return item;
	}

	//------------------------------------------------------------------------------------------------
	protected static AROR_GarageItemSnapshot CaptureItemTree(IEntity item)
	{
		if (!item)
			return null;

		InventoryItemComponent invItem = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		if (!invItem)
			return null;

		ResourceName prefab = SCR_ResourceNameUtils.GetPrefabName(item);
		if (prefab.IsEmpty())
			return null;

		AROR_GarageItemSnapshot snapshot = new AROR_GarageItemSnapshot();
		snapshot.m_sPrefab = prefab;

		BaseMagazineComponent magazine = BaseMagazineComponent.Cast(item.FindComponent(BaseMagazineComponent));
		if (magazine)
			snapshot.m_iAmmoCount = magazine.GetAmmoCount();

		DamageManagerComponent damage = DamageManagerComponent.Cast(item.FindComponent(DamageManagerComponent));
		if (damage)
			snapshot.m_fHealthScaled = damage.GetHealthScaled();

		array<Managed> components = {};
		item.FindComponents(BaseInventoryStorageComponent, components);
		foreach (Managed managed : components)
		{
			BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(managed);
			if (!storage)
				continue;

			array<IEntity> nested = {};
			storage.GetAll(nested, false);
			foreach (IEntity child : nested)
			{
				AROR_GarageItemSnapshot childSnapshot = CaptureItemTree(child);
				if (childSnapshot)
					snapshot.m_aChildren.Insert(childSnapshot);
			}
		}

		return snapshot;
	}

	//------------------------------------------------------------------------------------------------
	//! Captures current magazine fill on each managed weapon. Belt/muzzle-only guns without a magazine are skipped.
	protected static void CaptureWeaponAmmo(IEntity vehicle, notnull AROR_GarageVehicleEntry entry)
	{
		array<Managed> managers = {};
		vehicle.FindComponents(BaseWeaponManagerComponent, managers);

		int weaponIndex;
		foreach (Managed managed : managers)
		{
			BaseWeaponManagerComponent manager = BaseWeaponManagerComponent.Cast(managed);
			if (!manager)
				continue;

			array<BaseWeaponComponent> weapons = {};
			manager.GetWeapons(weapons);
			foreach (BaseWeaponComponent weapon : weapons)
			{
				if (!weapon)
				{
					weaponIndex++;
					continue;
				}

				BaseMagazineComponent magazine = weapon.GetCurrentMagazine();
				if (magazine)
				{
					AROR_GarageWeaponAmmoSnapshot ammo = new AROR_GarageWeaponAmmoSnapshot();
					ammo.m_iWeaponIndex = weaponIndex;
					ammo.m_iAmmoCount = magazine.GetAmmoCount();

					IEntity magOwner = magazine.GetOwner();
					if (magOwner)
						ammo.m_sMagazinePrefab = SCR_ResourceNameUtils.GetPrefabName(magOwner);

					entry.m_aWeaponAmmo.Insert(ammo);
				}

				weaponIndex++;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void ApplyWeaponAmmo(IEntity vehicle, notnull AROR_GarageVehicleEntry entry)
	{
		if (entry.m_aWeaponAmmo.IsEmpty())
			return;

		map<int, ref AROR_GarageWeaponAmmoSnapshot> byIndex = new map<int, ref AROR_GarageWeaponAmmoSnapshot>();
		foreach (AROR_GarageWeaponAmmoSnapshot ammo : entry.m_aWeaponAmmo)
		{
			if (ammo)
				byIndex.Set(ammo.m_iWeaponIndex, ammo);
		}

		array<Managed> managers = {};
		vehicle.FindComponents(BaseWeaponManagerComponent, managers);

		int weaponIndex;
		foreach (Managed managed : managers)
		{
			BaseWeaponManagerComponent manager = BaseWeaponManagerComponent.Cast(managed);
			if (!manager)
				continue;

			array<BaseWeaponComponent> weapons = {};
			manager.GetWeapons(weapons);
			foreach (BaseWeaponComponent weapon : weapons)
			{
				AROR_GarageWeaponAmmoSnapshot ammo;
				if (weapon && byIndex.Find(weaponIndex, ammo) && ammo)
				{
					BaseMagazineComponent magazine = weapon.GetCurrentMagazine();
					if (magazine)
						magazine.SetAmmoCount(ammo.m_iAmmoCount);
				}

				weaponIndex++;
			}
		}
	}
}
