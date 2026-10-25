[ComponentEditorProps(category: "OpenResistance/Garrisons", description: "Saved military defence roster; proximity manager owns its physical activation. Never put on settlements.")]
class AROR_GarrisonComponentClass : ScriptComponentClass {}

class AROR_GarrisonComponent : ScriptComponent
{
	[Attribute("1", desc: "Allow a defence roster for this installation")]
	protected bool m_bEnabled;
	[Attribute("0", desc: "Defence level override, 0 = type/random recipe", params: "0 4 1")]
	protected int m_iDefenceLevel;
	[Attribute("", desc: "Optional authored world-space positions for statics/vehicles. Otherwise use bounded clear terrain search.")]
	protected ref array<vector> m_aDefencePositions;
	[Attribute("0", desc: "Perimeter half-width in metres; zero uses the standard location-type preset. Independent of capture radius.", params: "0 64 1")]
	protected float m_fDefenceHalfExtent;
	[Attribute("2", desc: "Maximum terrain height range across the full perimeter footprint", params: "0.25 4 0.25")]
	protected float m_fMaxDefenceRelief;
	[Attribute("800", desc: "Editor-only clearing search radius; does not move runtime defenders or alter activation distance.", params: "25 1600 25")]
	protected float m_fEditorSearchRadius;
	[Attribute("6", desc: "Infantry defend-order radius around its authored perimeter position", params: "2 12 1")]
	protected float m_fDefenceHoldRadius;
	[Attribute("30", desc: "Seconds of failed pad placement before omitting that unqueued asset/team for this activation. Living and queued defenders still block capture.", params: "10 120 5")]
	protected int m_iPlacementRetrySeconds;
	[Attribute("0", desc: "Keep the complete defence footprint within this location's canonical capture radius. Used for fixed strategic anchors.")]
	protected bool m_bFixedDefenceAnchor;
	[Attribute("3", desc: "Player clearance for crew staging near the flag. Collision/navigation checks still apply.", params: "2 6 0.5")]
	protected float m_fCrewPlayerClearance;
	protected bool m_bDefenceTerrainChecked;
	protected bool m_bDefenceTerrainFits;

	protected AROR_LocationComponent m_Location;
	protected AROR_EFactionRole m_eRosterRole;
	protected string m_sFactionKey;
	protected string m_sAssetFactionKey;
	protected int m_iLevel;
	protected bool m_bGenerated;
	protected bool m_bActive;
	[RplProp()]
	protected bool m_bPendingDefense;
	[RplProp()]
	protected string m_sPendingDefenseReason;
	protected bool m_bDeploymentDeferred;
	protected int m_iNextGroup;
	protected int m_iNextAsset;
	protected int m_iOutsideSince = -1;
	protected int m_iFailureCount;
	protected void ReportFailure(string reason)
	{
		m_bDeploymentDeferred = true;
		m_iFailureCount++;
		if (m_iFailureCount == 1 || Math.Mod(m_iFailureCount, 30) == 0)
			Print(string.Format("[AROR GARRISON] %1 deferred: %2 (attempt %3)", m_Location.GetId(), reason, m_iFailureCount), LogLevel.WARNING);
	}
	protected ref array<ref AROR_GarrisonSlot> m_aSlots = {};
	protected ref array<ref AROR_GarrisonGroup> m_aGroups = {};

