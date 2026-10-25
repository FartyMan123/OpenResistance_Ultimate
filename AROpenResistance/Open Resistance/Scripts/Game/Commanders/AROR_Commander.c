//! Server-owned command context. The role is its identity; the faction key is configuration.
//! Resistance orders will come from players; enemy decisions use the manager's evaluation event.
//! This context does not own location state, entities or resource balances.
class AROR_Commander
{
	protected AROR_EFactionRole m_eRole;
	protected string m_sFactionKey;
	protected bool m_bPlayerDirected;

	void AROR_Commander(AROR_EFactionRole role, string factionKey, bool playerDirected)
	{
		m_eRole = role;
		m_sFactionKey = factionKey;
		m_bPlayerDirected = playerDirected;
	}

	AROR_EFactionRole GetRole()
	{
		return m_eRole;
	}

	string GetFactionKey()
	{
		return m_sFactionKey;
	}

	bool IsPlayerDirected()
	{
		return m_bPlayerDirected;
	}
}
