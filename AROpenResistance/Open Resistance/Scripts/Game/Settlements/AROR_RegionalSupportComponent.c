[ComponentEditorProps(category: "OpenResistance/Settlements", description: "Server regional support from held military installations. Settlements are never flag objectives.")]
class AROR_RegionalSupportComponentClass : SCR_BaseGameModeComponentClass {}

class AROR_RegionalSupportComponent : SCR_BaseGameModeComponent
{
	[Attribute("0", desc: "Opt this scenario into gradual military-control influence on settlement support.")]
	protected bool m_bRegionalSupport;
	[Attribute("60", desc: "Seconds per support cycle", params: "10 900 1")]
	protected int m_iInterval;
	[Attribute("2", desc: "Maximum support percentage points moved per cycle", params: "0.1 20 0.1")]
	protected float m_fStep;
	protected AROR_GameFlowComponent m_Flow;
	protected AROR_LocationManagerComponent m_Locations;
	protected ref array<ref AROR_RegionalSupportRegion> m_aRegions = {};
	protected bool m_bRegionsDirty = true;

	protected void OnRegistryChanged(AROR_LocationComponent location)
	{
		m_bRegionsDirty = true;
	}

	protected bool EnsureRegistry_S()
	{
		if (!m_Flow)
			m_Flow = AROR_GameFlowComponent.Cast(GetOwner().FindComponent(AROR_GameFlowComponent));
		if (m_Locations)
			return m_Flow != null;
		m_Locations = AROR_LocationManagerComponent.GetInstance();
		if (!m_Locations)
			return false;
		m_Locations.GetOnLocationRegistered().Insert(OnRegistryChanged);
		m_Locations.GetOnLocationUnregistered().Insert(OnRegistryChanged);
		m_bRegionsDirty = true;
		return m_Flow != null;
	}

	protected void RebuildRegions_S()
	{
		m_aRegions.Clear();
		array<AROR_LocationComponent> locations = {};
		m_Locations.GetLocations(locations);
		foreach (AROR_LocationComponent location : locations)
		{
			if (!location)
				continue;
			bool settlement = location.GetLocationType() == AROR_ELocationType.TOWN || location.GetLocationType() == AROR_ELocationType.CITY;
			if (!settlement && !AROR_CaptureLocationAction.IsCapturableType(location.GetLocationType()))
				continue;
			string id = RegionFor(location, locations);
			if (id.IsEmpty())
				continue;
			AROR_RegionalSupportRegion region;
			foreach (AROR_RegionalSupportRegion existing : m_aRegions)
			{
				if (existing.m_sId == id)
					region = existing;
			}
			if (!region)
			{
				region = new AROR_RegionalSupportRegion();
				region.m_sId = id;
				m_aRegions.Insert(region);
			}
			if (settlement)
			{
				AROR_SettlementComponent support = AROR_SettlementComponent.Cast(location.GetOwner().FindComponent(AROR_SettlementComponent));
				if (support)
					region.m_aSettlements.Insert(support);
			}
			else
				region.m_aSites.Insert(location);
		}
		m_bRegionsDirty = false;
	}

	//! Equal-distance ties use the stable ID, never registration order.
	string RegionFor(AROR_LocationComponent site, array<AROR_LocationComponent> locations)
	{
		if (!site.GetSupportRegionId().IsEmpty())
			return site.GetSupportRegionId();
		if (site.GetLocationType() == AROR_ELocationType.TOWN || site.GetLocationType() == AROR_ELocationType.CITY)
			return site.GetId();
		float best = float.MAX;
		string result;
		string bestId;
		foreach (AROR_LocationComponent town : locations)
		{
			if (!town || (town.GetLocationType() != AROR_ELocationType.TOWN && town.GetLocationType() != AROR_ELocationType.CITY))
				continue;
			float distance = vector.DistanceXZ(site.GetCenter(), town.GetCenter());
			if (distance < best || (distance == best && town.GetId().Compare(bestId) < 0))
			{
				best = distance;
				bestId = town.GetId();
				result = town.GetSupportRegionId();
				if (result.IsEmpty())
					result = town.GetId();
			}
		}
		return result;
	}

	protected void Tick()
	{
		if (!Replication.IsServer() || !m_bRegionalSupport || !EnsureRegistry_S() || !m_Flow.IsPlaythroughReady())
			return;
		if (m_bRegionsDirty)
			RebuildRegions_S();
		foreach (AROR_RegionalSupportRegion region : m_aRegions)
		{
			int resistance, occupiers, invaders;
			foreach (AROR_LocationComponent site : region.m_aSites)
			{
				if (!site)
					continue;
				switch (site.GetControllingRole())
				{
					case AROR_EFactionRole.RESISTANCE: resistance++; break;
					case AROR_EFactionRole.OCCUPIERS: occupiers++; break;
					case AROR_EFactionRole.INVADERS: invaders++; break;
				}
			}
			foreach (AROR_SettlementComponent support : region.m_aSettlements)
			{
				if (support)
					support.ApplyRegionalSupport_S(resistance, occupiers, invaders, m_fStep);
			}
		}
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (GetGame().InPlayMode() && Replication.IsServer() && m_bRegionalSupport && m_iInterval >= 10)
			GetGame().GetCallqueue().CallLater(Tick, m_iInterval * 1000, true);
	}
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(Tick);
		if (m_Locations)
		{
			m_Locations.GetOnLocationRegistered().Remove(OnRegistryChanged);
			m_Locations.GetOnLocationUnregistered().Remove(OnRegistryChanged);
		}
		super.OnDelete(owner);
	}
}

//! Scenario membership cache; ownership is read fresh each cycle.
class AROR_RegionalSupportRegion
{
	string m_sId;
	ref array<AROR_LocationComponent> m_aSites = {};
	ref array<AROR_SettlementComponent> m_aSettlements = {};
}
