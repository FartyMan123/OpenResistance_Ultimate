//! Optional tactical policy on the existing server executor; no separate spawner or faction system.
modded class AROR_OperationExecutorComponent
{
	[Attribute("0", desc: "Enable shared QRF tactical coordination and finite staged waves for both enemy roles")]
	protected bool m_bQRFTacticalCommander;
	[Attribute("1", desc: "Keep later-wave elements as reservations without actors until released")]
	protected bool m_bQRFTwoWaves;
	[Attribute("3", desc: "Minimum tier eligible for two waves (requires at least four elements)", params: "1 5 1")]
	protected int m_iQRFSplitTier;
	[Attribute("60", desc: "Seconds after first dispatch before reserve release; losses can shorten this to 15 seconds", params: "15 180 1")]
	protected int m_iQRFReinforcementDelay;
	[Attribute("300", desc: "Cancel unspawned reserves after this many seconds from first dispatch", params: "60 480 1")]
	protected int m_iQRFReserveWait;
	[Attribute("6", desc: "Maximum live native ground packages before constructing more tactical QRF elements", params: "4 8 1")]
	protected int m_iQRFMaxLiveGround;
	[Attribute("6", desc: "Maximum live aircraft before constructing more QRF transport/gunship pairs", params: "6 10 1")]
	protected int m_iQRFMaxLiveAircraft;
	[Attribute("600", desc: "Seconds of active tactical tasking before ordered withdrawal", params: "180 720 1")]
	protected int m_iQRFTacticalDuration;
	[Attribute("15", desc: "Minimum seconds between tactical contact retasks and routine order updates", params: "10 60 1")]
	protected int m_iQRFOrderInterval;
	[Attribute("200", desc: "Minimum vehicle dismount standoff with fresh hostile reports, metres", params: "100 350 1")]
	protected float m_fQRFThreatDismount;
	[Attribute("0.4", desc: "Living fraction at or below which the QRF withdraws when no reserve is available", params: "0.1 0.7 0.05")]
	protected float m_fQRFWithdrawFraction;
	[Attribute("0.5", desc: "Individual package living fraction triggering local withdrawal", params: "0.1 0.8 0.05")]
	protected float m_fQRFLocalWithdrawFraction;
	[Attribute("120", desc: "Minimum terminal ground casualty retention before distance-based cleanup", params: "30 600 1")]
	protected int m_iQRFGroundRetention;
	[Attribute("1200", desc: "Every owned ground member/vehicle must be this far from all players before tactical removal", params: "500 2500 1")]
	protected float m_fQRFGroundCleanupDistance;
	protected int m_iLastQRFConstructionTick = -1;
	protected int m_iQRFConstructionIndex = -1;
	protected string m_sQRFConstructionOperation;

	protected bool TacticalSettingsValid()
	{
		if (m_iQRFSplitTier < 1 || m_iQRFSplitTier > 5)
			return false;
		if (m_iQRFReinforcementDelay < 15 || m_iQRFReinforcementDelay > 180)
			return false;
		if (m_iQRFReserveWait < 60 || m_iQRFReserveWait > 480)
			return false;
		if (m_iQRFReserveWait <= m_iQRFReinforcementDelay)
			return false;
		if (m_iQRFMaxLiveGround < 4 || m_iQRFMaxLiveGround > 8)
			return false;
		if (m_iQRFMaxLiveAircraft < 6 || m_iQRFMaxLiveAircraft > 10)
			return false;
		if (m_iQRFTacticalDuration < 180 || m_iQRFTacticalDuration > 720)
			return false;
		if (m_iQRFOrderInterval < 10 || m_iQRFOrderInterval > 60)
			return false;
		if (m_fQRFThreatDismount < 100 || m_fQRFThreatDismount > 350)
			return false;
		if (m_fQRFWithdrawFraction < 0.1 || m_fQRFWithdrawFraction > 0.7)
			return false;
		if (m_fQRFLocalWithdrawFraction < 0.1 || m_fQRFLocalWithdrawFraction > 0.8)
			return false;
		if (m_iQRFGroundRetention < 30 || m_iQRFGroundRetention > 600)
			return false;
		return m_fQRFGroundCleanupDistance >= 500 && m_fQRFGroundCleanupDistance <= 2500;
	}

	protected void CreateQRFTacticalCommander(AROR_Operation operation, int packages, vector origin)
	{
		if (!m_bQRFTacticalCommander || operation.GetType() != AROR_EOperationType.QRF || operation.m_TacticalCommander)
			return;
		AROR_LocationManagerComponent locations = AROR_LocationManagerComponent.GetInstance();
		AROR_LocationComponent target;
		if (locations)
			target = locations.GetLocationById(operation.GetTargetLocationId());
		if (!target)
			return;
		AROR_QRFTacticalCommander commander = new AROR_QRFTacticalCommander();
		commander.Initialize(operation.GetResponseLevel(), packages, m_bQRFTwoWaves, m_iQRFSplitTier, m_iQRFReinforcementDelay, m_iQRFReserveWait, m_iQRFTacticalDuration, m_iQRFOrderInterval, m_fQRFWithdrawFraction);
		commander.m_vTown = target.GetCenter();
		commander.m_fTownRadius = target.GetRadius();
		commander.m_fMargin = m_fQRFSearchMargin;
		commander.m_fThreatStandoff = m_fQRFThreatDismount;
		commander.m_vOrigin = origin;
		array<AROR_LocationComponent> outposts = {};
		locations.GetLocationsOfType(AROR_ELocationType.OUTPOST, outposts);
		foreach (AROR_LocationComponent outpost : outposts)
		{
			if (outpost && outpost.GetControllingRole() == operation.GetRole() && vector.DistanceXZ(origin, outpost.GetCenter()) < 1)
				commander.m_sOriginLocationId = outpost.GetId();
		}
		operation.m_TacticalCommander = commander;
		Print(string.Format("[AROR TACTICAL] %1 role=%2 tier=%3 first wave=%4 reserve=%5; entire force reserved, one construction per tick", operation.GetId(), operation.GetRole(), operation.GetResponseLevel(), commander.m_iFirstWaveCount, packages - commander.m_iFirstWaveCount), LogLevel.NORMAL);
	}

	protected void QueueTacticalGroundQRF(AROR_Operation operation, array<AROR_ForceProfile> elements)
	{
		AROR_LocationManagerComponent locations = AROR_LocationManagerComponent.GetInstance();
		AROR_LocationComponent target;
		if (locations)
			target = locations.GetLocationById(operation.GetTargetLocationId());
		if (!target)
		{
			FailOperation(operation, "Tactical QRF target unavailable");
			return;
		}
		vector origin = m_vOccupiersSpawnPosition;
		if (operation.GetRole() == AROR_EFactionRole.INVADERS)
			origin = m_vInvadersSpawnPosition;
		origin = ResolveQRFOrigin(operation, target, origin);
		CreateQRFTacticalCommander(operation, elements.Count(), origin);
		if (!operation.m_TacticalCommander)
		{
			FailOperation(operation, "Cannot create QRF tactical plan without target");
			return;
		}
		operation.m_TacticalCommander.BalanceGroundWaves(elements);
		if (operation.m_TacticalCommander.m_iFirstWaveCount > m_iQRFMaxLiveGround)
		{
			FailOperation(operation, "QRF lead wave exceeds live-ground limit; enable waves, reduce recipe or raise configured limit");
			return;
		}
		for (int index = 0; index < elements.Count(); index++)
		{
			AROR_QRFDeferredForce pending = new AROR_QRFDeferredForce();
			pending.m_sOperationId = operation.GetId();
			pending.m_iPackageIndex = index;
			pending.m_bReservation = true;
			pending.m_Profile = elements[index];
			pending.m_iReservedGroups = pending.m_Profile.GetGroupBudget();
			pending.m_iReservedMembers = pending.m_Profile.m_iPersonnelBudget;
			if (pending.m_Profile.m_eType != AROR_EForceType.INFANTRY)
				pending.m_iReservedVehicles = 1;
			m_aForces.Insert(pending);
		}
	}

	protected int LiveGroundQRFCount()
	{
		int count = 0;
		foreach (AROR_OperationForce force : m_aForces)
		{
			if (force.m_bReservation || AROR_ReaperAirForce.Cast(force))
				continue;
			if (force.m_bTacticalRetained)
			{
				int living = force.CountLivingMembers();
				if (force.m_Group && force.m_Group.IsDormant())
					living = Math.Max(living, force.m_Group.GetDormantAliveCount());
				AROR_GroundOperationForce ground = AROR_GroundOperationForce.Cast(force);
				if (ground)
				{
					living += ground.CountCrew();
					if (ground.m_CrewGroup && ground.m_CrewGroup.IsDormant())
						living += ground.m_CrewGroup.GetDormantAliveCount();
				}
				if (living == 0)
					continue;
			}
			count++;
		}
		return count;
	}
	protected bool HasPendingQRF(string operationId)
	{
		foreach (AROR_OperationForce force : m_aForces)
		{
			if (force.m_sOperationId == operationId && AROR_QRFDeferredForce.Cast(force))
				return true;
			AROR_ReaperAirForce air = AROR_ReaperAirForce.Cast(force);
			if (force.m_sOperationId == operationId && air && air.m_bConstructionPending)
				return true;
		}
		return false;
	}
	protected bool HasOperatingQRF(string operationId)
	{
		foreach (AROR_OperationForce force : m_aForces)
		{
			if (force.m_sOperationId != operationId || force.m_bTacticalRetained)
				continue;
			if (!force.m_bReservation || AROR_QRFDeferredForce.Cast(force))
				return true;
		}
		return false;
	}
	protected int LiveTacticalAircraftCount()
	{
		int count = AROR_ReaperRetirement.LiveAircraftCount();
		foreach (AROR_OperationForce tracked : m_aForces)
		{
			AROR_ReaperAirForce air = AROR_ReaperAirForce.Cast(tracked);
			if (!air || air.m_bConstructionPending)
				continue;
			if (AROR_ReaperAirForce.AircraftAlive(air.m_Vehicle))
				count++;
			if (air.m_Escort && AROR_ReaperAirForce.AircraftAlive(air.m_Escort.m_Vehicle))
				count++;
		}
		return count;
	}

	protected void WithdrawTacticalAir(AROR_ReaperAirForce force, AROR_Operation operation)
	{
		AROR_QRFTacticalCommander commander = operation.m_TacticalCommander;
		force.m_iSupportEndTime = m_iExecutorClock;
		MonitorReaperEscort(force);
		if (!force.m_bTacticalWithdrawing)
		{
			force.m_bTacticalWithdrawing = true;
			if (!force.m_bTransportRetired && !force.m_bTransportLost && force.m_CrewGroup && force.m_CrewGroup.GetPlayerCount() == 0)
				ReaperOrder(force, ReaperMovePrefab(), force.m_vEgress, m_fReaperDepartureSpeed, m_fReaperTransportHeight + force.m_iPackageIndex * 15);
			if (force.m_bDismounted && commander.m_bSafeWithdrawalOrigin)
				ReplaceTacticalWaypoint(force, false, m_sMoveWaypointPrefab, commander.m_vOrigin);
			Print(string.Format("[AROR TACTICAL] %1 air package=%2 withdrawing; no nearby removal", operation.GetId(), force.m_iPackageIndex + 1), LogLevel.WARNING);
		}
		if (TacticalEntitiesFar(force))
		{
			bool escortFar = true;
			if (force.m_Escort)
				escortFar = TacticalEntitiesFar(force.m_Escort);
			if (escortFar)
				FailReaperPackage(force, operation, "Air package withdrew beyond all players; normal retention applies");
		}
	}

	protected void MonitorDeferredQRF(AROR_QRFDeferredForce pending)
	{
		AROR_Operation operation = m_Operations.GetOperation(pending.m_sOperationId);
		if (!operation || operation.IsTerminal())
		{
			ReleaseForce(pending);
			return;
		}
		AROR_QRFTacticalCommander commander = operation.m_TacticalCommander;
		if (!commander || commander.ReserveExpired(m_iExecutorClock))
		{
			ReleaseForce(pending);
			Print(string.Format("[AROR TACTICAL] %1 reserve package=%2 cancelled without spawning; deadline/withdrawal", operation.GetId(), pending.m_iPackageIndex + 1), LogLevel.WARNING);
			if (!HasOperatingQRF(operation.GetId()))
				FailOperation(operation, "No surviving QRF elements or usable reserve");
			return;
		}
		if (m_iLastQRFConstructionTick == m_iExecutorClock)
			return;
		if (!commander.CanConstruct(pending.m_iPackageIndex, m_iExecutorClock, LiveGroundQRFCount(), m_iQRFMaxLiveGround, 1))
			return;
		AROR_CommanderManagerComponent strategic = AROR_CommanderManagerComponent.GetInstance();
		if (!strategic || !strategic.GetCommander(operation.GetRole()))
			return;
		m_iLastQRFConstructionTick = m_iExecutorClock;
		m_iQRFConstructionIndex = pending.m_iPackageIndex;
		m_sQRFConstructionOperation = operation.GetId();
		if (pending.m_Profile.m_eType == AROR_EForceType.INFANTRY)
			StartInfantryForce(operation, pending.m_Profile, pending.m_iPackageIndex, pending);
		else
			StartGroundForce(operation, strategic.GetCommander(operation.GetRole()), pending.m_Profile, pending.m_iPackageIndex, pending);
		m_iQRFConstructionIndex = -1;
		m_sQRFConstructionOperation = string.Empty;
		ReleaseForce(pending);
		if (!operation.IsTerminal() && !HasOperatingQRF(operation.GetId()))
			FailOperation(operation, "No usable elements remain after reserve construction");
		else
			TryActivateOperation(operation);
	}

	protected void TickQRFTacticalCommanders()
	{
		array<AROR_Operation> operations = {};
		m_Operations.GetOperations(operations);
		AROR_LocationManagerComponent locations = AROR_LocationManagerComponent.GetInstance();
		foreach (AROR_Operation operation : operations)
		{
			AROR_QRFTacticalCommander commander = operation.m_TacticalCommander;
			if (!commander || operation.IsTerminal() || m_iExecutorClock < commander.m_iNextContactCheck)
				continue;
			commander.m_iNextContactCheck = m_iExecutorClock + 5;
			bool wasWithdrawing = commander.m_bWithdraw;
			AROR_LocationComponent target;
			if (locations)
				target = locations.GetLocationById(operation.GetTargetLocationId());
			if (!target || target.GetControllingRole() != operation.GetRole())
				commander.m_bWithdraw = true;
			if (locations && !commander.m_sOriginLocationId.IsEmpty())
			{
				AROR_LocationComponent origin = locations.GetLocationById(commander.m_sOriginLocationId);
				if (!origin || origin.GetControllingRole() != operation.GetRole())
				{
					commander.m_bWithdraw = true;
					RetargetWithdrawalOrigin(operation, commander, locations);
				}
			}
			AROR_CombatContact report = m_Operations.GetRecentCombatContact(operation.GetRole(), commander.m_vTown, commander.m_fTownRadius + commander.m_fMargin, m_iQRFContactMaxAge);
			commander.UpdateContact_S(report, m_iExecutorClock);
			commander.RefreshRoster(m_iExecutorClock);
			if (commander.ShouldWithdraw(m_iExecutorClock, HasPendingQRF(operation.GetId())) && !wasWithdrawing)
				Print(string.Format("[AROR TACTICAL] %1 withdrawing: ownership, casualties or tactical duration", operation.GetId()), LogLevel.WARNING);
		}
	}
	protected void RetargetWithdrawalOrigin(AROR_Operation operation, AROR_QRFTacticalCommander commander, AROR_LocationManagerComponent locations)
	{
		array<AROR_LocationComponent> outposts = {};
		locations.GetLocationsOfType(AROR_ELocationType.OUTPOST, outposts);
		AROR_LocationComponent best;
		float bestDistance = float.MAX;
		foreach (AROR_LocationComponent outpost : outposts)
		{
			if (!outpost || outpost.GetControllingRole() != operation.GetRole())
				continue;
			float distance = vector.DistanceXZ(outpost.GetCenter(), commander.m_vTown);
			if (distance < bestDistance && IsLaunchPositionSafe(outpost.GetCenter(), commander.m_vTown))
			{
				best = outpost;
				bestDistance = distance;
			}
		}
		commander.m_bSafeWithdrawalOrigin = best != null;
		if (best)
		{
			commander.m_sOriginLocationId = best.GetId();
			commander.m_vOrigin = best.GetCenter();
		}
	}

	protected bool TacticalEntitiesFar(AROR_OperationForce force)
	{
		array<IEntity> members = {};
		force.GetOwnedMembers(members);
		AROR_GroundOperationForce ground = AROR_GroundOperationForce.Cast(force);
		if (ground && ground.m_Vehicle)
			members.Insert(ground.m_Vehicle);
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int playerId : players)
		{
			IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (!player)
				continue;
			foreach (IEntity member : members)
			{
				if (member && vector.DistanceXZ(player.GetOrigin(), member.GetOrigin()) < m_fQRFGroundCleanupDistance)
					return false;
			}
		}
		return true;
	}
	protected void RetainTacticalForce(AROR_OperationForce force)
	{
		if (force.m_bTacticalRetained)
			return;
		force.m_bTacticalRetained = true;
		force.m_iTacticalRetainedAt = m_iExecutorClock;
		force.m_iTacticalNextOrder = m_iExecutorClock + 15;
		// Cancel the old charge/search order without deleting casualties or captured actors.
		if (force.m_Group && force.m_Group.GetLeaderEntity())
			ReplaceTacticalWaypoint(force, false, m_sQRFWaypointPrefab, force.m_Group.GetLeaderEntity().GetOrigin());
		AROR_GroundOperationForce ground = AROR_GroundOperationForce.Cast(force);
		if (ground && ground.m_CrewGroup && ground.m_Vehicle)
		{
			if (ground.m_TownSearch)
			{
				ground.m_TownSearch.Cleanup_S();
				ground.m_TownSearch = null;
			}
			ReplaceTacticalWaypoint(force, true, m_sQRFWaypointPrefab, ground.m_Vehicle.GetOrigin());
		}
		Print(string.Format("[AROR TACTICAL] %1 package=%2 retained with its budget; minimum age AND all-player distance required", force.m_sOperationId, force.m_iPackageIndex + 1), LogLevel.NORMAL);
	}
	protected void MonitorTacticalRetention(AROR_OperationForce force)
	{
		if (m_iExecutorClock < force.m_iTacticalNextOrder)
			return;
		force.m_iTacticalNextOrder = m_iExecutorClock + 15;
		AROR_GroundOperationForce ground = AROR_GroundOperationForce.Cast(force);
		if (ground && !ground.m_bDismounted && ground.m_Vehicle && ground.m_Profile.m_eType != AROR_EForceType.ARMOUR)
		{
			Physics physics = ground.m_Vehicle.GetPhysics();
			vector position = ground.m_Vehicle.GetOrigin();
			float height = position[1] - GetGame().GetWorld().GetSurfaceY(position[0], position[2]);
			if (physics && physics.GetVelocity().Length() < 2 && height < 5)
				ground.RequestDismount_S();
			if (ground.ArePassengersOut())
			{
				ground.m_bDismounted = true;
				AROR_Operation operation = m_Operations.GetOperation(force.m_sOperationId);
				if (operation && operation.m_TacticalCommander && operation.m_TacticalCommander.m_bSafeWithdrawalOrigin)
					ReplaceTacticalWaypoint(force, false, m_sMoveWaypointPrefab, operation.m_TacticalCommander.m_vOrigin);
			}
		}
		if (m_iExecutorClock - force.m_iTacticalRetainedAt < m_iQRFGroundRetention)
			return;
		if (TacticalEntitiesFar(force))
			ReleaseForce(force);
	}

	protected bool ReplaceTacticalWaypoint(AROR_OperationForce force, bool crew, ResourceName prefab, vector point)
	{
		SCR_AIGroup group = force.m_Group;
		AIWaypoint previous = force.m_Waypoint;
		AROR_GroundOperationForce ground = AROR_GroundOperationForce.Cast(force);
		if (crew && ground && ground.m_CrewGroup != force.m_Group)
		{
			group = ground.m_CrewGroup;
			previous = ground.m_CrewWaypoint;
		}
		if (!group || group.GetPlayerCount() > 0)
			return false;
		AIWaypoint next = SpawnWaypoint(prefab, point);
		if (!next)
			return false;
		next.SetCompletionRadius(20);
		group.AddWaypoint(next);
		if (previous)
		{
			group.RemoveWaypoint(previous);
			SCR_EntityHelper.DeleteEntityAndChildren(previous);
		}
		if (crew && ground && ground.m_CrewGroup != force.m_Group)
			ground.m_CrewWaypoint = next;
		else
			force.m_Waypoint = next;
		return true;
	}

	protected vector TacticalLaunchPoint(AROR_QRFTacticalCommander commander, int index)
	{
		vector heading = commander.m_vTown - commander.m_vOrigin;
		heading[1] = 0;
		if (heading.Length() < 1)
			heading = "0 0 1";
		else
			heading = heading.Normalized();
		vector right = Vector(heading[2], 0, -heading[0]);
		return commander.m_vOrigin + right * ((index - (commander.m_iPackages - 1) * 0.5) * m_fPackageSpacing);
	}
	protected bool TacticalRoadPoint(vector origin, vector desired, out vector reachable)
	{
		ChimeraAIWorld world = ChimeraAIWorld.Cast(GetGame().GetAIWorld());
		if (!world || !world.GetRoadNetworkManager())
			return false;
		return world.GetRoadNetworkManager().GetReachableWaypointInRoad(origin, desired, 75, reachable);
	}
	protected void RecordTacticalStrength(AROR_OperationForce force, AROR_Operation operation, int living)
	{
		if (!force.m_bTacticalControlled || !operation.m_TacticalCommander)
			return;
		if (force.m_iTacticalInitialStrength == 0)
			force.m_iTacticalInitialStrength = living;
		operation.m_TacticalCommander.RecordStrength(force.m_iPackageIndex, living);
	}

	protected bool TacticalWithdrawal(AROR_OperationForce force, AROR_Operation operation)
	{
		AROR_QRFTacticalCommander commander = operation.m_TacticalCommander;
		if (!commander)
			return false;
		if (m_iExecutorClock < force.m_iTacticalNextOrder && force.m_bTacticalWithdrawing)
			return true;
		bool first = !force.m_bTacticalWithdrawing;
		force.m_bTacticalWithdrawing = true;
		force.m_iTacticalNextOrder = m_iExecutorClock + m_iQRFOrderInterval;
		AROR_GroundOperationForce ground = AROR_GroundOperationForce.Cast(force);
		if (ground && ground.m_CrewGroup && ground.m_Vehicle && commander.m_bSafeWithdrawalOrigin)
		{
			vector home;
			if (TacticalRoadPoint(ground.m_Vehicle.GetOrigin(), commander.m_vOrigin, home))
				ReplaceTacticalWaypoint(force, true, m_sMoveWaypointPrefab, home);
			else
				ReplaceTacticalWaypoint(force, true, m_sQRFWaypointPrefab, ground.m_Vehicle.GetOrigin());
		}
		if ((!ground || ground.m_bDismounted) && commander.m_bSafeWithdrawalOrigin)
			ReplaceTacticalWaypoint(force, false, m_sMoveWaypointPrefab, commander.m_vOrigin);
		if (ground && ground.m_CrewGroup && ground.m_Vehicle && !commander.m_bSafeWithdrawalOrigin)
			ReplaceTacticalWaypoint(force, true, m_sQRFWaypointPrefab, ground.m_Vehicle.GetOrigin());
		if (!commander.m_bSafeWithdrawalOrigin && force.m_Group && force.m_Group.GetLeaderEntity())
			ReplaceTacticalWaypoint(force, false, m_sQRFWaypointPrefab, force.m_Group.GetLeaderEntity().GetOrigin());
		if (first)
			Print(string.Format("[AROR TACTICAL] %1 package=%2 withdrawing to origin; no remount teleport or nearby deletion", operation.GetId(), force.m_iPackageIndex + 1), LogLevel.WARNING);
		// Physical withdrawal releases actors only once outside every player's observation radius.
		if (TacticalEntitiesFar(force))
			FailPackage(force, operation, "Package withdrew beyond all players");
		return true;
	}

	protected void TacticalInfantryOrders(AROR_OperationForce force, AROR_Operation operation)
	{
		AROR_QRFTacticalCommander commander = operation.m_TacticalCommander;
		if (!commander || !force.m_Group || force.m_Group.GetPlayerCount() > 0)
			return;
		if (force.m_iTacticalContactVersion != commander.m_iContactVersion)
		{
			force.m_iTacticalContactVersion = commander.m_iContactVersion;
			force.m_iTacticalSearchStep = 0;
		}
		if (m_iExecutorClock < force.m_iTacticalNextOrder)
			return;
		IEntity leader = force.m_Group.GetLeaderEntity();
		if (force.m_iTacticalSearchStep > 0 && force.m_Waypoint && leader)
		{
			if (vector.DistanceXZ(leader.GetOrigin(), force.m_Waypoint.GetOrigin()) > 25 && m_iExecutorClock < force.m_iTacticalInfantryDeadline)
				return;
		}
		int step = force.m_iTacticalSearchStep;
		ResourceName prefab = m_sMoveWaypointPrefab;
		if (step >= 5)
			prefab = m_sQRFWaypointPrefab;
		int sectorStep = step;
		AROR_GroundOperationForce ground = AROR_GroundOperationForce.Cast(force);
		if (ground && ground.m_Profile.m_eType == AROR_EForceType.MECHANIZED && step == 0)
			sectorStep = 1;
		vector point = commander.InfantryPoint(force.m_iPackageIndex, sectorStep);
		force.m_iTacticalNextOrder = m_iExecutorClock + m_iQRFOrderInterval;
		if (!ReplaceTacticalWaypoint(force, false, prefab, point))
			return;
		force.m_iTacticalInfantryDeadline = m_iExecutorClock + 60;
		if (step < 5)
			force.m_iTacticalSearchStep++;
		else
			force.m_iTacticalNextOrder = m_iExecutorClock + 60;
		Print(string.Format("[AROR TACTICAL] %1 infantry package=%2 task=%3 sector point=%4 contactVersion=%5", operation.GetId(), force.m_iPackageIndex + 1, step, point, commander.m_iContactVersion), LogLevel.NORMAL);
	}

	//! Returns true when the commander owns this ground package's active orders.
	protected bool UpdateTacticalGround(AROR_GroundOperationForce force, AROR_Operation operation, int infantry, int crew)
	{
		AROR_QRFTacticalCommander commander = operation.m_TacticalCommander;
		if (!commander || !force.m_bTacticalControlled)
			return false;
		if (force.m_Group.GetPlayerCount() > 0)
			return true;
		if (force.m_CrewGroup && force.m_CrewGroup.GetPlayerCount() > 0)
			return true;
		int strength = infantry + crew;
		if (force.m_Profile.m_eType == AROR_EForceType.ARMOUR)
			strength = crew;
		RecordTacticalStrength(force, operation, strength);
		bool retreat = commander.m_bWithdraw || force.m_bTacticalWithdrawing;
		if (strength <= force.m_iTacticalInitialStrength * m_fQRFLocalWithdrawFraction)
			retreat = true;
		if (retreat)
			return TacticalWithdrawal(force, operation);
		if (force.m_Profile.m_eType != AROR_EForceType.ARMOUR && !force.m_bDismounted)
		{
			float distance = vector.DistanceXZ(force.m_Vehicle.GetOrigin(), force.m_vApproachPosition);
			Physics physics = force.m_Vehicle.GetPhysics();
			bool settled = physics && physics.GetVelocity().Length() < 3;
			if (distance <= 35 && settled && !force.m_bDismountRequested)
				force.RequestDismount_S();
			if (force.m_bDismountRequested && force.ArePassengersOut())
			{
				force.m_bDismounted = true;
				force.m_iTacticalNextOrder = 0;
				Print(string.Format("[AROR TACTICAL] %1 package=%2 delivered infantry; separate infantry search and vehicle support", operation.GetId(), force.m_iPackageIndex + 1), LogLevel.NORMAL);
			}
			else
			{
				if (m_iExecutorClock >= force.m_iTacticalLegDeadline)
					return TacticalWithdrawal(force, operation);
				return true;
			}
		}
		if (force.m_Profile.m_eType != AROR_EForceType.ARMOUR)
			TacticalInfantryOrders(force, operation);
		if (m_iExecutorClock < force.m_iTacticalNextSupport)
			return true;
		// Motorized transport waits at the drop point. Protected vehicles cover a road sector.
		if (force.m_Profile.m_eType == AROR_EForceType.MOTORIZED)
			return true;
		if (crew < 1 || !force.m_Vehicle)
			return true;
		DamageManagerComponent damage = DamageManagerComponent.Cast(force.m_Vehicle.FindComponent(DamageManagerComponent));
		if (damage && damage.IsDestroyed())
			return true;
		vector desired = commander.SupportPoint(force.m_iPackageIndex);
		vector support;
		if (TacticalRoadPoint(force.m_Vehicle.GetOrigin(), desired, support))
		{
			bool inside = vector.DistanceXZ(support, commander.m_vTown) <= commander.m_fTownRadius + commander.m_fMargin;
			if (inside && ReplaceTacticalWaypoint(force, true, m_sQRFWaypointPrefab, support))
				Print(string.Format("[AROR TACTICAL] %1 protected vehicle package=%2 covering separate road sector=%3", operation.GetId(), force.m_iPackageIndex + 1, support), LogLevel.NORMAL);
		}
		force.m_iTacticalNextSupport = m_iExecutorClock + 45;
		return true;
	}
}
