[ComponentEditorProps(category: "OpenResistance/Operations", description: "Server force-pool executor for infantry, motorized, mechanized and armoured operations. Airborne profiles await a flight adapter.")]
class AROR_OperationExecutorComponentClass : SCR_BaseGameModeComponentClass
{
}

class AROR_OperationExecutorComponent : SCR_BaseGameModeComponent
{
	[Attribute("0", desc: "Enable only after configuring infantry groups, waypoints and safe launch positions")]
	protected bool m_bEnabled;
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Occupier infantry group prefab; its faction and every member must match the commander", params: "et")]
	protected ResourceName m_sOccupiersGroupPrefab;
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Invader infantry group prefab; its faction and every member must match the commander", params: "et")]
	protected ResourceName m_sInvadersGroupPrefab;
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Attack waypoint prefab, e.g. a search-and-destroy waypoint", params: "et")]
	protected ResourceName m_sAttackWaypointPrefab;
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "QRF waypoint prefab, e.g. a defend waypoint", params: "et")]
	protected ResourceName m_sQRFWaypointPrefab;
	[Attribute("0 0 0", UIWidgets.Coords, desc: "Occupier infantry launch position; choose navigable land in the test world")]
	protected vector m_vOccupiersSpawnPosition;
	[Attribute("0 0 0", UIWidgets.Coords, desc: "Invader infantry launch position; choose navigable land in the test world")]
	protected vector m_vInvadersSpawnPosition;
	[Attribute("4", desc: "Maximum concurrent executor groups across both roles")]
	protected int m_iMaxGroups;
	[Attribute("32", desc: "Maximum reserved infantry prefab slots across pending and active groups")]
	protected int m_iMaxMembers;
	[Attribute("150", desc: "Minimum launch distance from the objective centre in metres")]
	protected float m_fMinTargetDistance;
	[Attribute("100", desc: "Minimum launch distance from player-controlled characters in metres")]
	protected float m_fMinPlayerDistance;
	[Attribute("50", desc: "Waypoint completion radius and arrival reporting distance in metres")]
	protected float m_fArrivalRadius;
	[Attribute("5", desc: "Seconds between wake requests if the engine evicts an active operation group")]
	protected int m_iWakeRetryInterval;
	[Attribute("", desc: "Optional force pool. Empty retains the existing infantry-only configuration.")]
	protected ref array<ref AROR_ForceProfile> m_aForceProfiles;
	[Attribute("4", desc: "Maximum reserved ground vehicles across both roles")]
	protected int m_iMaxVehicles;
	[Attribute("{000CD338713F2B5A}Prefabs/AI/Groups/Group_Base.et", UIWidgets.ResourceNamePicker, desc: "Empty base group used for separate vehicle crews", params: "et")]
	protected ResourceName m_sCrewGroupPrefab;
	[Attribute("{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et", UIWidgets.ResourceNamePicker, desc: "Vehicle approach waypoint", params: "et")]
	protected ResourceName m_sMoveWaypointPrefab;
	[Attribute("", desc: "Workbench only: force this profile ID for its matching role instead of weighted selection. Empty uses random selection.")]
	protected string m_sWorkbenchForceProfileId;
	[Attribute("", desc: "Response levels 1-5 with independent attack/QRF weights. Empty creates five equally weighted levels.")]
	protected ref array<ref AROR_ResponseLevel> m_aResponseLevels;
	[Attribute("0", desc: "Workbench only: force level 1-5; zero uses weighted random selection")]
	protected int m_iWorkbenchResponseLevel;
	[Attribute("25", desc: "Metres between package launch positions in a multi-package response")]
	protected float m_fPackageSpacing;
	[Attribute("1", desc: "Mounted QRFs sweep reachable town road points before returning to their deployment orders. Does not track player positions.")]
	protected bool m_bQRFVehicleSearch;
	[Attribute("180", desc: "Maximum seconds for a mounted town search", params: "1 900 1")]
	protected int m_iQRFSearchDuration;
	[Attribute("6", desc: "Maximum road stops per mounted QRF package", params: "1 12 1")]
	protected int m_iQRFSearchStops;
	[Attribute("8", desc: "Seconds to pause at each reached search stop", params: "0 60 1")]
	protected int m_iQRFSearchDwell;
	[Attribute("60", desc: "Seconds before skipping a stalled search leg", params: "10 300 1")]
	protected int m_iQRFSearchLegTimeout;
	[Attribute("0", desc: "Temporary diagnostic: report the first same-faction damage event on each operation-owned character/vehicle. Does not change combat behavior.")]
	protected bool m_bLogFriendlyDamage;

	protected AROR_OperationManagerComponent m_Operations;
	protected int m_iExecutorClock;
	protected ref array<ref AROR_OperationForce> m_aForces = {};
	protected ref map<string, string> m_mWaitReasons = new map<string, string>();

	//! Log changed blockers only; requests are retried each second without flooding the console.
	protected void ReportSpawnWait(AROR_Operation operation, string reason)
	{
		string previous;
		if (m_mWaitReasons.Find(operation.GetId(), previous) && previous == reason)
			return;
		m_mWaitReasons.Set(operation.GetId(), reason);
		Print(string.Format("[AROR] Executor waiting for %1: %2", operation.GetId(), reason), LogLevel.WARNING);
	}

	protected AROR_OperationForce FindForce(string operationId)
	{
		foreach (AROR_OperationForce force : m_aForces)
		{
			if (force.m_sOperationId == operationId)
				return force;
		}
		return null;
	}

	protected void ReleaseForce(AROR_OperationForce force)
	{
		if (!force)
			return;
		force.Cleanup_S();
		m_aForces.RemoveItem(force);
	}

	protected AROR_OperationForce FindPackage(string operationId, int packageIndex)
	{
		foreach (AROR_OperationForce force : m_aForces)
		{
			if (!force.m_bReservation && force.m_sOperationId == operationId && force.m_iPackageIndex == packageIndex)
				return force;
		}
		return null;
	}

	protected void ReleaseOperationForces(string operationId)
	{
		array<ref AROR_OperationForce> snapshot = {};
		foreach (AROR_OperationForce force : m_aForces)
		{
			if (force.m_sOperationId == operationId)
				snapshot.Insert(force);
		}
		foreach (AROR_OperationForce packageForce : snapshot)
			ReleaseForce(packageForce);
		if (snapshot.Count() > 0)
			Print(string.Format("[AROR] Operation %1 cleanup completed for %2 force records; player preservation rules apply", operationId, snapshot.Count()), LogLevel.NORMAL);
	}

	protected void ReportPackageProgress(AROR_OperationForce force, string progress)
	{
		if (force.m_sLastProgress == progress)
			return;
		force.m_sLastProgress = progress;
		Print(string.Format("[AROR] QRF/attack %1 package=%2: %3", force.m_sOperationId, force.m_iPackageIndex + 1, progress), LogLevel.NORMAL);
	}

	protected void TryActivateOperation(AROR_Operation operation)
	{
		if (!operation || operation.GetState() != AROR_EOperationState.SPAWNING)
			return;
		int packages;
		foreach (AROR_OperationForce force : m_aForces)
		{
			if (force.m_sOperationId != operation.GetId())
				continue;
			if (force.m_bReservation || !force.m_bReady)
				return;
			packages++;
		}
		if (packages == 0)
			return;
		foreach (AROR_OperationForce ready : m_aForces)
		{
			if (ready.m_sOperationId != operation.GetId() || ready.m_bOrdersIssued)
				continue;
			AROR_GroundOperationForce ground = AROR_GroundOperationForce.Cast(ready);
			if (ground)
			{
				if (ground.m_Profile.m_eType == AROR_EForceType.ARMOUR)
					ground.m_CrewGroup.AddWaypoint(ground.m_Waypoint);
				else
					ground.m_CrewGroup.AddWaypoint(ground.m_CrewWaypoint);
			}
			else
				ready.m_Group.AddWaypoint(ready.m_Waypoint);
			ready.m_bOrdersIssued = true;
		}
		m_Operations.MarkActive_S(operation.GetId());
	}

	//! One lost element reduces a multi-package response; the final lost element fails it.
	protected void FailPackage(AROR_OperationForce force, AROR_Operation operation, string reason)
	{
		Print(string.Format("[AROR] Operation %1 lost package %2: %3", operation.GetId(), force.m_iPackageIndex + 1, reason), LogLevel.NORMAL);
		ReleaseForce(force);
		foreach (AROR_OperationForce survivor : m_aForces)
		{
			if (survivor.m_sOperationId == operation.GetId() && !survivor.m_bReservation)
			{
				TryActivateOperation(operation);
				return;
			}
		}
		FailOperation(operation, reason);
	}

	protected void FailOperation(AROR_Operation operation, string reason)
	{
		m_Operations.ResolveOperation_S(operation.GetId(), AROR_EOperationState.FAILED, reason);
	}

	protected bool IsLaunchPositionSafe(vector position, vector target)
	{
		if (GetGame().GetWorld().GetSurfaceY(position[0], position[2]) <= 0 || vector.DistanceXZ(position, target) < m_fMinTargetDistance)
			return false;
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (player && vector.DistanceXZ(position, player.GetOrigin()) < m_fMinPlayerDistance)
				return false;
		}
		return true;
	}

	protected IEntity SpawnResource(Resource resource, vector position)
	{
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]);
		spawnParams.Transform[3] = position;
		return GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
	}

	protected int GetReservedGroups()
	{
		int count;
		foreach (AROR_OperationForce force : m_aForces)
			count += force.m_iReservedGroups;
		return count;
	}

	protected int GetReservedMembers()
	{
		int count;
		foreach (AROR_OperationForce force : m_aForces)
			count += force.m_iReservedMembers;
		return count;
	}

	protected int GetReservedVehicles()
	{
		int count;
		foreach (AROR_OperationForce force : m_aForces)
			count += force.m_iReservedVehicles;
		return count;
	}

	protected void ConvertReservation(AROR_OperationForce reservation, AROR_ForceProfile profile)
	{
		if (!reservation || !profile)
			return;
		reservation.m_iReservedGroups -= profile.GetGroupBudget();
		reservation.m_iReservedMembers -= profile.m_iPersonnelBudget;
		if (profile.m_eType != AROR_EForceType.INFANTRY)
			reservation.m_iReservedVehicles--;
	}

	protected AROR_ForceProfile SelectProfile(AROR_Operation operation, AROR_Commander commander)
	{
		if (!operation.GetForceProfileId().IsEmpty())
		{
			foreach (AROR_ForceProfile bound : m_aForceProfiles)
			{
				if (bound && bound.m_sId == operation.GetForceProfileId() && AROR_ForcePool.GetEligibleWeight(bound, commander.GetRole(), commander.GetFactionKey(), operation.GetType(), m_iMaxGroups - GetReservedGroups(), m_iMaxMembers - GetReservedMembers(), m_iMaxVehicles - GetReservedVehicles(), operation.GetResponseLevel()) > 0)
					return bound;
			}
			return null;
		}
		array<ref AROR_ForceProfile> candidates = {};
#ifdef WORKBENCH
		if (!m_sWorkbenchForceProfileId.IsEmpty())
		{
			foreach (AROR_ForceProfile forced : m_aForceProfiles)
			{
				if (forced && forced.m_sId == m_sWorkbenchForceProfileId && forced.m_eRole == commander.GetRole())
				{
					candidates.Insert(forced);
					break;
				}
			}
		}
#endif
		if (candidates.Count() == 0)
		{
			foreach (AROR_ForceProfile candidate : m_aForceProfiles)
				candidates.Insert(candidate);
		}
		int freeGroups = m_iMaxGroups - GetReservedGroups();
		int freeMembers = m_iMaxMembers - GetReservedMembers();
		int freeVehicles = m_iMaxVehicles - GetReservedVehicles();
		int level;
		int levelTotal;
		foreach (AROR_ResponseLevel response : m_aResponseLevels)
			levelTotal += AROR_ResponseScale.GetEligibleWeight(response, candidates, commander.GetRole(), commander.GetFactionKey(), operation.GetType(), freeGroups, freeMembers, freeVehicles);
		if (levelTotal <= 0)
		{
			ReportSpawnWait(operation, string.Format("No eligible response level: role=%1 faction=%2 candidates=%3 free groups=%4 members=%5 vehicles=%6; check enabled profiles, role/faction, weights and budgets", commander.GetRole(), commander.GetFactionKey(), candidates.Count(), freeGroups, freeMembers, freeVehicles));
			return null;
		}
		level = AROR_ResponseScale.PickLevel(m_aResponseLevels, candidates, commander.GetRole(), commander.GetFactionKey(), operation.GetType(), freeGroups, freeMembers, freeVehicles, Math.RandomInt(0, levelTotal));
#ifdef WORKBENCH
		if (m_iWorkbenchResponseLevel > 0)
		{
			level = 0;
			foreach (AROR_ResponseLevel forcedLevel : m_aResponseLevels)
			{
				if (forcedLevel.m_iLevel == m_iWorkbenchResponseLevel && AROR_ResponseScale.GetEligibleWeight(forcedLevel, candidates, commander.GetRole(), commander.GetFactionKey(), operation.GetType(), freeGroups, freeMembers, freeVehicles) > 0)
					level = forcedLevel.m_iLevel;
			}
		}
#endif
		if (level < 1)
		{
			ReportSpawnWait(operation, "Selected/Workbench-forced response level is not eligible for this role and available capacity");
			return null;
		}
		int total;
		foreach (AROR_ForceProfile profile : candidates)
			total += AROR_ForcePool.GetEligibleWeight(profile, commander.GetRole(), commander.GetFactionKey(), operation.GetType(), freeGroups, freeMembers, freeVehicles, level);
		if (total <= 0)
			return null;
		AROR_ForceProfile selected = AROR_ForcePool.Pick(candidates, commander.GetRole(), commander.GetFactionKey(), operation.GetType(), freeGroups, freeMembers, freeVehicles, Math.RandomInt(0, total), level);
		if (!selected || !operation.BindForceProfile_S(selected.m_sId, level))
			return null;
		Print(string.Format("[AROR] Operation %1 selected force profile %2 (type=%3), response level=%4, packages=%4", operation.GetId(), selected.m_sId, selected.m_eType, level), LogLevel.NORMAL);
		return selected;
	}

	protected void StartForce(AROR_Operation operation)
	{
		if (!operation || operation.GetState() != AROR_EOperationState.SPAWNING || FindForce(operation.GetId()))
			return;
		if (!GetGame().GetAIWorld())
		{
			FailOperation(operation, "No AIWorld in this scenario; add the terrain's AIWorld prefab before deploying AI");
			return;
		}
		if (m_aForceProfiles.Count() == 0)
		{
			StartInfantryForce(operation);
			return;
		}
		AROR_CommanderManagerComponent commanders = AROR_CommanderManagerComponent.GetInstance();
		if (!commanders)
		{
			ReportSpawnWait(operation, "Commander manager unavailable");
			return;
		}
		AROR_Commander commander = commanders.GetCommander(operation.GetRole());
		if (!commander || commander.IsPlayerDirected())
			return;
		AROR_ForceProfile profile = SelectProfile(operation, commander);
		if (!profile)
			return;
		int packages = operation.GetResponseLevel();
		AROR_OperationForce reservation = new AROR_OperationForce();
		reservation.m_sOperationId = operation.GetId();
		reservation.m_bReservation = true;
		reservation.m_iPackageIndex = -1;
		reservation.m_iReservedGroups = profile.GetGroupBudget() * packages;
		reservation.m_iReservedMembers = profile.m_iPersonnelBudget * packages;
		if (profile.m_eType != AROR_EForceType.INFANTRY)
			reservation.m_iReservedVehicles = packages;
		m_aForces.Insert(reservation);
		for (int packageIndex = 0; packageIndex < packages; packageIndex++)
		{
			if (operation.IsTerminal())
				break;
			if (profile.m_eType == AROR_EForceType.INFANTRY)
				StartInfantryForce(operation, profile, packageIndex, reservation);
			else
				StartGroundForce(operation, commander, profile, packageIndex, reservation);
		}
		m_aForces.RemoveItem(reservation);
		TryActivateOperation(operation);
	}

	protected SCR_AIGroup SpawnEmptyGroup(ResourceName prefab, vector position)
	{
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return null;
		SCR_AIGroup.IgnoreSpawning(true);
		IEntity entity = SpawnResource(resource, position);
		SCR_AIGroup.IgnoreSpawning(false);
		SCR_AIGroup group = SCR_AIGroup.Cast(entity);
		if (!group && entity)
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
		return group;
	}

	protected AIWaypoint SpawnWaypoint(ResourceName prefab, vector position)
	{
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return null;
		IEntity entity = SpawnResource(resource, position);
		AIWaypoint waypoint = AIWaypoint.Cast(entity);
		if (!waypoint && entity)
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
		return waypoint;
	}

	protected void ConfigureGroup(SCR_AIGroup group, AROR_Operation operation)
	{
		group.SetCanDeleteIfNoPlayer(false);
		group.SetPrivate(true);
		group.SetPrivacyChangeable(false);
		group.SetLifecyclePolicy(SCR_EAIGroupLifecyclePolicy.Manual);
		if (operation.GetType() == AROR_EOperationType.QRF)
			group.SetImportance(SCR_EAISpawnImportance.HIGH);
		else
			group.SetImportance(SCR_EAISpawnImportance.NORMAL);
	}

	protected void StartGroundForce(AROR_Operation operation, AROR_Commander commander, AROR_ForceProfile profile, int packageIndex, AROR_OperationForce reservation)
	{
		AROR_LocationManagerComponent locations = AROR_LocationManagerComponent.GetInstance();
		AROR_LocationComponent target;
		if (locations)
			target = locations.GetLocationById(operation.GetTargetLocationId());
		vector launchPosition = m_vOccupiersSpawnPosition;
		if (operation.GetRole() == AROR_EFactionRole.INVADERS)
			launchPosition = m_vInvadersSpawnPosition;
		launchPosition[0] = launchPosition[0] + packageIndex * m_fPackageSpacing;
		if (!target || !IsLaunchPositionSafe(launchPosition, target.GetCenter()))
		{
			FailOperation(operation, "Ground-force target or launch position invalid");
			return;
		}
		// Reserve the whole package before queuing any member; failures release through terminal events.
		AROR_GroundOperationForce force = new AROR_GroundOperationForce();
		force.m_sOperationId = operation.GetId();
		force.m_iPackageIndex = packageIndex;
		force.m_sFactionKey = commander.GetFactionKey();
		force.m_Profile = profile;
		force.m_bLogFriendlyDamage = m_bLogFriendlyDamage;
		force.m_iReservedGroups = profile.GetGroupBudget();
		force.m_iReservedMembers = profile.m_iPersonnelBudget;
		force.m_iReservedVehicles = 1;
		ConvertReservation(reservation, profile);
		m_aForces.Insert(force);
		Resource vehicleResource = Resource.Load(profile.m_sVehicle);
		if (vehicleResource && vehicleResource.IsValid())
			force.m_Vehicle = SpawnResource(vehicleResource, launchPosition);
		if (!force.m_Vehicle)
		{
			FailOperation(operation, "Ground-force vehicle resource failed");
			return;
		}
		FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(force.m_Vehicle.FindComponent(FactionAffiliationComponent));
		// Fresh standalone vehicles may have no runtime affiliation despite their prefab faction.
		// Assign only an empty affiliation; an explicitly conflicting faction remains an error.
		if (affiliation && affiliation.GetAffiliatedFactionKey().IsEmpty())
			affiliation.SetAffiliatedFactionByKey(force.m_sFactionKey);
		if (!affiliation || affiliation.GetAffiliatedFactionKey() != force.m_sFactionKey)
		{
			FailOperation(operation, "Ground-force vehicle faction mismatch");
			return;
		}
		force.TrackDamage_S(force.m_Vehicle);
		vector crewPosition = launchPosition;
		crewPosition[0] = crewPosition[0] + 5;
		force.m_CrewGroup = SpawnEmptyGroup(m_sCrewGroupPrefab, crewPosition);
		if (!force.m_CrewGroup)
		{
			FailOperation(operation, "Ground-force crew group could not be created");
			return;
		}
		force.m_CrewGroup.GetOnAgentAdded().Insert(force.OnAgentAdded);
		force.m_CrewGroup.InitFactionKey(force.m_sFactionKey);
		if (!force.m_CrewGroup.m_aUnitPrefabSlots)
			force.m_CrewGroup.m_aUnitPrefabSlots = {};
		force.m_CrewGroup.m_aUnitPrefabSlots.Clear();
		for (int c = 0; c < profile.m_iCrewCount; c++)
			force.m_CrewGroup.m_aUnitPrefabSlots.Insert(profile.m_sCrewCharacter);
		if (profile.m_eType == AROR_EForceType.ARMOUR)
			force.m_Group = force.m_CrewGroup;
		else
		{
			vector infantryPosition = launchPosition;
			infantryPosition[0] = infantryPosition[0] - 8;
			force.m_Group = SpawnEmptyGroup(profile.m_sInfantryGroup, infantryPosition);
			if (force.m_Group)
				force.m_Group.GetOnAgentAdded().Insert(force.OnAgentAdded);
		}
		if (!force.m_Group || !force.m_Group.m_aUnitPrefabSlots || force.m_Group.m_aUnitPrefabSlots.Count() < 1)
		{
			FailOperation(operation, "Ground-force infantry group is invalid");
			return;
		}
		force.m_Group.InitFactionKey(force.m_sFactionKey);
		int requested = force.m_Group.m_aUnitPrefabSlots.Count();
		int totalRequested = requested;
		if (force.m_Group != force.m_CrewGroup)
			totalRequested += profile.m_iCrewCount;
		if (totalRequested > profile.m_iPersonnelBudget)
		{
			FailOperation(operation, "Ground-force members exceed profile reservation");
			return;
		}
		ResourceName objectiveWaypoint = m_sAttackWaypointPrefab;
		if (operation.GetType() == AROR_EOperationType.QRF)
			objectiveWaypoint = m_sQRFWaypointPrefab;
		force.m_Waypoint = SpawnWaypoint(objectiveWaypoint, target.GetCenter());
		vector approach = target.GetCenter();
		if (profile.m_eType != AROR_EForceType.ARMOUR)
		{
			float distance = vector.DistanceXZ(launchPosition, target.GetCenter());
			if (distance <= profile.m_fDismountDistance + 20)
			{
				FailOperation(operation, "Launch position is too close for profile dismount distance");
				return;
			}
			approach[0] = approach[0] + (launchPosition[0] - approach[0]) / distance * profile.m_fDismountDistance;
			approach[2] = approach[2] + (launchPosition[2] - approach[2]) / distance * profile.m_fDismountDistance;
			force.m_CrewWaypoint = SpawnWaypoint(m_sMoveWaypointPrefab, approach);
		}
		force.m_vApproachPosition = approach;
		if (!force.m_Waypoint || (profile.m_eType != AROR_EForceType.ARMOUR && !force.m_CrewWaypoint))
		{
			FailOperation(operation, "Ground-force waypoint resource failed");
			return;
		}
		force.m_Waypoint.SetCompletionRadius(m_fArrivalRadius);
		if (force.m_CrewWaypoint)
			force.m_CrewWaypoint.SetCompletionRadius(10);
		ConfigureGroup(force.m_CrewGroup, operation);
		if (force.m_Group != force.m_CrewGroup)
		{
			ConfigureGroup(force.m_Group, operation);
			force.m_Group.RequestSpawn(requested);
		}
		force.m_CrewGroup.RequestSpawn(profile.m_iCrewCount);
		Print(string.Format("[AROR] Ground package queued: %1 profile=%2 personnel=%3", operation.GetId(), profile.m_sId, totalRequested), LogLevel.NORMAL);
	}

	protected void StartInfantryForce(AROR_Operation operation, AROR_ForceProfile profile = null, int packageIndex = 0, AROR_OperationForce reservation = null)
	{
		if (FindPackage(operation.GetId(), packageIndex))
			return;
		// A busy executor leaves requests pending; the manager's spawn deadline still applies.
		if (!profile && GetReservedGroups() >= m_iMaxGroups)
		{
			ReportSpawnWait(operation, "Legacy infantry group budget is full");
			return;
		}
		AROR_CommanderManagerComponent commanders = AROR_CommanderManagerComponent.GetInstance();
		AROR_LocationManagerComponent locations = AROR_LocationManagerComponent.GetInstance();
		if (!commanders || !locations)
			return;
		AROR_Commander commander = commanders.GetCommander(operation.GetRole());
		AROR_LocationComponent target = locations.GetLocationById(operation.GetTargetLocationId());
		if (!commander || commander.IsPlayerDirected() || !target)
		{
			FailOperation(operation, "Executor has no enemy commander or target");
			return;
		}

		ResourceName groupPrefab = m_sOccupiersGroupPrefab;
		vector launchPosition = m_vOccupiersSpawnPosition;
		if (operation.GetRole() == AROR_EFactionRole.INVADERS)
		{
			groupPrefab = m_sInvadersGroupPrefab;
			launchPosition = m_vInvadersSpawnPosition;
		}
		if (profile)
			groupPrefab = profile.m_sInfantryGroup;
		launchPosition[0] = launchPosition[0] + packageIndex * m_fPackageSpacing;
		ResourceName waypointPrefab = m_sAttackWaypointPrefab;
		if (operation.GetType() == AROR_EOperationType.QRF)
			waypointPrefab = m_sQRFWaypointPrefab;
		if (groupPrefab.IsEmpty() || waypointPrefab.IsEmpty() || !IsLaunchPositionSafe(launchPosition, target.GetCenter()))
		{
			FailOperation(operation, "Executor group/waypoint missing or launch position unsafe");
			return;
		}
		Resource groupResource = Resource.Load(groupPrefab);
		Resource waypointResource = Resource.Load(waypointPrefab);
		if (!groupResource || !groupResource.IsValid() || !waypointResource || !waypointResource.IsValid())
		{
			FailOperation(operation, "Executor resource could not be loaded");
			return;
		}

		// Create a cheap empty group; all members must go through the engine's shared queue.
		SCR_AIGroup.IgnoreSpawning(true);
		IEntity groupEntity = SpawnResource(groupResource, launchPosition);
		SCR_AIGroup.IgnoreSpawning(false);
		SCR_AIGroup group = SCR_AIGroup.Cast(groupEntity);
		if (!group)
		{
			if (groupEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(groupEntity);
			FailOperation(operation, "Configured infantry prefab is not an SCR_AIGroup");
			return;
		}

		AROR_OperationForce force = new AROR_OperationForce();
		force.m_sOperationId = operation.GetId();
		force.m_iPackageIndex = packageIndex;
		force.m_sFactionKey = commander.GetFactionKey();
		force.m_Group = group;
		force.m_bLogFriendlyDamage = m_bLogFriendlyDamage;
		group.InitFactionKey(force.m_sFactionKey);
		group.GetOnAgentAdded().Insert(force.OnAgentAdded);
		if (group.m_aUnitPrefabSlots)
			force.m_iReservedMembers = group.m_aUnitPrefabSlots.Count();
		int requestedMembers = force.m_iReservedMembers;
		if (profile && requestedMembers <= profile.m_iPersonnelBudget)
			force.m_iReservedMembers = profile.m_iPersonnelBudget;
		ConvertReservation(reservation, profile);
		m_aForces.Insert(force);
		int reserved = GetReservedMembers();
		if (requestedMembers < 1 || reserved > m_iMaxMembers || (profile && requestedMembers > profile.m_iPersonnelBudget))
		{
			FailOperation(operation, "Infantry group has no member slots or exceeds executor member budget");
			return;
		}
		if (group.GetFaction() && group.GetFaction().GetFactionKey() != force.m_sFactionKey)
		{
			FailOperation(operation, "Group prefab faction does not match commander role");
			return;
		}

		IEntity waypointEntity = SpawnResource(waypointResource, target.GetCenter());
		force.m_Waypoint = AIWaypoint.Cast(waypointEntity);
		if (!force.m_Waypoint)
		{
			if (waypointEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(waypointEntity);
			FailOperation(operation, "Configured waypoint prefab is not an AIWaypoint");
			return;
		}
		force.m_Waypoint.SetCompletionRadius(m_fArrivalRadius);
		group.SetPrivate(true);
		group.SetPrivacyChangeable(false);
		group.SetLifecyclePolicy(SCR_EAIGroupLifecyclePolicy.Manual);
		if (operation.GetType() == AROR_EOperationType.QRF)
			group.SetImportance(SCR_EAISpawnImportance.HIGH);
		else
			group.SetImportance(SCR_EAISpawnImportance.NORMAL);
		group.RequestSpawn(requestedMembers);
		Print(string.Format("[AROR] Force queued for %1: faction=%2, slots=%3", operation.GetId(), force.m_sFactionKey, force.m_iReservedMembers), LogLevel.NORMAL);
	}

	protected void OnOperationChanged(AROR_Operation operation)
	{
		if (!Replication.IsServer() || !operation)
			return;
		if (operation.IsTerminal())
		{
			m_mWaitReasons.Remove(operation.GetId());
			ReleaseOperationForces(operation.GetId());
		}
		else if (operation.GetState() == AROR_EOperationState.SPAWNING)
			StartForce(operation);
	}

	protected void MonitorForce(AROR_OperationForce force)
	{
		if (!force || force.m_bReservation)
			return;
		AROR_GroundOperationForce ground = AROR_GroundOperationForce.Cast(force);
		if (ground)
			MonitorGroundForce(ground);
		else
			MonitorInfantryForce(force);
	}

	protected void MonitorGroundForce(AROR_GroundOperationForce force)
	{
		AROR_Operation operation = m_Operations.GetOperation(force.m_sOperationId);
		if (!operation || operation.IsTerminal())
		{
			ReleaseForce(force);
			return;
		}
		if (!force.m_Group || (!force.m_CrewGroup && !force.m_bDismounted))
		{
			FailPackage(force, operation, "Ground-force group deleted before delivery");
			return;
		}
		int infantry = force.CountInfantry();
		int crew = force.CountCrew();
		if (!force.HasCorrectMemberFactions())
		{
			FailOperation(operation, "Ground-force personnel faction mismatch");
			return;
		}
		bool vehicleLost = !force.m_Vehicle;
		if (force.m_Vehicle)
		{
			DamageManagerComponent damage = DamageManagerComponent.Cast(force.m_Vehicle.FindComponent(DamageManagerComponent));
			if (damage && damage.IsDestroyed())
				vehicleLost = true;
		}
		if (vehicleLost && !force.m_bDismounted)
		{
			FailPackage(force, operation, "Operation vehicle destroyed before delivery or during armoured attack");
			return;
		}
		if (!vehicleLost)
		{
			FactionAffiliationComponent vehicleFaction = FactionAffiliationComponent.Cast(force.m_Vehicle.FindComponent(FactionAffiliationComponent));
			if (!vehicleFaction || vehicleFaction.GetAffiliatedFactionKey() != force.m_sFactionKey)
			{
				FailOperation(operation, "Operation vehicle faction changed or became unassigned");
				return;
			}
		}
		// Retain reservations and wake engine-dormant groups rather than declaring combat losses.
		bool dormant;
		force.m_iWakeRetryTicks--;
		array<SCR_AIGroup> groups = {force.m_Group};
		if (force.m_CrewGroup && force.m_CrewGroup != force.m_Group && !force.m_bDismounted)
			groups.Insert(force.m_CrewGroup);
		foreach (SCR_AIGroup group : groups)
		{
			if (!group.IsDormant())
				continue;
			dormant = true;
			bool sawGroupMembers = force.m_bSawInfantry;
			if (group == force.m_CrewGroup)
				sawGroupMembers = force.m_bSawCrew;
			// A newly queued group has no members yet. Zero is a defeat only after
			// this particular element has actually materialized, not another package's group.
			if (sawGroupMembers && group.GetDormantAliveCount() == 0)
			{
				FailPackage(force, operation, "Ground-force element eliminated");
				return;
			}
			if (force.m_iWakeRetryTicks <= 0)
				group.RequestSpawn();
		}
		if (dormant)
		{
			ReportPackageProgress(force, "Waiting for engine-dormant groups to materialize");
			if (force.m_iWakeRetryTicks <= 0)
				force.m_iWakeRetryTicks = m_iWakeRetryInterval;
			return;
		}
		if (!force.m_bMounted)
		{
			if (crew < 1 || infantry < 1 || !force.m_Group.IsExpandComplete() || !force.m_CrewGroup.IsExpandComplete())
			{
				ReportPackageProgress(force, string.Format("Spawning members: crew=%1 infantry=%2 crew complete=%3 infantry complete=%4", crew, infantry, force.m_CrewGroup.IsExpandComplete(), force.m_Group.IsExpandComplete()));
				return;
			}
			if (!force.m_Group.GetFaction() || !force.m_CrewGroup.GetFaction() || force.m_Group.GetFaction().GetFactionKey() != force.m_sFactionKey || force.m_CrewGroup.GetFaction().GetFactionKey() != force.m_sFactionKey)
			{
				FailOperation(operation, "Ground-force group faction mismatch");
				return;
			}
			if (!force.Mount_S())
			{
				FailOperation(operation, "Vehicle is not AI-drivable or crew/passenger seating failed");
				return;
			}
			force.m_bReady = true;
			FactionAffiliationComponent deployedFaction = FactionAffiliationComponent.Cast(force.m_Vehicle.FindComponent(FactionAffiliationComponent));
			Print(string.Format("[AROR] Deployment faction verified: operation=%1 package=%2 expected=%3 vehicle=%4 crewGroup=%5 infantryGroup=%6 members=%7", operation.GetId(), force.m_iPackageIndex + 1, force.m_sFactionKey, deployedFaction.GetAffiliatedFactionKey(), force.m_CrewGroup.GetFaction().GetFactionKey(), force.m_Group.GetFaction().GetFactionKey(), force.CountLivingMembers()), LogLevel.NORMAL);
			ReportPackageProgress(force, "Crew and infantry boarded; waiting for all packages before issuing vehicle orders");
			TryActivateOperation(operation);
			return;
		}
		if (infantry == 0 || (crew == 0 && !force.m_bDismounted))
		{
			FailPackage(force, operation, "Ground-force required infantry or crew eliminated");
			return;
		}
		if (operation.GetState() != AROR_EOperationState.ACTIVE)
			return;
		if (UpdateTownSearch(force, operation))
			return;
		if (force.m_Profile.m_eType != AROR_EForceType.ARMOUR && !force.m_bDismounted)
		{
			if (!force.m_bDismountRequested && !force.m_CrewGroup.GetCurrentWaypoint() && vector.DistanceXZ(force.m_Vehicle.GetOrigin(), force.m_vApproachPosition) <= 30)
			{
				if (force.RequestDismount_S())
					ReportPackageProgress(force, "Vehicle reached dismount point; passengers exiting");
				else
					ReportPackageProgress(force, "At dismount point; waiting for usable passenger exits");
			}
			else if (!force.m_bDismountRequested)
				ReportPackageProgress(force, "Vehicle orders active; travelling to dismount point");
			if (force.m_bDismountRequested && force.ArePassengersOut())
			{
				force.m_bDismounted = true;
				force.m_Group.AddWaypoint(force.m_Waypoint);
				if (force.m_Profile.m_eType == AROR_EForceType.MECHANIZED)
				{
					if (force.m_CrewWaypoint)
						SCR_EntityHelper.DeleteEntityAndChildren(force.m_CrewWaypoint);
					ResourceName supportWaypoint = m_sAttackWaypointPrefab;
					if (operation.GetType() == AROR_EOperationType.QRF)
						supportWaypoint = m_sQRFWaypointPrefab;
					force.m_CrewWaypoint = SpawnWaypoint(supportWaypoint, force.m_Waypoint.GetOrigin());
					if (!force.m_CrewWaypoint)
					{
						FailOperation(operation, "Mechanized support waypoint could not be created");
						return;
					}
					force.m_CrewGroup.AddWaypoint(force.m_CrewWaypoint);
				}
				Print(string.Format("[AROR] Operation %1 infantry dismounted; objective orders issued", operation.GetId()), LogLevel.NORMAL);
				ReportPackageProgress(force, "Passengers clear of vehicle; infantry moving to defended location");
			}
		}
		AROR_LocationManagerComponent locations = AROR_LocationManagerComponent.GetInstance();
		AROR_LocationComponent target;
		if (locations)
			target = locations.GetLocationById(operation.GetTargetLocationId());
		IEntity leader = force.m_Group.GetLeaderEntity();
		if (!force.m_bArrivalLogged && target && leader && vector.DistanceXZ(leader.GetOrigin(), target.GetCenter()) <= m_fArrivalRadius)
		{
			force.m_bArrivalLogged = true;
			Print(string.Format("[AROR] Force %1 reached objective; awaiting territory/combat outcome", operation.GetId()), LogLevel.NORMAL);
		}
	}

	//! Owns only QRF search orders; attacks and the post-search deployment policy keep their orders.
	protected bool UpdateTownSearch(AROR_GroundOperationForce force, AROR_Operation operation)
	{
		if (!m_bQRFVehicleSearch || operation.GetType() != AROR_EOperationType.QRF || force.m_bDismounted || force.m_bTownSearchFinished)
			return false;
		if (!force.m_bTownSearchAttempted)
		{
			AROR_LocationManagerComponent locations = AROR_LocationManagerComponent.GetInstance();
			if (!locations)
				return false;
			AROR_LocationComponent location = locations.GetLocationById(operation.GetTargetLocationId());
			if (!location || !force.m_Vehicle)
				return false;
			vector entry = force.m_vApproachPosition;
			float entryRadius = 30;
			AIWaypoint previousWaypoint = force.m_CrewWaypoint;
			if (force.m_Profile.m_eType == AROR_EForceType.ARMOUR)
			{
				entry = location.GetCenter();
				entryRadius = m_fArrivalRadius + 10;
				previousWaypoint = force.m_Waypoint;
			}
			if (vector.DistanceXZ(force.m_Vehicle.GetOrigin(), entry) > entryRadius)
				return false;
			force.m_bTownSearchAttempted = true;
			force.m_TownSearch = new AROR_VehicleTownSearch();
			if (!force.m_TownSearch.Start_S(force.m_CrewGroup, force.m_Vehicle, location, previousWaypoint, m_sMoveWaypointPrefab, operation.GetId(), m_iExecutorClock, m_iQRFSearchDuration, m_iQRFSearchStops, m_iQRFSearchDwell, m_iQRFSearchLegTimeout))
			{
				force.m_TownSearch.Cleanup_S();
				force.m_TownSearch = null;
				force.m_bTownSearchFinished = true;
				Print(string.Format("[AROR] Mounted search skipped for %1: no reachable road stop or move waypoint; keeping deployment orders", operation.GetId()), LogLevel.WARNING);
				return false;
			}
			ReportPackageProgress(force, "Searching town roads while mounted; native AI handles spotting and combat");
			return true;
		}
		if (force.m_TownSearch && force.m_TownSearch.Tick_S(m_iExecutorClock))
			return true;
		force.m_bTownSearchFinished = true;
		if (force.m_Profile.m_eType == AROR_EForceType.ARMOUR)
			force.m_CrewGroup.AddWaypoint(force.m_Waypoint);
		else
			force.m_CrewGroup.AddWaypoint(force.m_CrewWaypoint);
		ReportPackageProgress(force, "Mounted search ended; returning to deployment waypoint");
		return true;
	}

	protected void MonitorInfantryForce(AROR_OperationForce force)
	{
		AROR_Operation operation = m_Operations.GetOperation(force.m_sOperationId);
		if (!operation || operation.IsTerminal())
		{
			ReleaseForce(force);
			return;
		}
		if (!force.m_Group)
		{
			FailPackage(force, operation, "Operation group was deleted");
			return;
		}
		int living = force.CountLivingMembers();
		if (!force.HasCorrectMemberFactions())
		{
			FailOperation(operation, "Spawned infantry faction does not match commander role");
			return;
		}
		if (force.m_Group.IsDormant())
		{
			if (force.m_bSawMembers && force.m_Group.GetDormantAliveCount() == 0)
			{
				FailPackage(force, operation, "Operation infantry eliminated");
				return;
			}
			// Engine budget eviction is not a combat defeat. Retain the operation and request wake-up.
			force.m_iWakeRetryTicks--;
			if (force.m_iWakeRetryTicks <= 0 && force.m_Group.GetDormantAliveCount() > 0)
			{
				force.m_iWakeRetryTicks = m_iWakeRetryInterval;
				force.m_Group.RequestSpawn();
			}
			return;
		}
		force.m_iWakeRetryTicks = 0;
		if (force.m_bSawMembers && living == 0)
		{
			FailPackage(force, operation, "Operation infantry eliminated");
			return;
		}
		if (operation.GetState() == AROR_EOperationState.SPAWNING && living > 0 && force.m_Group.IsExpandComplete())
		{
			if (!force.m_Group.GetFaction() || force.m_Group.GetFaction().GetFactionKey() != force.m_sFactionKey)
				FailOperation(operation, "Spawned group has no matching faction");
			else
			{
				force.m_bReady = true;
				TryActivateOperation(operation);
			}
			return;
		}
		AROR_LocationManagerComponent locations = AROR_LocationManagerComponent.GetInstance();
		AROR_LocationComponent target;
		if (locations)
			target = locations.GetLocationById(operation.GetTargetLocationId());
		IEntity leader = force.m_Group.GetLeaderEntity();
		if (operation.GetState() == AROR_EOperationState.ACTIVE && !force.m_bArrivalLogged && target && leader && vector.DistanceXZ(leader.GetOrigin(), target.GetCenter()) <= m_fArrivalRadius)
		{
			force.m_bArrivalLogged = true;
			Print(string.Format("[AROR] Force %1 reached objective; awaiting territory/combat outcome", operation.GetId()), LogLevel.NORMAL);
		}
	}

	protected void Tick()
	{
		if (!Replication.IsServer())
			return;
		m_iExecutorClock++;
		if (!m_Operations)
		{
			m_Operations = AROR_OperationManagerComponent.GetInstance();
			if (m_Operations)
			{
				m_Operations.GetOnOperationChanged().Insert(OnOperationChanged);
				Print("[AROR] Operation executor connected to scheduler", LogLevel.NORMAL);
			}
		}
		if (!m_Operations)
			return;
		array<AROR_OperationForce> forces = {};
		foreach (AROR_OperationForce force : m_aForces)
			forces.Insert(force);
		foreach (AROR_OperationForce tracked : forces)
			MonitorForce(tracked);
		// Recover dispatches emitted before subscription, and retry capacity-blocked requests.
		array<AROR_Operation> operations = {};
		m_Operations.GetOperations(operations);
		foreach (AROR_Operation qrf : operations)
		{
			if (qrf.GetState() == AROR_EOperationState.SPAWNING && qrf.GetType() == AROR_EOperationType.QRF)
				StartForce(qrf);
		}
		foreach (AROR_Operation attack : operations)
		{
			if (attack.GetState() == AROR_EOperationState.SPAWNING && attack.GetType() == AROR_EOperationType.ATTACK)
				StartForce(attack);
		}
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;
		if (!m_bEnabled)
		{
			Print("[AROR] Operation executor disabled: enable m_bEnabled on the Chicken game-mode instance", LogLevel.WARNING);
			return;
		}
		// Attribute containers are populated from the prefab/world. Declaration initializers
		// overwrite that source data; create only missing containers after deserialization.
		if (!m_aForceProfiles)
			m_aForceProfiles = {};
		if (!m_aResponseLevels)
			m_aResponseLevels = {};
		if (m_bQRFVehicleSearch && (m_iQRFSearchDuration < 1 || m_iQRFSearchDuration > 900 || m_iQRFSearchStops < 1 || m_iQRFSearchStops > 12 || m_iQRFSearchDwell < 0 || m_iQRFSearchDwell > 60 || m_iQRFSearchLegTimeout < 10 || m_iQRFSearchLegTimeout > 300))
		{
			Print("[AROR] Invalid mounted QRF search settings; executor disabled", LogLevel.ERROR);
			return;
		}
		if (m_aResponseLevels.Count() == 0)
		{
			for (int level = 1; level <= 5; level++)
			{
				AROR_ResponseLevel response = new AROR_ResponseLevel();
				response.m_iLevel = level;
				response.m_iAttackWeight = 20;
				response.m_iQRFWeight = 20;
				m_aResponseLevels.Insert(response);
			}
		}
		if (m_iMaxGroups < 1 || m_iMaxMembers < 1 || m_iMaxVehicles < 0 || m_iWakeRetryInterval < 1 || m_fPackageSpacing < 10 || m_iWorkbenchResponseLevel < 0 || m_iWorkbenchResponseLevel > 5 || m_fMinTargetDistance < 0 || m_fMinPlayerDistance < 0 || m_fArrivalRadius <= 0 || owner.FindComponent(AROR_OperationExecutorComponent) != this || !ValidatePool())
		{
			Print("[AROR] Invalid or duplicate operation executor; disabled", LogLevel.ERROR);
			return;
		}
		GetGame().GetCallqueue().CallLater(Tick, 1000, true);
		Print("[AROR] Operation force executor enabled", LogLevel.NORMAL);
		Print(string.Format("[AROR] Executor configuration: profiles=%1 levels=%2 budgets groups=%3 members=%4 vehicles=%5", m_aForceProfiles.Count(), m_aResponseLevels.Count(), m_iMaxGroups, m_iMaxMembers, m_iMaxVehicles), LogLevel.NORMAL);
	}

	protected bool ValidatePool()
	{
		if (m_aForceProfiles.Count() > 64)
			return false;
		if (m_aResponseLevels.Count() > 5)
			return false;
		array<int> levels = {};
		foreach (AROR_ResponseLevel response : m_aResponseLevels)
		{
			if (!response || !response.IsValid() || levels.Contains(response.m_iLevel))
				return false;
			levels.Insert(response.m_iLevel);
		}
		array<string> ids = {};
		foreach (AROR_ForceProfile profile : m_aForceProfiles)
		{
			if (!profile || !profile.IsValid() || ids.Contains(profile.m_sId))
			{
				Print("[AROR] Force profiles must be valid and have globally unique IDs", LogLevel.ERROR);
				return false;
			}
			ids.Insert(profile.m_sId);
			if (!profile.HasNativeExecutor())
				Print(string.Format("[AROR] Profile %1 excluded: airborne flight/landing/escort adapter is not implemented", profile.m_sId), LogLevel.WARNING);
		}
#ifdef WORKBENCH
		if ((!m_sWorkbenchForceProfileId.IsEmpty() && !ids.Contains(m_sWorkbenchForceProfileId)) || (m_iWorkbenchResponseLevel > 0 && !levels.Contains(m_iWorkbenchResponseLevel)))
		{
			Print("[AROR] Workbench forced profile/level is absent from configuration", LogLevel.ERROR);
			return false;
		}
#endif
		return true;
	}

	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(Tick);
		if (m_Operations)
			m_Operations.GetOnOperationChanged().Remove(OnOperationChanged);
		foreach (AROR_OperationForce force : m_aForces)
			force.Cleanup_S();
		m_aForces.Clear();
		super.OnDelete(owner);
	}
}