	AROR_LocationComponent GetLocation() { return m_Location; }
	bool IsGarrisonActive() { return m_bActive; }
	bool HasRoster() { return m_bGenerated; }
	int GetLevel() { return m_iLevel; }
	AROR_EFactionRole GetRosterRole() { return m_eRosterRole; }
	string GetRosterFaction() { return m_sFactionKey; }
	string GetAssetFaction() { return m_sAssetFactionKey; }
	void GetSlots(array<ref AROR_GarrisonSlot> slots)
	{
		// Copy() cannot accept this owned-reference element type in Enforce.
		slots.Clear();
		foreach (AROR_GarrisonSlot slot : m_aSlots)
			slots.Insert(slot);
	}
	int ReservedPersonnel()
	{
		int count;
		foreach (AROR_GarrisonSlot slot : m_aSlots)
		{
			if (!slot.m_bLost && slot.m_iKind == 0)
				count++;
			// Reserve conservatively before crew seats have been discovered.
			if (!slot.m_bLost && slot.m_iKind == 1)
				count++;
			if (!slot.m_bLost && slot.m_iKind == 3)
				count += 4;
		}
		return count;
	}
	void ClearAssignments_S()
	{
		if (!Replication.IsServer())
			return;
		foreach (AROR_GarrisonGroup group : m_aGroups)
		{
			if (!group.m_Group)
				continue;
			AROR_QRFDefenseComponent defense = AROR_QRFDefenseComponent.Cast(group.m_Group.FindComponent(AROR_QRFDefenseComponent));
			if (defense)
				defense.ClearDefense_S();
		}
	}
	bool IsEligible()
	{
		return m_bEnabled && m_Location && AROR_CaptureLocationAction.IsCapturableLocation(m_Location);
	}
	//! Once real personnel have deployed and all submitted native queues have
	//! resolved, capture depends on actual hostile presence, not unused recipe slots.
	protected bool HasEstablishedDefence_S()
	{
		bool deployed;
		foreach (AROR_GarrisonSlot slot : m_aSlots)
		{
			if ((slot.m_iKind == 0 || slot.m_iKind == 4) && slot.m_bSpawned)
				deployed = true;
		}
		if (!deployed)
			return false;
		foreach (AROR_GarrisonGroup queued : m_aGroups)
		{
			foreach (int index : queued.m_aSlots)
			{
				if (!m_aSlots[index].m_bLost && !m_aSlots[index].m_bSpawned)
					return false;
			}
		}
		return true;
	}
	//! Pending native spawns cannot make an uncleared garrison look empty to a flag interaction.
	bool IsDefencePending()
	{
		if (!Replication.IsServer())
			return m_bPendingDefense;
		if (!IsEligible() || !AROR_GarrisonPolicy.IsEnemyRole(m_Location.GetControllingRole()))
			return false;
		AROR_GarrisonManagerComponent manager = AROR_GarrisonManagerComponent.GetInstance();
		if (!manager || !manager.CanDefendRole(m_Location.GetControllingRole()))
			return false;
		if (!m_bGenerated)
			return true;
		if (m_eRosterRole != m_Location.GetControllingRole())
			return false;
		if (HasEstablishedDefence_S())
			return false;
		foreach (AROR_GarrisonSlot slot : m_aSlots)
		{
			if (!slot.m_bLost && !slot.m_bPlacementUnavailable && slot.m_iKind != 2 && !slot.m_bSpawned)
				return true;
		}
		return false;
	}
	void RefreshPending_S()
	{
		if (!Replication.IsServer())
			return;
		bool pending = IsDefencePending();
		string reason = GetDefenceCaptureBlockReason();
		if (pending == m_bPendingDefense && reason == m_sPendingDefenseReason)
			return;
		m_bPendingDefense = pending;
		m_sPendingDefenseReason = reason;
		Replication.BumpMe();
	}
	//! Pending virtual defenders are a deployment gate, never proof of living hostiles.
	string GetDefenceCaptureBlockReason()
	{
		if (!Replication.IsServer())
			return m_sPendingDefenseReason;
		if (!IsDefencePending())
			return string.Empty;
		if (m_bDefenceTerrainChecked && !m_bDefenceTerrainFits)
			return "Defence site unsuitable - relocation required";
		if (m_bDeploymentDeferred)
			return "Defence deployment delayed";
		return "Defenders awaiting deployment";
	}

	//! Bound failed pad placement without inventing deaths, deleting live actors,
	//! or releasing personnel already submitted to the native spawn queue.
	protected void ReportPlacementFailure(int index, string reason)
	{
		if (!Replication.IsServer())
			return;
		AROR_GarrisonSlot slot = m_aSlots[index];
		if (slot.m_bSpawned || slot.m_Entity || slot.m_bPlacementUnavailable)
			return;
		foreach (AROR_GarrisonGroup queued : m_aGroups)
		{
			if (slot.m_iGroup >= 0 && queued.m_iId == slot.m_iGroup)
				return;
		}
		int now = GetGame().GetWorld().GetWorldTime();
		if (slot.m_iPlacementFailureSince < 0)
			slot.m_iPlacementFailureSince = now;
		ReportFailure(string.Format("slot=%1 %2", index, reason));
		if (now - slot.m_iPlacementFailureSince < m_iPlacementRetrySeconds * 1000)
			return;
		foreach (AROR_GarrisonSlot candidate : m_aSlots)
		{
			bool sameTeam = (slot.m_iKind == 0 || slot.m_iKind == 4) && candidate.m_iGroup == slot.m_iGroup;
			if ((candidate == slot || sameTeam || candidate.m_iVehicle == index) && !candidate.m_bSpawned && !candidate.m_Entity)
				candidate.m_bPlacementUnavailable = true;
		}
		Print(string.Format("[AROR GARRISON] %1 slot=%2 omitted for this activation after failed pad placement; no casualty recorded. Live/queued defenders still block capture.", m_Location.GetId(), index), LogLevel.WARNING);
	}

	protected int AddSlot(ResourceName prefab, int kind, int group = -1, int vehicle = -1, int seat = -1)
	{
		AROR_GarrisonSlot slot = new AROR_GarrisonSlot();
		slot.m_sPrefab = prefab;
		slot.m_iKind = kind;
		slot.m_iGroup = group;
		slot.m_iVehicle = vehicle;
		slot.m_iSeat = seat;
		m_aSlots.Insert(slot);
		return m_aSlots.Count() - 1;
	}

