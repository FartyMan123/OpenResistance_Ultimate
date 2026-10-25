[ComponentEditorProps(category: "OpenResistance/Settlements", description: "Server regional support from held military installations. Settlements are never flag objectives.")]
class AROR_RegionalSupportComponentClass : SCR_BaseGameModeComponentClass {}

class AROR_RegionalSupportComponent : SCR_BaseGameModeComponent
{
	[Attribute("0", desc: "Opt this scenario into gradual military-control influence on settlement support.")]
	protected bool m_bEnabled;
	[Attribute("60", desc: "Seconds per support cycle", params: "10 900 1")]
	protected int m_iInterval;
	[Attribute("2", desc: "Maximum support percentage points moved per cycle", params: "0.1 20 0.1")]
	protected float m_fStep;

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
		if (!Replication.IsServer() || !m_bEnabled)
			return;
		AROR_GameFlowComponent flow = AROR_GameFlowComponent.Cast(GetOwner().FindComponent(AROR_GameFlowComponent));
		AROR_LocationManagerComponent manager = AROR_LocationManagerComponent.GetInstance();
		if (!flow || !flow.IsPlaythroughReady() || !manager)
			return;
		array<AROR_LocationComponent> locations = {};
		manager.GetLocations(locations);
		foreach (AROR_LocationComponent town : locations)
		{
			if (!town || (town.GetLocationType() != AROR_ELocationType.TOWN && town.GetLocationType() != AROR_ELocationType.CITY))
				continue;
			AROR_SettlementComponent support = AROR_SettlementComponent.Cast(town.GetOwner().FindComponent(AROR_SettlementComponent));
			if (!support)
				continue;
			string region = RegionFor(town, locations);
			int resistance, occupiers, invaders;
			foreach (AROR_LocationComponent site : locations)
			{
				if (!site || !AROR_CaptureLocationAction.IsCapturableType(site.GetLocationType()) || RegionFor(site, locations) != region)
					continue;
				switch (site.GetControllingRole())
				{
					case AROR_EFactionRole.RESISTANCE: resistance++; break;
					case AROR_EFactionRole.OCCUPIERS: occupiers++; break;
					case AROR_EFactionRole.INVADERS: invaders++; break;
				}
			}
			// No installations means no military influence, rather than invented neutral support.
			support.ApplyRegionalSupport_S(resistance, occupiers, invaders, m_fStep);
		}
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (GetGame().InPlayMode() && Replication.IsServer() && m_bEnabled && m_iInterval >= 10)
			GetGame().GetCallqueue().CallLater(Tick, m_iInterval * 1000, true);
	}
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(Tick);
		super.OnDelete(owner);
	}
}
