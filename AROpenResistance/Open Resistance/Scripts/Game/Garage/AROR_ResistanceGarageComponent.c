[ComponentEditorProps(category: "OpenResistance/Garage", description: "Resistance shared vehicle garage (unique snapshots). Attach to the game mode entity.")]
class AROR_ResistanceGarageComponentClass : SCR_BaseGameModeComponentClass
{
}

void AROR_OnResistanceGarageChanged();
typedef func AROR_OnResistanceGarageChanged;

//! Resistance Garage (see ARCHITECTURE.md -> Resistance Garage): a campaign ledger of unique vehicle
//! snapshots for the Resistance. Capacity comes from the HQ plus Resistance-controlled military
//! objectives. Server owns full entry data; compact summaries replicate. Retrieve/spawn is deferred.
//! Saved by AROR_ResistanceGarageComponentSerializer.
class AROR_ResistanceGarageComponent : SCR_BaseGameModeComponent
{
	[Attribute("10", desc: "Garage slots granted by the Resistance HQ", params: "0 100", category: "Capacity")]
	protected int m_iHqSlots;

	[Attribute("2", desc: "Garage slots per Resistance-controlled military objective (excl. towns/cities)", params: "0 20", category: "Capacity")]
	protected int m_iObjectiveSlots;

	protected static AROR_ResistanceGarageComponent s_Instance;

	//! Server: full unique entries (cargo trees included)
	protected ref array<ref AROR_GarageVehicleEntry> m_aEntries = {};

	protected int m_iNextId = 1;

	//! Replicated summaries for clients / future UI
	[RplProp(onRplName: "OnGarageReplicated")]
	protected ref array<int> m_aEntryIds = {};

	[RplProp(onRplName: "OnGarageReplicated")]
	protected ref array<ResourceName> m_aPrefabs = {};

	[RplProp(onRplName: "OnGarageReplicated")]
	protected ref array<float> m_aHealths = {};

	[RplProp(onRplName: "OnGarageReplicated")]
	protected ref array<float> m_aFuelFractions = {};

	protected ref ScriptInvokerBase<AROR_OnResistanceGarageChanged> m_OnGarageChanged = new ScriptInvokerBase<AROR_OnResistanceGarageChanged>();

	protected bool m_bListeningToLocations;

	//------------------------------------------------------------------------------------------------
	static AROR_ResistanceGarageComponent GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	//! Every machine; fires after a mutate, capacity-affecting location change, or client replicate
	ScriptInvokerBase<AROR_OnResistanceGarageChanged> GetOnGarageChanged()
	{
		return m_OnGarageChanged;
	}

	//------------------------------------------------------------------------------------------------
	//! Every machine: HQ slots + objective slots for Resistance-held capturable locations
	int GetCapacity()
	{
		int capacity;
		AROR_LocationManagerComponent locations = AROR_LocationManagerComponent.GetInstance();
		if (!locations)
			return capacity;

		array<AROR_LocationComponent> all = {};
		locations.GetLocations(all);
		foreach (AROR_LocationComponent location : all)
		{
			if (!location || location.GetControllingRole() != AROR_EFactionRole.RESISTANCE)
				continue;

			AROR_ELocationType type = location.GetLocationType();
			if (type == AROR_ELocationType.HQ)
				capacity += m_iHqSlots;
			else if (AROR_CaptureLocationAction.IsCapturableType(type))
				capacity += m_iObjectiveSlots;
		}

		return capacity;
	}

	//------------------------------------------------------------------------------------------------
	//! Every machine
	int GetCount()
	{
		if (Replication.IsServer())
			return m_aEntries.Count();

		return m_aEntryIds.Count();
	}

	//------------------------------------------------------------------------------------------------
	//! Every machine
	bool IsFull()
	{
		return GetCount() >= GetCapacity();
	}

