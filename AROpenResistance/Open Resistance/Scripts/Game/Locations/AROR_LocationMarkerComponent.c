[ComponentEditorProps(category: "OpenResistance/Locations", description: "Places a local map marker for every location on each client. Attach to the game mode entity.")]
class AROR_LocationMarkerComponentClass : SCR_BaseGameModeComponentClass
{
}

//! Location markers (map marker layer 1): built from world data on each client, never replicated or saved.
//! Follows the location registry, so spawned locations (the HQ) get markers when they stream in.
class AROR_LocationMarkerComponent : SCR_BaseGameModeComponent
{
	//! Interval for waiting until the local player controller and marker manager exist
	protected static const int INIT_RETRY_MS = 1000;

	protected ref array<AROR_LocationComponent> m_aMarkedLocations = {};
	protected ref array<SCR_MapMarkerBase> m_aMarkers = {};

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode() || RplSession.Mode() == RplMode.Dedicated)
			return;

		GetGame().GetCallqueue().CallLater(TryInitialise, INIT_RETRY_MS, true);
	}

	//------------------------------------------------------------------------------------------------
	//! Local markers need the local player controller (marker owner) and the marker manager
	protected void TryInitialise()
	{
		AROR_LocationManagerComponent locationManager = AROR_LocationManagerComponent.GetInstance();
		if (!locationManager || !GetGame().GetPlayerController() || !SCR_MapMarkerManagerComponent.GetInstance())
			return;

		GetGame().GetCallqueue().Remove(TryInitialise);

		array<AROR_LocationComponent> locations = {};
		locationManager.GetLocations(locations);
		foreach (AROR_LocationComponent location : locations)
		{
			AddMarker(location);
		}

		locationManager.GetOnLocationRegistered().Insert(AddMarker);
		locationManager.GetOnLocationUnregistered().Insert(RemoveMarker);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddMarker(AROR_LocationComponent location)
	{
		if (!location || m_aMarkedLocations.Contains(location))
			return;

		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerManager)
			return;

		string text = location.GetName();
		if (text.IsEmpty())
			text = location.GetId();

		vector center = location.GetCenter();

		SCR_MapMarkerBase marker = new SCR_MapMarkerBase();
		marker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
		marker.SetWorldPos((int)center[0], (int)center[2]);
		marker.SetCustomText(text);
		marker.SetCanBeRemovedByOwner(false);
		markerManager.InsertStaticMarker(marker, true);

		m_aMarkedLocations.Insert(location);
		m_aMarkers.Insert(marker);
	}

	//------------------------------------------------------------------------------------------------
	protected void RemoveMarker(AROR_LocationComponent location)
	{
		int index = m_aMarkedLocations.Find(location);
		if (index < 0)
			return;

		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (markerManager && m_aMarkers[index])
			markerManager.RemoveStaticMarker(m_aMarkers[index]);

		m_aMarkedLocations.Remove(index);
		m_aMarkers.Remove(index);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(TryInitialise);

		AROR_LocationManagerComponent locationManager = AROR_LocationManagerComponent.GetInstance();
		if (locationManager)
		{
			locationManager.GetOnLocationRegistered().Remove(AddMarker);
			locationManager.GetOnLocationUnregistered().Remove(RemoveMarker);
		}

		super.OnDelete(owner);
	}
}
