//! Server-side capture of a world vehicle into an AROR_GarageVehicleEntry.
//! Restore/spawn is intentionally not implemented here (retrieve is a later milestone).
class AROR_GarageSnapshotHelper
{
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
}