	bool Generate_S(AROR_GarrisonFaction faction)
	{
		if (!Replication.IsServer() || !IsEligible() || m_bGenerated || !faction || faction.m_eRole != m_Location.GetControllingRole())
			return false;
		m_eRosterRole = faction.m_eRole;
		m_sFactionKey = faction.m_sFactionKey;
		m_sAssetFactionKey = faction.m_sAssetFactionKey;
		m_iLevel = AROR_GarrisonPolicy.DefenceLevel(m_Location.GetLocationType(), Math.RandomInt(0, 3));
		if (m_iDefenceLevel > 0)
			m_iLevel = Math.Clamp(m_iDefenceLevel, 1, 4);
		if (m_Location.GetLocationType() == AROR_ELocationType.AIRPORT)
			m_iLevel = 4;
		// An outpost override may strengthen infantry/statics, never add armour.
		int group;
		for (int team = 0; team < AROR_GarrisonPolicy.InfantryTeams(m_iLevel); team++)
		{
			for (int member = 0; member < 4; member++)
				AddSlot(faction.m_sInfantry, 0, group);
			group++;
		}
		for (int mg = 0; mg < AROR_GarrisonPolicy.StaticCount(m_iLevel); mg++)
			AddSlot(faction.m_sStaticMG, 1, group++);
		if (m_Location.GetLocationType() != AROR_ELocationType.OUTPOST)
		{
			if (m_iLevel >= 3)
				AddSlot(faction.m_sParkedArmour, 2);
			for (int hull = 0; hull < AROR_GarrisonPolicy.MannedCount(m_iLevel); hull++)
			{
				if (!faction.m_aMannedVehicles || faction.m_aMannedVehicles.IsEmpty())
					break;
				AddSlot(faction.m_aMannedVehicles[Math.RandomInt(0, faction.m_aMannedVehicles.Count())], 3, group++);
			}
		}
		// Crew slots are added from each actual hull's seat list on first activation.
		m_bGenerated = true;
		Print(string.Format("[AROR GARRISON] %1 roster role=%2 level=%3 personnel reservation=%4", m_Location.GetId(), m_eRosterRole, m_iLevel, ReservedPersonnel()), LogLevel.NORMAL);
		return true;
	}

	protected bool InitializeFaction(IEntity entity)
	{
		FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (!affiliation)
			return false;
		string key = affiliation.GetAffiliatedFactionKey();
		if (!key.IsEmpty() && key != m_sFactionKey && key != m_sAssetFactionKey)
			return false;
		affiliation.SetAffiliatedFactionByKey(m_sFactionKey);
		return affiliation.GetAffiliatedFactionKey() == m_sFactionKey;
	}

	float GetDefenceHalfExtent()
	{
		if (m_fDefenceHalfExtent > 0)
			return Math.Clamp(m_fDefenceHalfExtent, 12, 64);
		return AROR_DefenceLayout.HalfExtent(m_Location.GetLocationType());
	}
	float GetMaxDefenceRelief() { return m_fMaxDefenceRelief; }
	float GetEditorSearchRadius() { return Math.Clamp(m_fEditorSearchRadius, 25, 1600); }
	bool IsDefenceCenterAllowed(vector position, float halfExtent)
	{
		if (!m_bFixedDefenceAnchor)
			return true;
		return m_Location && vector.DistanceXZ(position, m_Location.GetCenter()) + (halfExtent + 2) * 1.414214 <= m_Location.GetRadius();
	}
	vector GetDefenceCenter()
	{
		IEntity child = GetOwner().GetChildren();
		while (child)
		{
			if (child.FindComponent(AROR_CaptureFlagPlacementComponent))
				return child.GetOrigin();
			child = child.GetSibling();
		}
		return m_Location.GetCenter();
	}
	protected int PositionOrdinal(int index)
	{
		int kind = m_aSlots[index].m_iKind;
		int ordinal;
		for (int previous = 0; previous < index; previous++)
		{
			if (m_aSlots[previous].m_iKind == kind)
				ordinal++;
		}
		if (kind == 0)
			return ordinal / 4;
		if (kind == 4)
			return m_aSlots[index].m_iGroup;
		return ordinal;
	}
	protected IEntity FindDefencePosition(int kind, int ordinal)
	{
		IEntity child = GetOwner().GetChildren();
		while (child)
		{
			AROR_DefencePositionComponent position = AROR_DefencePositionComponent.Cast(child.FindComponent(AROR_DefencePositionComponent));
			if (position && position.MatchesDefencePosition(kind, ordinal))
				return child;
			child = child.GetSibling();
		}
		return null;
	}
	protected bool HasClearMGArc(vector point, float yaw)
	{
		return AROR_DefenceLayout.ClearMGArc(GetGame().GetWorld(), point, yaw);
	}
	bool HasSuitableDefenceTerrain_S()
	{
		if (!Replication.IsServer())
			return false;
		if (!m_bDefenceTerrainChecked)
		{
			if (!IsDefenceCenterAllowed(GetDefenceCenter(), GetDefenceHalfExtent()))
			{
				m_bDefenceTerrainChecked = true;
				m_bDefenceTerrainFits = false;
				ReportFailure("defence perimeter exceeds fixed strategic capture radius");
				return false;
			}
			float relief;
			vector siteAngles = GetOwner().GetYawPitchRoll();
			m_bDefenceTerrainFits = AROR_DefenceLayout.TerrainFits(GetGame().GetWorld(), GetDefenceCenter(), GetDefenceHalfExtent() + 2, m_fMaxDefenceRelief, relief, siteAngles[0]);
			m_bDefenceTerrainChecked = true;
			if (!m_bDefenceTerrainFits)
				ReportFailure(string.Format("perimeter terrain rejected: half-width=%1 relief=%2 limit=%3; relocate to an audited flat site", GetDefenceHalfExtent(), relief, m_fMaxDefenceRelief));
		}
		return m_bDefenceTerrainFits;
	}

