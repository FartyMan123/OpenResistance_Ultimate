//! Session-local strategic request. Holds no entity references or ownership/resource state.
class AROR_Operation
{
	protected string m_sId;
	protected AROR_EFactionRole m_eRole;
	protected AROR_EOperationType m_eType;
	protected string m_sTargetLocationId;
	protected AROR_EOperationState m_eState;
	protected int m_iDueTime;
	protected int m_iStateSince;
	protected string m_sResult;
	protected string m_sForceProfileId;
	protected int m_iResponseLevel;

	int GetResponseLevel()
	{
		return m_iResponseLevel;
	}

	string GetForceProfileId()
	{
		return m_sForceProfileId;
	}

	//! A request selects once; retries must retain its original force profile.
	bool BindForceProfile_S(string profileId, int responseLevel = 1)
	{
		if (!Replication.IsServer() || m_eState != AROR_EOperationState.SPAWNING || profileId.IsEmpty() || !m_sForceProfileId.IsEmpty() || responseLevel < 1 || responseLevel > 5)
			return false;
		m_sForceProfileId = profileId;
		m_iResponseLevel = responseLevel;
		return true;
	}

	void AROR_Operation(string id, AROR_EFactionRole role, AROR_EOperationType type, string targetLocationId, int now, int delaySeconds)
	{
		m_sId = id;
		m_eRole = role;
		m_eType = type;
		m_sTargetLocationId = targetLocationId;
		m_eState = AROR_EOperationState.PLANNED;
		m_iDueTime = now + delaySeconds;
		m_iStateSince = now;
	}

	string GetId()
	{
		return m_sId;
	}
	AROR_EFactionRole GetRole()
	{
		return m_eRole;
	}
	AROR_EOperationType GetType()
	{
		return m_eType;
	}
	string GetTargetLocationId()
	{
		return m_sTargetLocationId;
	}
	AROR_EOperationState GetState()
	{
		return m_eState;
	}
	int GetDueTime()
	{
		return m_iDueTime;
	}
	int GetStateSince()
	{
		return m_iStateSince;
	}
	string GetResult()
	{
		return m_sResult;
	}

	bool IsTerminal()
	{
		return m_eState == AROR_EOperationState.SUCCEEDED || m_eState == AROR_EOperationState.FAILED || m_eState == AROR_EOperationState.CANCELLED;
	}

	//! Internal manager mutation. Consumers use manager acknowledgement/result APIs so events fire.
	bool Transition_S(AROR_EOperationState nextState, int now, string result)
	{
		if (!Replication.IsServer() || IsTerminal())
			return false;

		bool valid = nextState == AROR_EOperationState.FAILED || nextState == AROR_EOperationState.CANCELLED;
		if (m_eState == AROR_EOperationState.PLANNED && nextState == AROR_EOperationState.SPAWNING)
			valid = true;
		if (m_eState == AROR_EOperationState.SPAWNING && nextState == AROR_EOperationState.ACTIVE)
			valid = true;
		if (m_eState == AROR_EOperationState.ACTIVE && nextState == AROR_EOperationState.SUCCEEDED)
			valid = true;
		if (!valid)
			return false;

		m_eState = nextState;
		m_iStateSince = now;
		m_sResult = result;
		return true;
	}
}