	//------------------------------------------------------------------------------------------------
	//! True while nothing is garaged (nothing to save)
	bool IsEmpty()
	{
		return GetCount() == 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Every machine: compact summaries (id, prefab, health, fuel fraction)
	void GetEntries(notnull array<int> ids, notnull array<ResourceName> prefabs, notnull array<float> healths, notnull array<float> fuelFractions)
	{
		ids.Clear();
		prefabs.Clear();
		healths.Clear();
		fuelFractions.Clear();

		if (Replication.IsServer())
		{
			foreach (AROR_GarageVehicleEntry entry : m_aEntries)
			{
				if (!entry)
					continue;

				ids.Insert(entry.m_iId);
				prefabs.Insert(entry.m_sPrefab);
				healths.Insert(entry.m_fHealthScaled);
				fuelFractions.Insert(entry.m_fFuelNormalized);
			}

			return;
		}

		int count = Math.Min(Math.Min(m_aEntryIds.Count(), m_aPrefabs.Count()), Math.Min(m_aHealths.Count(), m_aFuelFractions.Count()));
		for (int i = 0; i < count; i++)
		{
			ids.Insert(m_aEntryIds[i]);
			prefabs.Insert(m_aPrefabs[i]);
			healths.Insert(m_aHealths[i]);
			fuelFractions.Insert(m_aFuelFractions[i]);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Server only: full entry by id, or null
	AROR_GarageVehicleEntry GetEntry_S(int id)
	{
		if (!Replication.IsServer())
			return null;

		foreach (AROR_GarageVehicleEntry entry : m_aEntries)
		{
			if (entry && entry.m_iId == id)
				return entry;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Server only. Snapshots the vehicle, appends a unique entry, deletes the world entity.
	//! Rejects non-vehicles, destroyed hulls, occupied vehicles, and stores while at/over capacity.
	//! Any faction/prefab is allowed. Returns the new entry id, or -1 on failure.
	int StoreVehicle_S(IEntity vehicle)
	{
		if (!Replication.IsServer() || !vehicle)
			return -1;

		if (!AROR_GarageSnapshotHelper.IsVehicle(vehicle))
			return -1;

		if (AROR_GarageSnapshotHelper.HasLivingOccupant(vehicle))
			return -1;

		if (GetCount() >= GetCapacity())
			return -1;

		AROR_GarageVehicleEntry entry = AROR_GarageSnapshotHelper.Capture(vehicle);
		if (!entry)
			return -1;

		entry.m_iId = m_iNextId;
		m_iNextId++;
		m_aEntries.Insert(entry);

		SCR_EntityHelper.DeleteEntityAndChildren(vehicle);
		Publish_S();
		return entry.m_iId;
	}

	//------------------------------------------------------------------------------------------------
	//! Server only. Removes an entry without spawning. Returns false if the id is unknown.
	bool DiscardVehicle_S(int id)
	{
		if (!Replication.IsServer() || id <= 0)
			return false;

		for (int i = m_aEntries.Count() - 1; i >= 0; i--)
		{
			AROR_GarageVehicleEntry entry = m_aEntries[i];
			if (!entry || entry.m_iId != id)
				continue;

			m_aEntries.Remove(i);
			Publish_S();
			return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Server only: replace garage contents from a save (used by the serializer)
	void RestoreEntries_S(notnull array<ref AROR_GarageVehicleEntry> entries, int nextId)
	{
		if (!Replication.IsServer())
			return;

		m_aEntries.Clear();
		int maxId;
		foreach (AROR_GarageVehicleEntry entry : entries)
		{
			if (!entry || entry.m_sPrefab.IsEmpty() || entry.m_iId <= 0)
				continue;

			m_aEntries.Insert(entry);
			if (entry.m_iId > maxId)
				maxId = entry.m_iId;
		}

		if (nextId > maxId)
			m_iNextId = nextId;
		else
			m_iNextId = maxId + 1;

		Publish_S();
	}

	//------------------------------------------------------------------------------------------------
	//! Server only: copy of full entries for the serializer
	void GetFullEntries_S(notnull array<ref AROR_GarageVehicleEntry> outEntries)
	{
		outEntries.Clear();
		if (!Replication.IsServer())
			return;

		foreach (AROR_GarageVehicleEntry entry : m_aEntries)
		{
			if (entry)
				outEntries.Insert(entry);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Server only
	int GetNextId_S()
	{
		return m_iNextId;
	}

	//------------------------------------------------------------------------------------------------
	protected void Publish_S()
	{
		if (!Replication.IsServer())
			return;

		m_aEntryIds.Clear();
		m_aPrefabs.Clear();
		m_aHealths.Clear();
		m_aFuelFractions.Clear();

		foreach (AROR_GarageVehicleEntry entry : m_aEntries)
		{
			if (!entry)
				continue;

			m_aEntryIds.Insert(entry.m_iId);
			m_aPrefabs.Insert(entry.m_sPrefab);
			m_aHealths.Insert(entry.m_fHealthScaled);
			m_aFuelFractions.Insert(entry.m_fFuelNormalized);
		}

		Replication.BumpMe();
		m_OnGarageChanged.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnGarageReplicated()
	{
		m_OnGarageChanged.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnLocationChanged(AROR_LocationComponent location)
	{
		m_OnGarageChanged.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	protected void BindLocationListeners()
	{
		if (m_bListeningToLocations)
			return;

		AROR_LocationManagerComponent locations = AROR_LocationManagerComponent.GetInstance();
		if (!locations)
			return;

		locations.GetOnLocationRegistered().Insert(OnLocationChanged);
		locations.GetOnLocationUnregistered().Insert(OnLocationChanged);
		locations.GetOnLocationControlChanged().Insert(OnLocationChanged);
		m_bListeningToLocations = true;
	}

	//------------------------------------------------------------------------------------------------
	protected void UnbindLocationListeners()
	{
		if (!m_bListeningToLocations)
			return;

		AROR_LocationManagerComponent locations = AROR_LocationManagerComponent.GetInstance();
		if (locations)
		{
			locations.GetOnLocationRegistered().Remove(OnLocationChanged);
			locations.GetOnLocationUnregistered().Remove(OnLocationChanged);
			locations.GetOnLocationControlChanged().Remove(OnLocationChanged);
		}

		m_bListeningToLocations = false;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode())
			return;

		s_Instance = this;
		// Location manager may init in the same frame; defer the bind slightly
		GetGame().GetCallqueue().CallLater(BindLocationListeners, 0, false);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(BindLocationListeners);
		UnbindLocationListeners();

		if (s_Instance == this)
			s_Instance = null;

		super.OnDelete(owner);
	}
}