	//! Fixed flag-centred positions, not scattered points across the location's capture radius.
	protected bool PlaceSlot(int index, float playerExclusion)
	{
		AROR_GarrisonSlot slot = m_aSlots[index];
		if (!HasSuitableDefenceTerrain_S())
			return false;
		if (slot.m_bPlaced)
		{
			IEntity currentMarker = FindDefencePosition(slot.m_iKind, PositionOrdinal(index));
			if (currentMarker && vector.DistanceXZ(slot.m_vPosition, currentMarker.GetOrigin()) > 1.5)
				slot.m_bPlaced = false; // Layout edits invalidate positions, never casualties or the stored recipe.
			vector savedLocal = GetOwner().VectorToLocal(slot.m_vPosition - GetDefenceCenter());
			if (Math.AbsFloat(savedLocal[0]) > GetDefenceHalfExtent() - 2 || Math.AbsFloat(savedLocal[2]) > GetDefenceHalfExtent() - 2)
				slot.m_bPlaced = false;
		}
		if (slot.m_bPlaced)
		{
			if (!IsClear(slot.m_vPosition, slot.m_iKind, index, playerExclusion))
				return false;
			return slot.m_iKind != 1 || HasClearMGArc(slot.m_vPosition, slot.m_vAngles[0]);
		}
		int ordinal = PositionOrdinal(index);
		IEntity marker = FindDefencePosition(slot.m_iKind, ordinal);
		vector seed;
		vector ownerAngles = GetOwner().GetYawPitchRoll();
		float yaw = ownerAngles[0] + AROR_DefenceLayout.Facing(slot.m_iKind, ordinal);
		if (marker)
		{
			seed = marker.GetOrigin();
			vector markerAngles = marker.GetYawPitchRoll();
			yaw = markerAngles[0];
		}
		else if (slot.m_iKind > 0 && slot.m_iKind < 4 && m_aDefencePositions && !m_aDefencePositions.IsEmpty())
		{
			int padOrdinal;
			for (int previous = 0; previous < index; previous++)
			{
				if (m_aSlots[previous].m_iKind > 0 && m_aSlots[previous].m_iKind < 4)
					padOrdinal++;
			}
			if (padOrdinal >= m_aDefencePositions.Count())
				return false; // Explicit layouts never fall through to scattered automatic positions.
			seed = m_aDefencePositions[padOrdinal];
		}
		else
			seed = GetDefenceCenter() + GetOwner().VectorToParent(AROR_DefenceLayout.LocalPosition(slot.m_iKind, ordinal, GetDefenceHalfExtent()));
		for (int attempt = 0; attempt < 9; attempt++)
		{
			vector position = seed + AROR_DefenceLayout.RepairOffset(attempt);
			position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]);
			if (!IsClear(position, slot.m_iKind, index, playerExclusion))
				continue;
			if (slot.m_iKind == 1 && !HasClearMGArc(position, yaw))
				continue;
			slot.m_vPosition = position;
			slot.m_vAngles = Vector(yaw, 0, 0);
			slot.m_bPlaced = true;
			return true;
		}
		return false;
	}

	protected bool HasPlayerClearance(vector position, int kind, float playerExclusion)
	{
		if (kind == 4)
			playerExclusion = Math.Min(playerExclusion, m_fCrewPlayerClearance);
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int playerId : players)
		{
			IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (player && vector.DistanceXZ(player.GetOrigin(), position) < playerExclusion)
				return false;
		}
		return true;
	}

	protected bool IsClear(vector position, int kind, int index, float playerExclusion)
	{
		BaseWorld world = GetGame().GetWorld();
		float size = 5;
		if (kind == 1)
			size = 1.5;
		if (kind == 0)
			size = 2.5; // Check room for a four-person team, not just its first soldier.
		if (kind == 4)
			size = 0.6;
		if (position[1] <= world.GetOceanHeight(position[0], position[2]) + 0.2 || !m_Location.IsPositionInside(position))
			return false;
		vector local = GetOwner().VectorToLocal(position - GetDefenceCenter());
		if (Math.AbsFloat(local[0]) > GetDefenceHalfExtent() - 2 || Math.AbsFloat(local[2]) > GetDefenceHalfExtent() - 2)
			return false;
		if (Math.AbsFloat(world.GetSurfaceY(position[0] + size, position[2]) - position[1]) > 1 || Math.AbsFloat(world.GetSurfaceY(position[0], position[2] + size) - position[1]) > 1 || Math.AbsFloat(world.GetSurfaceY(position[0] - size, position[2]) - position[1]) > 1 || Math.AbsFloat(world.GetSurfaceY(position[0], position[2] - size) - position[1]) > 1)
			return false;
		for (int other = 0; other < m_aSlots.Count(); other++)
		{
			AROR_GarrisonSlot existing = m_aSlots[other];
			float spacing = 12;
			if (kind == 0 || kind == 4)
				spacing = 7;
			if (other != index && existing.m_iKind > 0 && existing.m_iKind < 4 && existing.m_bPlaced && !existing.m_bLost && vector.DistanceXZ(position, existing.m_vPosition) < spacing)
				return false;
		}
		if (!HasPlayerClearance(position, kind, playerExclusion))
			return false;
		TraceBox trace = new TraceBox();
		ChimeraAIWorld aiWorld = ChimeraAIWorld.Cast(GetGame().GetAIWorld());
		NavmeshWorldComponent soldiers;
		if (aiWorld)
			soldiers = aiWorld.GetNavmeshWorldComponent("Soldiers");
		if (!soldiers)
			return false;
		if (!soldiers.IsTileLoaded(position))
		{
			soldiers.LoadTileIn(position);
			return false;
		}
		vector reachable;
		if (!soldiers.GetReachablePoint(position, 2, reachable) || vector.DistanceXZ(position, reachable) > 3 || Math.AbsFloat(position[1] - reachable[1]) > 1)
			return false;
		trace.Start = position + Vector(0, 0.25, 0);
		trace.End = trace.Start + Vector(0, 0.05, 0);
		trace.Mins = Vector(-size, 0, -size);
		trace.Maxs = Vector(size, 3, size);
		trace.Flags = TraceFlags.ENTS | TraceFlags.VISIBILITY;
		return world.TraceMove(trace, null) == 1;
	}

	protected IEntity Spawn(ResourceName prefab, vector position, vector angles)
	{
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return null;
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		vector rotation[3];
		Math3D.AnglesToMatrix(angles, rotation);
		for (int axis = 0; axis < 3; axis++)
			spawnParams.Transform[axis] = rotation[axis];
		spawnParams.Transform[3] = position;
		return GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
	}

	//! One hull per call; native crew seat order is saved alongside the actual asset recipe.
	bool SpawnNextAsset_S(AROR_GarrisonFaction faction, float exclusion)
	{
		if (!Replication.IsServer())
			return false;
		m_bActive = true;
		int count = m_aSlots.Count();
		for (int offset = 0; offset < count; offset++)
		{
			int index = Math.Mod(m_iNextAsset + offset, count);
			AROR_GarrisonSlot slot = m_aSlots[index];
			if (slot.m_bLost || slot.m_bPlacementUnavailable || slot.m_bSpawned || slot.m_iKind == 0 || slot.m_iKind == 4)
				continue;
			m_iNextAsset = index + 1;
			if (!PlaceSlot(index, exclusion))
			{
				ReportPlacementFailure(index, "no clear asset pad (terrain, spacing, player clearance, navigation or obstruction)");
				return false;
			}
			IEntity entity = Spawn(slot.m_sPrefab, slot.m_vPosition, slot.m_vAngles);
			slot.m_iPlacementFailureSince = -1;
			if (!entity)
			{
				ReportFailure("asset resource failed: " + slot.m_sPrefab);
				return false;
			}
			if (!InitializeFaction(entity))
			{
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
				ReportFailure("asset faction binding failed: " + slot.m_sPrefab);
				return false;
			}
			array<BaseCompartmentSlot> availableSeats = {};
			if (slot.m_iKind != 2)
			{
				BaseCompartmentManagerComponent compartments = BaseCompartmentManagerComponent.Cast(entity.FindComponent(BaseCompartmentManagerComponent));
				if (compartments)
					compartments.GetCompartments(availableSeats);
				int crewCount;
				foreach (BaseCompartmentSlot available : availableSeats)
				{
					if (available && !CargoCompartmentSlot.Cast(available))
						crewCount++;
				}
				int maximumCrew = 4;
				if (slot.m_iKind == 1)
					maximumCrew = 1;
				if (crewCount < 1 || crewCount > maximumCrew)
				{
					SCR_EntityHelper.DeleteEntityAndChildren(entity);
					ReportFailure("asset has no usable crew compartments or exceeds its reserved crew allowance");
					return false;
				}
			}
			slot.m_Entity = entity;
			slot.m_bSpawned = true;
			DamageManagerComponent damage = DamageManagerComponent.Cast(entity.FindComponent(DamageManagerComponent));
			if (damage)
				damage.SetHealthScaled(slot.m_fHealth);
			if (slot.m_iKind != 2)
			{
				bool hasCrew;
				foreach (AROR_GarrisonSlot crew : m_aSlots)
				{
					if (crew.m_iVehicle == index)
						hasCrew = true;
				}
				if (!hasCrew)
				{
					for (int seat = 0; seat < availableSeats.Count(); seat++)
					{
						if (availableSeats[seat] && !CargoCompartmentSlot.Cast(availableSeats[seat]))
							AddSlot(faction.m_sCrew, 4, slot.m_iGroup, index, seat);
					}
				}
			}
			Print(string.Format("[AROR GARRISON] %1 asset slot=%2 kind=%3 at %4", m_Location.GetId(), index, slot.m_iKind, slot.m_vPosition), LogLevel.NORMAL);
			m_bDeploymentDeferred = false;
			return true;
		}
		return false;
	}

	//! Queue one team through the engine's shared AI queue, never spawn every soldier in one tick.
	bool SpawnNextGroup_S(int now, float exclusion)
	{
		if (!Replication.IsServer())
			return false;
		int maximum = -1;
		foreach (AROR_GarrisonSlot member : m_aSlots)
			maximum = Math.Max(maximum, member.m_iGroup);
		int firstGroup = m_iNextGroup;
		for (int groupOffset = 0; groupOffset <= maximum; groupOffset++)
		{
			int groupId = Math.Mod(firstGroup + groupOffset, maximum + 1);
			bool requested;
			foreach (AROR_GarrisonGroup existing : m_aGroups)
			{
				if (existing.m_iId == groupId)
					requested = true;
			}
			if (requested)
				continue;
			array<int> indices = {};
			foreach (int index, AROR_GarrisonSlot candidate : m_aSlots)
			{
				if (candidate.m_iGroup == groupId && !candidate.m_bLost && !candidate.m_bPlacementUnavailable && (candidate.m_iKind == 0 || candidate.m_iKind == 4))
				{
					if (candidate.m_iVehicle >= 0 && !m_aSlots[candidate.m_iVehicle].m_bSpawned)
						continue;
					indices.Insert(index);
				}
			}
			if (indices.IsEmpty())
			{
				continue;
			}
			m_iNextGroup = groupId + 1;
			AROR_GarrisonSlot first = m_aSlots[indices[0]];
			if (!PlaceSlot(indices[0], exclusion))
			{
				ReportPlacementFailure(indices[0], "no clear walkable team/crew spawn point");
				return false;
			}
			vector position = first.m_vPosition;
			first.m_iPlacementFailureSince = -1;
			SCR_AIGroup.IgnoreSpawning(true);
			IEntity groupEntity = Spawn("{A22D9D957ABE44DF}Prefabs/Groups/AROR_GarrisonGroup.et", position, vector.Zero);
			SCR_AIGroup.IgnoreSpawning(false);
			SCR_AIGroup group = SCR_AIGroup.Cast(groupEntity);
			if (!group)
			{
				if (groupEntity)
					SCR_EntityHelper.DeleteEntityAndChildren(groupEntity);
				return false;
			}
			Faction faction = GetGame().GetFactionManager().GetFactionByKey(m_sFactionKey);
			bool assigned;
			if (faction)
			{
				if (!group.GetFaction())
					assigned = group.InitFactionKey(m_sFactionKey);
				else
					assigned = group.SetFaction(faction);
			}
			if (!assigned)
			{
				SCR_EntityHelper.DeleteEntityAndChildren(group);
				return false;
			}
			group.SetCanDeleteIfNoPlayer(false);
			group.SetPrivate(true);
			group.SetPrivacyChangeable(false);
			group.SetLifecyclePolicy(SCR_EAIGroupLifecyclePolicy.Manual);
			if (!group.m_aUnitPrefabSlots)
				group.m_aUnitPrefabSlots = {};
			group.m_aUnitPrefabSlots.Clear();
			foreach (int slotIndex : indices)
				group.m_aUnitPrefabSlots.Insert(m_aSlots[slotIndex].m_sPrefab);
			AROR_GarrisonGroup pending = new AROR_GarrisonGroup();
			pending.m_iId = groupId;
			pending.m_Group = group;
			pending.m_aSlots.Copy(indices);
			pending.m_iQueuedAt = now;
			m_aGroups.Insert(pending);
			AROR_QRFDefenseComponent defense = AROR_QRFDefenseComponent.Cast(group.FindComponent(AROR_QRFDefenseComponent));
			if (defense)
				defense.BindDefense_S(m_eRosterRole, m_Location.GetId());
			group.RequestSpawn(indices.Count());
			m_bDeploymentDeferred = false;
			Print(string.Format("[AROR GARRISON] %1 group=%2 queued personnel=%3", m_Location.GetId(), groupId, indices.Count()), LogLevel.NORMAL);
			m_iNextGroup = groupId + 1;
			return true;
		}
		return false;
	}

	protected void AssignMembers(AROR_GarrisonGroup pending)
	{
		if (!pending.m_Group)
			return;
		array<AIAgent> agents = {};
		pending.m_Group.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			IEntity entity;
			if (agent)
				entity = agent.GetControlledEntity();
			if (!entity)
				continue;
			bool known;
			foreach (int knownIndex : pending.m_aSlots)
			{
				if (m_aSlots[knownIndex].m_Entity == entity)
					known = true;
			}
			if (known)
				continue;
			AROR_GarrisonSlot slot;
			foreach (int availableIndex : pending.m_aSlots)
			{
				AROR_GarrisonSlot candidate = m_aSlots[availableIndex];
				if (!candidate.m_bLost && !candidate.m_Entity)
				{
					slot = candidate;
					break;
				}
			}
			if (!slot)
				continue;
			slot.m_Entity = entity;
			slot.m_bSpawned = true;
			if (!InitializeFaction(entity))
			{
				slot.m_bLost = true;
				continue;
			}
			DamageManagerComponent damage = DamageManagerComponent.Cast(entity.FindComponent(DamageManagerComponent));
			if (damage)
				damage.SetHealthScaled(slot.m_fHealth);
		}
	}

	protected void CrewAndOrder(AROR_GarrisonGroup pending)
	{
		if (!pending.m_Group || pending.m_bOrdered)
			return;
		foreach (int expected : pending.m_aSlots)
		{
			if (!m_aSlots[expected].m_bLost && !m_aSlots[expected].m_Entity)
				return;
		}
		bool ready = true;
		foreach (int index : pending.m_aSlots)
		{
			AROR_GarrisonSlot slot = m_aSlots[index];
			if (slot.m_iVehicle < 0 || slot.m_bLost || !slot.m_Entity)
				continue;
			AROR_GarrisonSlot vehicle = m_aSlots[slot.m_iVehicle];
			if (!vehicle.m_Entity || vehicle.m_bLost)
				continue; // Hull loss leaves surviving crew as defenders on foot.
			BaseCompartmentManagerComponent manager = BaseCompartmentManagerComponent.Cast(vehicle.m_Entity.FindComponent(BaseCompartmentManagerComponent));
			array<BaseCompartmentSlot> seats = {};
			if (manager)
				manager.GetCompartments(seats);
			if (slot.m_iSeat < 0 || slot.m_iSeat >= seats.Count() || !seats[slot.m_iSeat])
			{
				ready = false;
				continue;
			}
			BaseCompartmentSlot seat = seats[slot.m_iSeat];
			if (seat.GetOccupant() != slot.m_Entity)
			{
				CompartmentAccessComponent access = CompartmentAccessComponent.Cast(slot.m_Entity.FindComponent(CompartmentAccessComponent));
				if (access && !seat.IsOccupied())
					access.GetInVehicle(vehicle.m_Entity, seat, true, -1, ECloseDoorAfterActions.CLOSE_DOOR, false);
				ready = false;
			}
			SCR_AIVehicleUsageComponent usage = SCR_AIVehicleUsageComponent.Cast(vehicle.m_Entity.FindComponent(SCR_AIVehicleUsageComponent));
			if (usage && pending.m_Group.GetGroupUtilityComponent())
				pending.m_Group.GetGroupUtilityComponent().AddUsableVehicle(usage);
		}
		if (!ready)
			return;
		if (pending.m_Waypoint)
		{
			pending.m_Group.RemoveWaypoint(pending.m_Waypoint);
			SCR_EntityHelper.DeleteEntityAndChildren(pending.m_Waypoint);
		}
		AROR_GarrisonSlot lead = m_aSlots[pending.m_aSlots[0]];
		vector defend = lead.m_vPosition;
		if (lead.m_iVehicle >= 0 && m_aSlots[lead.m_iVehicle].m_Entity)
			defend = m_aSlots[lead.m_iVehicle].m_Entity.GetOrigin();
		IEntity waypointEntity = Spawn("{93291E72AC23930F}Prefabs/AI/Waypoints/AIWaypoint_Defend.et", defend, vector.Zero);
		pending.m_Waypoint = AIWaypoint.Cast(waypointEntity);
		if (!pending.m_Waypoint)
			return;
		pending.m_Waypoint.SetCompletionRadius(m_fDefenceHoldRadius);
		pending.m_Group.AddWaypoint(pending.m_Waypoint);
		pending.m_bOrdered = true;
		Print(string.Format("[AROR GARRISON] %1 group ready with verified crew and defend orders", m_Location.GetId()), LogLevel.NORMAL);
	}

	//! Native dormancy is not a confirmed death. Keep its reserve until the engine rematerialises it.
	protected bool HasDormantReserve(AROR_GarrisonSlot slot)
	{
		if (slot.m_iKind != 0 && slot.m_iKind != 4)
			return false; // A dormant crew is not evidence that its missing hull still exists.
		foreach (AROR_GarrisonGroup group : m_aGroups)
		{
			if (group.m_iId == slot.m_iGroup && group.m_Group && group.m_Group.IsDormant() && group.m_Group.GetDormantAliveCount() > 0)
				return true;
		}
		return false;
	}

	//! Save losses before any retirement; virtualisation must never count as combat damage.
	void Observe_S(bool issueOrders = true)
	{
		if (!Replication.IsServer())
			return;
		foreach (AROR_GarrisonGroup pending : m_aGroups)
		{
			AssignMembers(pending);
			if (issueOrders && pending.m_Group && pending.m_Group.IsDormant() && pending.m_Group.GetDormantAliveCount() > 0)
			{
				pending.m_bOrdered = false;
				int now = GetGame().GetWorld().GetWorldTime();
				if (now >= pending.m_iNextWake)
				{
					pending.m_iNextWake = now + 30000;
					pending.m_Group.RequestSpawn();
				}
			}
		}
		foreach (AROR_GarrisonSlot slot : m_aSlots)
		{
			if (slot.m_bLost || !slot.m_bSpawned)
				continue;
			if (!slot.m_Entity)
			{
				if (HasDormantReserve(slot))
					continue;
				slot.m_bLost = true; // Missing previously physical assets must not regenerate.
				continue;
			}
			if (IsPlayerOrForeignAsset(slot.m_Entity))
			{
				slot.m_bLost = true;
				slot.m_Entity = null; // Explicit transfer; garrison never deletes or respawns it.
				continue;
			}
			DamageManagerComponent damage = DamageManagerComponent.Cast(slot.m_Entity.FindComponent(DamageManagerComponent));
			ChimeraCharacter character = ChimeraCharacter.Cast(slot.m_Entity);
			if ((damage && damage.IsDestroyed()) || (character && character.GetCharacterController() && character.GetCharacterController().IsDead()))
				slot.m_bLost = true;
			if (damage)
				slot.m_fHealth = Math.Min(slot.m_fHealth, damage.GetHealthScaled());
		}
		if (issueOrders && m_Location.GetControllingRole() == m_eRosterRole)
		{
			foreach (AROR_GarrisonGroup group : m_aGroups)
				CrewAndOrder(group);
		}
	}

	//! Never delete a stolen/foreign-occupied asset or a player-controlled soldier.
	protected bool IsPlayerOrForeignAsset(IEntity entity)
	{
		if (GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity) > 0)
			return true;
		FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (affiliation && !affiliation.GetAffiliatedFactionKey().IsEmpty() && affiliation.GetAffiliatedFactionKey() != m_sFactionKey)
			return true;
		BaseCompartmentManagerComponent manager = BaseCompartmentManagerComponent.Cast(entity.FindComponent(BaseCompartmentManagerComponent));
		if (!manager)
			return false;
		array<BaseCompartmentSlot> seats = {};
		manager.GetCompartments(seats);
		foreach (BaseCompartmentSlot seat : seats)
		{
			if (!seat)
				continue;
			IEntity occupant = seat.GetOccupant();
			if (!occupant)
				continue;
			bool occupantIsOwned;
			foreach (AROR_GarrisonSlot slot : m_aSlots)
			{
				if (slot.m_Entity == occupant)
					occupantIsOwned = true;
			}
			if (!occupantIsOwned || GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(occupant) > 0)
				return true;
		}
		return false;
	}

	bool CanRetire(float distance)
	{
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int id : players)
		{
			IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(id);
			if (!player)
				continue;
			if (vector.DistanceXZ(player.GetOrigin(), m_Location.GetCenter()) <= distance)
				return false;
			foreach (AROR_GarrisonSlot slot : m_aSlots)
			{
				if (slot.m_Entity && vector.DistanceXZ(player.GetOrigin(), slot.m_Entity.GetOrigin()) <= distance)
					return false;
			}
		}
		return true;
	}

	void Retire_S()
	{
		if (!Replication.IsServer())
			return;
		Observe_S(false);
		ClearAssignments_S();
		foreach (AROR_GarrisonGroup group : m_aGroups)
		{
			if (group.m_Waypoint)
				SCR_EntityHelper.DeleteEntityAndChildren(group.m_Waypoint);
			if (group.m_Group && group.m_Group.GetPlayerCount() == 0)
				SCR_EntityHelper.DeleteEntityAndChildren(group.m_Group); // Cancels outstanding native queue requests.
		}
		m_aGroups.Clear();
		foreach (AROR_GarrisonSlot slot : m_aSlots)
		{
			if (slot.m_Entity)
			{
				if (IsPlayerOrForeignAsset(slot.m_Entity) || (slot.m_iKind > 0 && slot.m_iKind < 4 && !m_Location.IsPositionInside(slot.m_Entity.GetOrigin())))
					slot.m_bLost = true; // Transfer to the world; no subsequent respawn.
				else
					SCR_EntityHelper.DeleteEntityAndChildren(slot.m_Entity);
			}
			slot.m_Entity = null;
			slot.m_bSpawned = false;
			slot.m_bPlacementUnavailable = false;
			slot.m_iPlacementFailureSince = -1;
		}
		m_bActive = false;
		m_iNextGroup = 0;
		m_iNextAsset = 0;
		m_bDefenceTerrainChecked = false;
		m_bDeploymentDeferred = false;
	}

	//! Ownership mismatch is reconciled only after old physical actors can safely retire.
	void ResetRoster_S()
	{
		if (!Replication.IsServer() || m_bActive)
			return;
		m_aSlots.Clear();
		m_bGenerated = false;
	}
	void RestoreRoster_S(AROR_EFactionRole role, string faction, string assetFaction, int level, array<ref AROR_GarrisonSlot> slots)
	{
		if (!Replication.IsServer() || m_bActive)
			return;
		m_eRosterRole = role;
		m_sFactionKey = faction;
		m_sAssetFactionKey = assetFaction;
		m_iLevel = level;
		m_aSlots.Clear();
		foreach (AROR_GarrisonSlot slot : slots)
			m_aSlots.Insert(slot);
		m_bGenerated = true;
	}
	bool OutsideLongEnough(bool outside, int now, int delay)
	{
		if (!outside)
			m_iOutsideSince = -1;
		else if (m_iOutsideSince < 0)
			m_iOutsideSince = now;
		return outside && now - m_iOutsideSince >= delay;
	}
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_Location = AROR_LocationComponent.Cast(owner.FindComponent(AROR_LocationComponent));
	}
	override void OnDelete(IEntity owner)
	{
		if (GetGame() && Replication.IsServer() && m_bActive)
			Retire_S();
		super.OnDelete(owner);
	}
}
