//! Owns only entities created for one operation. References to world entities are weak.
class AROR_OperationForce
{
	string m_sOperationId;
	string m_sFactionKey;
	SCR_AIGroup m_Group;
	AIWaypoint m_Waypoint;
	int m_iReservedMembers;
	int m_iReservedGroups = 1;
	int m_iReservedVehicles;
	int m_iPackageIndex;
	bool m_bReservation;
	bool m_bReady;
	bool m_bOrdersIssued;
	bool m_bSawMembers;
	bool m_bArrivalLogged;
	string m_sLastProgress;
	bool m_bLogFriendlyDamage;
	protected ref array<ref AROR_OperationDamageObserver> m_aDamageObservers = {};
	int m_iWakeRetryTicks;
	protected ref array<IEntity> m_aMembers = {};

	void OnAgentAdded(AIAgent agent)
	{
		if (!Replication.IsServer() || !agent)
			return;
		IEntity entity = agent.GetControlledEntity();
		if (entity && !m_aMembers.Contains(entity))
		{
			// Initialize only on first registration. Later faction changes must fail validation,
			// rather than being silently reset by the periodic member counter.
			FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
			if (affiliation && affiliation.GetAffiliatedFactionKey().IsEmpty() && !m_sFactionKey.IsEmpty())
				affiliation.SetAffiliatedFactionByKey(m_sFactionKey);
			m_aMembers.Insert(entity);
			TrackDamage_S(entity);
			m_bSawMembers = true;
		}
	}

	void TrackDamage_S(IEntity entity)
	{
		if (!Replication.IsServer() || !m_bLogFriendlyDamage || !entity)
			return;
		foreach (AROR_OperationDamageObserver observer : m_aDamageObservers)
		{
			if (observer.GetEntity() == entity)
				return;
		}
		AROR_OperationDamageObserver added = new AROR_OperationDamageObserver();
		if (added.Attach_S(entity, m_sOperationId, m_iPackageIndex))
			m_aDamageObservers.Insert(added);
	}

	//! Also captures already-created members when recovering an existing dispatch.
	int CountLivingMembers()
	{
		if (!m_Group)
			return 0;
		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);
		int living;
		foreach (AIAgent agent : agents)
		{
			OnAgentAdded(agent);
			if (!agent)
				continue;
			ChimeraCharacter character = ChimeraCharacter.Cast(agent.GetControlledEntity());
			if (character && character.GetCharacterController() && !character.GetCharacterController().IsDead())
				living++;
		}
		if (living > 0)
			m_bSawMembers = true;
		return living;
	}

	bool HasCorrectMemberFactions()
	{
		foreach (IEntity member : m_aMembers)
		{
			if (!member)
				continue;
			FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(member.FindComponent(FactionAffiliationComponent));
			if (!affiliation || affiliation.GetAffiliatedFactionKey() != m_sFactionKey)
				return false;
		}
		return true;
	}

	void Cleanup_S()
	{
		if (!Replication.IsServer())
			return;
		CountLivingMembers();
		foreach (AROR_OperationDamageObserver observer : m_aDamageObservers)
			observer.Detach_S();
		m_aDamageObservers.Clear();
		if (m_Group)
		{
			m_Group.GetOnAgentAdded().Remove(OnAgentAdded);
			if (m_Waypoint)
				m_Group.RemoveWaypoint(m_Waypoint);
			// Deleting the group retires its outstanding engine spawn requests.
			// Preserve the group entity if a player has taken command during testing.
			if (m_Group.GetPlayerCount() == 0)
				SCR_EntityHelper.DeleteEntityAndChildren(m_Group);
		}
		foreach (IEntity member : m_aMembers)
		{
			if (member && GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(member) == 0)
				SCR_EntityHelper.DeleteEntityAndChildren(member);
		}
		if (m_Waypoint)
			SCR_EntityHelper.DeleteEntityAndChildren(m_Waypoint);
		m_Group = null;
		m_Waypoint = null;
		m_aMembers.Clear();
	}
}
