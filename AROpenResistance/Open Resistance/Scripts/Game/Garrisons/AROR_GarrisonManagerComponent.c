[ComponentEditorProps(category: "OpenResistance/Garrisons", description: "Server proximity activation and shared garrison budgets. Uses canonical ownership; no simulated territory capture.")]
class AROR_GarrisonManagerComponentClass : SCR_BaseGameModeComponentClass {}

class AROR_GarrisonManagerComponent : SCR_BaseGameModeComponent
{
	protected static AROR_GarrisonManagerComponent s_Instance;
	static AROR_GarrisonManagerComponent GetInstance() { return s_Instance; }
	bool CanDefendRole(AROR_EFactionRole role) { return m_bEnabled && FactionFor(role) != null; }
	[Attribute("1")]
	protected bool m_bEnabled;
	[Attribute("1000", desc: "Activate when any living player is within this many metres of the location centre, including after teleporting", params: "250 5000 50")]
	protected float m_fActivationRadius;
	[Attribute("6", desc: "Minimum player clearance at each defender pad. Keep small so arriving inside a site does not suppress its garrison.", params: "3 20 1")]
	protected float m_fPlayerSpawnClearance;
	[Attribute("2500", desc: "All players must be beyond this distance from the site AND its actors before retirement", params: "500 6000 50")]
	protected float m_fRetirementRadius;
	[Attribute("120", desc: "Seconds continuously outside the retirement radius before virtualisation", params: "30 900 5")]
	protected int m_iRetirementDelay;
	[Attribute("96", desc: "Maximum live/reserved garrison personnel, separate from Operations' existing QRF budget", params: "48 512 1")]
	protected int m_iMaxPersonnel;
	[Attribute("", desc: "Role/faction/native-asset bindings. Missing or mismatched bindings disable spawning for that role.")]
	protected ref array<ref AROR_GarrisonFaction> m_aFactions;
	protected int m_iElapsedMilliseconds;
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
	protected bool PlayerNear(AROR_LocationComponent location)
	{
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int playerId : players)
		{
			ChimeraCharacter player = ChimeraCharacter.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId));
			if (player && player.GetCharacterController() && !player.GetCharacterController().IsDead() && vector.DistanceXZ(player.GetOrigin(), location.GetCenter()) <= m_fActivationRadius)
				return true;
		}
		return false;
	}
	protected void Tick()
	{
		if (!Replication.IsServer() || !m_bEnabled)
			return;
		m_iElapsedMilliseconds += 250;
		int now = m_iElapsedMilliseconds / 1000;
		AROR_GameFlowComponent flow = AROR_GameFlowComponent.Cast(GetOwner().FindComponent(AROR_GameFlowComponent));
		AROR_LocationManagerComponent manager = AROR_LocationManagerComponent.GetInstance();
		if (!flow || !flow.IsPlaythroughReady() || !manager)
			return;
		array<AROR_LocationComponent> locations = {};
		manager.GetLocations(locations);
		array<AROR_GarrisonComponent> candidates = {};
		int reserved;
		foreach (AROR_LocationComponent location : locations)
		{
			if (!location)
				continue;
			AROR_GarrisonComponent garrison = AROR_GarrisonComponent.Cast(location.GetOwner().FindComponent(AROR_GarrisonComponent));
			if (!garrison || !garrison.IsEligible())
				continue;
			if (garrison.IsGarrisonActive())
			{
				garrison.Observe_S();
				bool outside = garrison.CanRetire(Math.Max(m_fActivationRadius + 100, m_fRetirementRadius));
				if (garrison.OutsideLongEnough(outside, now, m_iRetirementDelay))
				{
					garrison.Retire_S();
					Print(string.Format("[AROR GARRISON] %1 virtualised; casualties retained", location.GetId()), LogLevel.NORMAL);
				}
			}
			garrison.RefreshPending_S();
			if (garrison.HasRoster() && garrison.GetRosterRole() != location.GetControllingRole())
			{
				garrison.ClearAssignments_S();
				if (!garrison.IsGarrisonActive())
					garrison.ResetRoster_S();
				continue;
			}
			if (garrison.IsGarrisonActive())
				reserved += garrison.ReservedPersonnel();
			if (AROR_GarrisonPolicy.IsEnemyRole(location.GetControllingRole()) && PlayerNear(location))
				candidates.Insert(garrison);
		}
		if (candidates.IsEmpty())
			return;
		// Poll current positions, not boundary crossings: teleport arrivals work too.
		// One physical hull or one queued team per quarter-second across the island.
		// Rotate fairly so a blocked pad cannot starve another site.
		m_iCursor = Math.Mod(m_iCursor, candidates.Count());
		AROR_GarrisonComponent next = candidates[m_iCursor++];
		AROR_GarrisonFaction binding = FactionFor(next.GetLocation().GetControllingRole());
		if (!binding)
			return;
		if (!next.HasRoster() && !next.Generate_S(binding))
			return;
		if (!next.IsGarrisonActive() && !next.HasSuitableDefenceTerrain_S())
			return; // A rejected hillside/shoreline site must not reserve capacity away from usable sites.
		if (!next.IsGarrisonActive() && reserved + next.ReservedPersonnel() > m_iMaxPersonnel)
			return;
		if (!next.SpawnNextAsset_S(binding, m_fPlayerSpawnClearance))
			next.SpawnNextGroup_S(now, m_fPlayerSpawnClearance);
	}
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_Instance = this;
		if (GetGame().InPlayMode() && Replication.IsServer() && m_bEnabled)
			GetGame().GetCallqueue().CallLater(Tick, 250, true);
	}
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(Tick);
		if (s_Instance == this)
			s_Instance = null;
		super.OnDelete(owner);
	}
}
