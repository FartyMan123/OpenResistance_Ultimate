[ComponentEditorProps(category: "OpenResistance/Garrisons", description: "Server proximity activation and shared garrison budgets. Uses canonical ownership; no simulated territory capture.")]
class AROR_GarrisonManagerComponentClass : SCR_BaseGameModeComponentClass {}

class AROR_GarrisonManagerComponent : SCR_BaseGameModeComponent
{
	protected static AROR_GarrisonManagerComponent s_Instance;
	static AROR_GarrisonManagerComponent GetInstance() { return s_Instance; }
	bool CanDefendRole(AROR_EFactionRole role) { return m_bGarrisonsActive && FactionFor(role) != null; }
	[Attribute("1", desc: "Garrisons Active: allow proximity activation of hostile military-site defenders.")]
	protected bool m_bGarrisonsActive;
	[Attribute("1000", desc: "Activate when any living player is within this many metres of the location centre, including after teleporting", params: "250 5000 50")]
	protected float m_fActivationRadius;
	[Attribute("1", desc: "Seconds between proximity and retirement scans; physical spawn requests remain paced at 250 ms.", params: "1 2 0.25")]
	protected float m_fProximityInterval;
	[Attribute("6", desc: "Minimum player clearance at each defender pad.", params: "3 20 1")]
	protected float m_fPlayerSpawnClearance;
	[Attribute("2500", desc: "All players must be beyond this distance from the site AND its actors before retirement", params: "500 6000 50")]
	protected float m_fRetirementRadius;
	[Attribute("120", desc: "Seconds continuously outside the retirement radius before virtualisation", params: "30 900 5")]
	protected int m_iRetirementDelay;
	[Attribute("96", desc: "Maximum live/reserved garrison personnel, separate from Operations' QRF budget", params: "48 512 1")]
	protected int m_iMaxPersonnel;
	[Attribute("", desc: "Role/faction/native-asset bindings. Missing or mismatched bindings disable spawning for that role.")]
	protected ref array<ref AROR_GarrisonFaction> m_aFactions;
	protected AROR_GameFlowComponent m_Flow;
	protected AROR_LocationManagerComponent m_Locations;
	protected ref array<AROR_GarrisonComponent> m_aGarrisons = {};
	protected ref array<AROR_GarrisonComponent> m_aNearby = {};
	protected int m_iNextProximityAt;
	protected int m_iCursor;

	protected AROR_GarrisonFaction FactionFor(AROR_EFactionRole role)
	{
		AROR_FactionRolesComponent roles = AROR_FactionRolesComponent.GetInstance();
		if (!roles || !m_aFactions)
			return null;
		foreach (AROR_GarrisonFaction faction : m_aFactions)
		{
			if (faction && faction.m_eRole == role && faction.m_sFactionKey == roles.GetFactionKey(role))
				return faction;
		}
		return null;
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
		m_Locations.GetOnLocationRegistered().Insert(OnRegistered);
		m_Locations.GetOnLocationUnregistered().Insert(OnUnregistered);
		m_Locations.GetOnLocationControlChanged().Insert(OnControlChanged);
		array<AROR_LocationComponent> locations = {};
		m_Locations.GetLocations(locations);
		foreach (AROR_LocationComponent location : locations)
			OnRegistered(location);
		return m_Flow != null;
	}

	protected void OnRegistered(AROR_LocationComponent location)
	{
		if (!location)
			return;
		AROR_GarrisonComponent garrison = AROR_GarrisonComponent.Cast(location.GetOwner().FindComponent(AROR_GarrisonComponent));
		if (garrison && !m_aGarrisons.Contains(garrison))
			m_aGarrisons.Insert(garrison);
		m_iNextProximityAt = 0;
	}

	protected void OnUnregistered(AROR_LocationComponent location)
	{
		for (int index = m_aGarrisons.Count() - 1; index >= 0; index--)
		{
			AROR_GarrisonComponent garrison = m_aGarrisons[index];
			if (!garrison || garrison.GetLocation() == location)
			{
				m_aNearby.RemoveItem(garrison);
				m_aGarrisons.Remove(index);
			}
		}
		m_iNextProximityAt = 0;
	}

	protected void OnControlChanged(AROR_LocationComponent location)
	{
		foreach (AROR_GarrisonComponent garrison : m_aGarrisons)
		{
			if (garrison && garrison.GetLocation() == location && garrison.HasRoster() && garrison.GetRosterRole() != location.GetControllingRole())
				garrison.CloseDeployment_S("ownership changed");
		}
		m_iNextProximityAt = 0;
	}

