#ifdef WORKBENCH
//! Regression probe: resolving another operation from a notification must not invoke recursively.
class AROR_OperationReentryProbe
{
	AROR_OperationManagerComponent m_Manager;
	string m_sTriggerId;
	string m_sFollowupId;
	bool m_bInside;
	bool m_bReentered;
	int m_iFollowupNotifications;

	void OnChanged(AROR_Operation operation)
	{
		if (m_bInside)
			m_bReentered = true;
		if (operation.GetId() == m_sFollowupId && operation.IsTerminal())
			m_iFollowupNotifications++;
		if (operation.GetId() != m_sTriggerId || !operation.IsTerminal())
			return;
		m_bInside = true;
		m_Manager.ResolveOperation_S(m_sFollowupId, AROR_EOperationState.CANCELLED, "Workbench nested notification regression");
		m_bInside = false;
	}
}
#endif
