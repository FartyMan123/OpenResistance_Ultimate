void AROR_OnLocationChanged(AROR_LocationComponent location);
typedef func AROR_OnLocationChanged;
typedef ScriptInvokerBase<AROR_OnLocationChanged> AROR_OnLocationChangedInvoker;

[ComponentEditorProps(category: "OpenResistance/Locations", description: "Registry of all map locations. Attach to the game mode entity.")]
class AROR_LocationManagerComponentClass : SCR_BaseGameModeComponentClass
{
}

//! Single source of truth for map locations. Other systems query locations through this.
//! Runs on server and clients: world-placed locations (towns) and replicated spawned ones (HQ) register on every machine.
class AROR_LocationManagerComponent : SCR_BaseGameModeComponent
{
	protected static AROR_LocationManagerComponent s_Instance;

	//! Locations that initialised before the manager existed
	protected static ref array<AROR_LocationComponent> s_aPendingLocations;

	protected ref array<AROR_LocationComponent> m_aLocations = {};

	protected ref AROR_OnLocationChangedInvoker m_OnLocationRegistered = new AROR_OnLocationChangedInvoker();
	protected ref AROR_OnLocationChangedInvoker m_OnLocationUnregistered = new AROR_OnLocationChangedInvoker();
	protected ref AROR_OnLocationChangedInvoker m_OnLocationControlChanged = new AROR_OnLocationChangedInvoker();

	//------------------------------------------------------------------------------------------------
	static AROR_LocationManagerComponent GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	//! Invoked on every machine when a location registers, including spawned ones (e.g. the HQ)
	AROR_OnLocationChangedInvoker GetOnLocationRegistered()
	{
		return m_OnLocationRegistered;
	}

	//------------------------------------------------------------------------------------------------
	//! Invoked on every machine when a location is deleted
	AROR_OnLocationChangedInvoker GetOnLocationUnregistered()
	{
		return m_OnLocationUnregistered;
	}

	//------------------------------------------------------------------------------------------------
	//! Invoked on every machine when a location's controlling role changes (read it with GetControllingRole)
	AROR_OnLocationChangedInvoker GetOnLocationControlChanged()
	{
		return m_OnLocationControlChanged;
	}

	//------------------------------------------------------------------------------------------------
	static void RegisterLocation(notnull AROR_LocationComponent location)
	{
		if (s_Instance)
		{
			s_Instance.AddLocation(location);
			return;
		}

		if (!s_aPendingLocations)
			s_aPendingLocations = {};

		if (!s_aPendingLocations.Contains(location))
			s_aPendingLocations.Insert(location);
	}

	//------------------------------------------------------------------------------------------------
	static void UnregisterLocation(AROR_LocationComponent location)
	{
		if (s_aPendingLocations)
			s_aPendingLocations.RemoveItem(location);

		if (s_Instance && s_Instance.m_aLocations.RemoveItem(location))
			s_Instance.m_OnLocationUnregistered.Invoke(location);
	}

	//------------------------------------------------------------------------------------------------
	//! Fills outLocations with every registered location. Returns the count.
	int GetLocations(notnull array<AROR_LocationComponent> outLocations)
	{
		outLocations.Copy(m_aLocations);
		return outLocations.Count();
	}

	//------------------------------------------------------------------------------------------------
	//! Fills outLocations with every registered location of the given type. Returns the count.
	int GetLocationsOfType(AROR_ELocationType type, notnull array<AROR_LocationComponent> outLocations)
	{
		outLocations.Clear();
		foreach (AROR_LocationComponent location : m_aLocations)
		{
			if (location && location.GetLocationType() == type)
				outLocations.Insert(location);
		}

		return outLocations.Count();
	}

	//------------------------------------------------------------------------------------------------
	//! Returns the location with the given stable ID, or null
	AROR_LocationComponent GetLocationById(string id)
	{
		if (id.IsEmpty())
			return null;

		foreach (AROR_LocationComponent location : m_aLocations)
		{
			if (location && location.GetId() == id)
				return location;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	AROR_LocationComponent GetNearestLocation(vector position)
	{
		return FindNearest(position, -1);
	}

	//------------------------------------------------------------------------------------------------
	AROR_LocationComponent GetNearestLocationOfType(vector position, AROR_ELocationType type)
	{
		return FindNearest(position, type);
	}

	//------------------------------------------------------------------------------------------------
	//! Returns the location whose area contains the position, or null. If areas overlap, the closest centre wins.
	AROR_LocationComponent GetLocationAt(vector position)
	{
		AROR_LocationComponent result;
		float bestDistance = float.MAX;

		foreach (AROR_LocationComponent location : m_aLocations)
		{
			if (!location || !location.IsPositionInside(position))
				continue;

			float distance = vector.DistanceXZ(location.GetCenter(), position);
			if (distance < bestDistance)
			{
				bestDistance = distance;
				result = location;
			}
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! typeFilter < 0 matches any type
	protected AROR_LocationComponent FindNearest(vector position, int typeFilter)
	{
		AROR_LocationComponent result;
		float bestDistance = float.MAX;

		foreach (AROR_LocationComponent location : m_aLocations)
		{
			if (!location)
				continue;

			if (typeFilter >= 0 && location.GetLocationType() != typeFilter)
				continue;

			float distance = vector.DistanceXZ(location.GetCenter(), position);
			if (distance < bestDistance)
			{
				bestDistance = distance;
				result = location;
			}
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	protected void AddLocation(notnull AROR_LocationComponent location)
	{
		if (m_aLocations.Contains(location))
			return;

		AROR_LocationComponent existing = GetLocationById(location.GetId());
		if (existing)
			Print(string.Format("[AROR] Duplicate location ID '%1' on '%2' and '%3'; saves will resolve to the first", location.GetId(), existing.GetName(), location.GetName()), LogLevel.ERROR);

		m_aLocations.Insert(location);
		Print(string.Format("[AROR] Location manager registered '%1' (%2 total)", location.GetName(), m_aLocations.Count()), LogLevel.NORMAL);
		m_OnLocationRegistered.Invoke(location);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode())
			return;

		s_Instance = this;

		if (s_aPendingLocations)
		{
			foreach (AROR_LocationComponent location : s_aPendingLocations)
			{
				if (location)
					AddLocation(location);
			}

			s_aPendingLocations.Clear();
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (s_Instance == this)
			s_Instance = null;

		super.OnDelete(owner);
	}
}