	protected bool PlayerNear(AROR_LocationComponent location, array<vector> players)
	{
		foreach (vector position : players)
		{
			if (vector.DistanceXZ(position, location.GetCenter()) <= m_fActivationRadius)
				return true;
		}
		return false;
	}

	protected void ScanProximity_S(int now)
	{
		m_iNextProximityAt = now + Math.Max(1, m_fProximityInterval) * 1000;
		m_aNearby.Clear();
		array<vector> living = {};
		array<vector> allPlayers = {};
		array<int> ids = {};
		GetGame().GetPlayerManager().GetPlayers(ids);
		foreach (int id : ids)
		{
			IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(id);
			if (!entity)
				continue;
			allPlayers.Insert(entity.GetOrigin());
			ChimeraCharacter player = ChimeraCharacter.Cast(entity);
			if (player && player.GetCharacterController() && !player.GetCharacterController().IsDead())
				living.Insert(player.GetOrigin());
		}
		foreach (AROR_GarrisonComponent garrison : m_aGarrisons)
		{
			if (!garrison || !garrison.IsEligible())
				continue;
			AROR_LocationComponent location = garrison.GetLocation();
			if (garrison.IsGarrisonActive())
				garrison.Observe_S();
			if (garrison.IsGarrisonActive() || garrison.HasDeploymentStarted())
			{
				bool outside = garrison.CanRetire(Math.Max(m_fActivationRadius + 100, m_fRetirementRadius), allPlayers);
				if (garrison.OutsideLongEnough(outside, now, m_iRetirementDelay * 1000))
					garrison.Retire_S();
			}
			if (garrison.HasRoster() && garrison.GetRosterRole() != location.GetControllingRole())
			{
				garrison.ClearAssignments_S();
				if (!garrison.IsGarrisonActive())
					garrison.ResetRoster_S();
				garrison.RefreshPending_S();
				continue;
			}
			if (AROR_GarrisonPolicy.IsEnemyRole(location.GetControllingRole()) && PlayerNear(location, living) && CanDefendRole(location.GetControllingRole()))
			{
				garrison.BeginDeployment_S(now);
				if (!garrison.IsDeploymentClosed())
					m_aNearby.Insert(garrison);
			}
			garrison.RefreshPending_S();
		}
	}

	protected void Tick()
	{
		if (!Replication.IsServer() || !m_bGarrisonsActive || !EnsureRegistry_S() || !m_Flow.IsPlaythroughReady())
			return;
		int now = GetGame().GetWorld().GetWorldTime();
		if (now >= m_iNextProximityAt)
			ScanProximity_S(now);
		if (m_aNearby.IsEmpty())
			return;
		int reserved;
		foreach (AROR_GarrisonComponent garrison : m_aGarrisons)
		{
			if (garrison && garrison.IsGarrisonActive())
				reserved += garrison.ReservedPersonnel();
		}
		m_iCursor = Math.Mod(m_iCursor, m_aNearby.Count());
		AROR_GarrisonComponent next = m_aNearby[m_iCursor++];
		next.UpdateDeployment_S(now);
		AROR_GarrisonFaction binding = FactionFor(next.GetLocation().GetControllingRole());
		if (!binding || next.IsDeploymentClosed() || !AROR_GarrisonPolicy.IsEnemyRole(next.GetLocation().GetControllingRole()))
			return;
		if (next.HasRoster() && next.GetRosterRole() != next.GetLocation().GetControllingRole())
			return;
		if (!next.HasRoster() && !next.Generate_S(binding))
			return;
		if (!next.BindAssets_S(binding))
			return;
		if (!next.IsGarrisonActive() && (!next.HasSuitableDefenceTerrain_S() || reserved + next.ReservedPersonnel() > m_iMaxPersonnel))
			return;
		if (!next.SpawnNextAsset_S(binding, m_fPlayerSpawnClearance))
			next.SpawnNextGroup_S(now, m_fPlayerSpawnClearance);
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_Instance = this;
		if (GetGame().InPlayMode() && Replication.IsServer() && m_bGarrisonsActive)
			GetGame().GetCallqueue().CallLater(Tick, 250, true);
	}
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(Tick);
		if (m_Locations)
		{
			m_Locations.GetOnLocationRegistered().Remove(OnRegistered);
			m_Locations.GetOnLocationUnregistered().Remove(OnUnregistered);
			m_Locations.GetOnLocationControlChanged().Remove(OnControlChanged);
		}
		if (s_Instance == this)
			s_Instance = null;
		super.OnDelete(owner);
	}
}
