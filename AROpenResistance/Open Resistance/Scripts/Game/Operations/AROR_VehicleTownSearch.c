//! Server-owned, bounded mounted search. Uses town bounds and reachable roads, never player positions.
class AROR_VehicleTownSearch
{
	protected SCR_AIGroup m_Group;
	protected IEntity m_Vehicle;
	protected AROR_LocationComponent m_Location;
	protected AIWaypoint m_Waypoint;
	protected ResourceName m_sWaypointPrefab;
	protected string m_sOperationId;
	protected bool m_bActive;
	protected int m_iEndTime;
	protected int m_iLegEndTime;
	protected int m_iDwellEndTime;
	protected int m_iDwellSeconds;
	protected int m_iLegSeconds;
	protected int m_iMaxStops;
	protected int m_iIssuedStops;
	protected int m_iPointOrdinal;
	protected float m_fAngle;

	bool Start_S(SCR_AIGroup group, IEntity vehicle, AROR_LocationComponent location, AIWaypoint previousWaypoint, ResourceName waypointPrefab, string operationId, int now, int duration, int maxStops, int dwellSeconds, int legSeconds)
	{
		if (!Replication.IsServer() || !group || !vehicle || !location || duration < 1 || maxStops < 1 || dwellSeconds < 0 || legSeconds < 1)
			return false;
		m_Group = group;
		m_Vehicle = vehicle;
		m_Location = location;
		m_sWaypointPrefab = waypointPrefab;
		m_sOperationId = operationId;
		m_iEndTime = now + duration;
		m_iDwellSeconds = dwellSeconds;
		m_iLegSeconds = legSeconds;
		m_iMaxStops = maxStops;
		m_fAngle = Math.RandomFloat(0, 6.2831853);
		// Find a usable stop before disturbing the original deployment order.
		if (!CreateNextStop_S(now))
			return false;
		if (previousWaypoint)
			m_Group.RemoveWaypoint(previousWaypoint);
		m_Group.AddWaypoint(m_Waypoint);
		m_bActive = true;
		Print(string.Format("[AROR] Mounted town search started: %1 location=%2 duration=%3s", m_sOperationId, location.GetId(), duration), LogLevel.NORMAL);
		return true;
	}

	//! True while searching. False hands the vehicle back to its original deployment policy.
	bool Tick_S(int now)
	{
		if (!Replication.IsServer() || !m_bActive)
			return false;
		if (!m_Group || !m_Vehicle || !m_Location || now >= m_iEndTime)
		{
			Finish_S("Search duration elapsed or search entity unavailable");
			return false;
		}
		if (m_Waypoint)
		{
			if (vector.DistanceXZ(m_Vehicle.GetOrigin(), m_Waypoint.GetOrigin()) <= 25)
			{
				Print(string.Format("[AROR] Mounted search %1 reached stop %2", m_sOperationId, m_iIssuedStops), LogLevel.NORMAL);
				ClearWaypoint_S();
				m_iDwellEndTime = now + m_iDwellSeconds;
			}
			else if (now >= m_iLegEndTime)
			{
				Print(string.Format("[AROR] Mounted search %1 skipped stalled stop %2", m_sOperationId, m_iIssuedStops), LogLevel.NORMAL);
				ClearWaypoint_S();
				m_iDwellEndTime = now;
			}
			else
				return true;
		}
		if (now < m_iDwellEndTime)
			return true;
		if (m_iIssuedStops >= m_iMaxStops || !CreateNextStop_S(now))
		{
			Finish_S("Stop limit reached or no further reachable town road point");
			return false;
		}
		m_Group.AddWaypoint(m_Waypoint);
		return true;
	}

	protected bool CreateNextStop_S(int now)
	{
		ChimeraAIWorld aiWorld = ChimeraAIWorld.Cast(GetGame().GetAIWorld());
		if (!aiWorld)
			return false;
		RoadNetworkManager roads = aiWorld.GetRoadNetworkManager();
		if (!roads)
			return false;
		vector center = m_Location.GetCenter();
		// Visit different sectors, with varied radii. Bounded retries reject roads outside town.
		for (int attempt = 0; attempt < 8; attempt++)
		{
			float angle = m_fAngle + m_iPointOrdinal * 2.3999632;
			m_iPointOrdinal++;
			float radius = m_Location.GetRadius() * Math.RandomFloat(0.35, 0.8);
			vector desired = center;
			desired[0] = desired[0] + Math.Cos(angle) * radius;
			desired[2] = desired[2] + Math.Sin(angle) * radius;
			vector reachable;
			if (!roads.GetReachableWaypointInRoad(m_Vehicle.GetOrigin(), desired, 75, reachable))
				continue;
			if (!m_Location.IsPositionInside(reachable) || vector.DistanceXZ(m_Vehicle.GetOrigin(), reachable) < 40)
				continue;
			Resource resource = Resource.Load(m_sWaypointPrefab);
			if (!resource || !resource.IsValid())
				return false;
			EntitySpawnParams spawnParams = new EntitySpawnParams();
			spawnParams.TransformMode = ETransformMode.WORLD;
			reachable[1] = GetGame().GetWorld().GetSurfaceY(reachable[0], reachable[2]);
			spawnParams.Transform[3] = reachable;
			IEntity entity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
			m_Waypoint = AIWaypoint.Cast(entity);
			if (!m_Waypoint)
			{
				if (entity)
					SCR_EntityHelper.DeleteEntityAndChildren(entity);
				return false;
			}
			m_Waypoint.SetCompletionRadius(20);
			m_iLegEndTime = now + m_iLegSeconds;
			m_iIssuedStops++;
			Print(string.Format("[AROR] Mounted search %1 stop=%2 road point=%3", m_sOperationId, m_iIssuedStops, reachable), LogLevel.NORMAL);
			return true;
		}
		return false;
	}

	protected void ClearWaypoint_S()
	{
		if (m_Waypoint)
		{
			if (m_Group)
				m_Group.RemoveWaypoint(m_Waypoint);
			SCR_EntityHelper.DeleteEntityAndChildren(m_Waypoint);
		}
		m_Waypoint = null;
	}

	protected void Finish_S(string reason)
	{
		Cleanup_S();
		Print(string.Format("[AROR] Mounted town search finished: %1: %2; resuming deployment", m_sOperationId, reason), LogLevel.NORMAL);
	}

	void Cleanup_S()
	{
		if (!Replication.IsServer())
			return;
		ClearWaypoint_S();
		m_bActive = false;
	}
}
