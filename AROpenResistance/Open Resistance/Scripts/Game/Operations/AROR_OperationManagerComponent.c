void AROR_OnOperationChanged(AROR_Operation operation);
typedef func AROR_OnOperationChanged;
typedef ScriptInvokerBase<AROR_OnOperationChanged> AROR_OnOperationChangedInvoker;

[ComponentEditorProps(category: "OpenResistance/Operations", description: "Server attack scheduler and QRF requests for both enemy roles. Attach once to the game mode.")]
class AROR_OperationManagerComponentClass : SCR_BaseGameModeComponentClass
{
}

class AROR_OperationManagerComponent : SCR_BaseGameModeComponent
{
	[Attribute("", desc: "Optional Occupier scheduled attack target: stable location ID. Empty disables automatic attacks for this role.")]
	protected string m_sOccupiersAttackTargetId;
	[Attribute("", desc: "Optional Invader scheduled attack target: stable location ID. Empty disables automatic attacks for this role.")]
	protected string m_sInvadersAttackTargetId;
	[Attribute("300", desc: "Minimum seconds between scheduled attacks per role", params: "1 86400 1")]
	protected int m_iAttackInterval;
	[Attribute("60", desc: "Preparation delay in seconds for attacks", params: "0 3600 1")]
	protected int m_iAttackPreparation;
	[Attribute("10", desc: "Reaction delay in seconds for QRFs", params: "0 3600 1")]
	protected int m_iQRFReaction;
	[Attribute("120", desc: "Minimum seconds between accepted QRF requests per role", params: "1 86400 1")]
	protected int m_iQRFCooldown;
	[Attribute("1", desc: "Maximum outstanding attacks per role, including planned requests", params: "1 16 1")]
	protected int m_iMaxAttacks;
	[Attribute("1", desc: "Separate maximum outstanding QRFs per role", params: "1 16 1")]
	protected int m_iMaxQRFs;
	[Attribute("30", desc: "Seconds allowed for executor spawn acknowledgement", params: "1 3600 1")]
	protected int m_iSpawnTimeout;
	[Attribute("900", desc: "Seconds allowed for an active operation before failure", params: "1 86400 1")]
	protected int m_iActiveTimeout;
	[Attribute("32", desc: "Maximum terminal operation records retained for inspection", params: "0 256 1")]
	protected int m_iHistoryLimit;

	protected static AROR_OperationManagerComponent s_Instance;
	protected AROR_CommanderManagerComponent m_Commanders;
	protected ref array<ref AROR_Operation> m_aOperations = {};
	protected ref AROR_OnOperationChangedInvoker m_OnOperationChanged = new AROR_OnOperationChangedInvoker();
	protected ref array<ref AROR_Operation> m_aPendingNotifications = {};
	protected bool m_bNotifying;
	protected int m_iClock;
	protected int m_iNextId;
	protected int m_iOccupiersNextAttack;
	protected int m_iInvadersNextAttack;
	protected int m_iOccupiersNextQRF;
	protected int m_iInvadersNextQRF;

	static AROR_OperationManagerComponent GetInstance()
	{
		return s_Instance;
	}
	AROR_OnOperationChangedInvoker GetOnOperationChanged()
	{
		return m_OnOperationChanged;
	}

	AROR_Operation GetOperation(string id)
	{
		foreach (AROR_Operation operation : m_aOperations)
		{
			if (operation.GetId() == id)
				return operation;
		}
		return null;
	}

	int GetOperations(notnull array<AROR_Operation> outOperations)
	{
		outOperations.Clear();
		foreach (AROR_Operation operation : m_aOperations)
			outOperations.Insert(operation);
		return outOperations.Count();
	}

	//! Trusted server call. Target hostility must be checked by the territory/strategy caller.
	//! Returns the existing ID for an identical outstanding request, or empty on rejection.
	string ScheduleAttack_S(AROR_EFactionRole role, string targetId, int delaySeconds = -1)
	{
		if (delaySeconds == -1)
			delaySeconds = m_iAttackPreparation;
		return RequestOperation_S(role, AROR_EOperationType.ATTACK, targetId, delaySeconds);
	}

	//! Trusted combat/territory integration: defendingRole owns the attacked location.
	//! No client RPC: callers must establish ownership and a real hostile attack before reporting.
	string ReportAttack_S(AROR_EFactionRole defendingRole, string locationId)
	{
		return RequestOperation_S(defendingRole, AROR_EOperationType.QRF, locationId, m_iQRFReaction);
	}

	//! Executor acknowledges that its force exists and has received its orders.
	bool MarkActive_S(string operationId)
	{
		return ChangeState_S(GetOperation(operationId), AROR_EOperationState.ACTIVE, string.Empty);
	}

	//! Executor/territory integration reports a terminal outcome; never changes ownership itself.
	bool ResolveOperation_S(string operationId, AROR_EOperationState result, string reason)
	{
		if (result != AROR_EOperationState.SUCCEEDED && result != AROR_EOperationState.FAILED && result != AROR_EOperationState.CANCELLED)
			return false;
		return ChangeState_S(GetOperation(operationId), result, reason);
	}

	protected bool IsReady()
	{
		return Replication.IsServer() && s_Instance == this && m_Commanders && m_Commanders.IsCampaignActive();
	}

	protected bool IsEnemyRole(AROR_EFactionRole role)
	{
		return role == AROR_EFactionRole.OCCUPIERS || role == AROR_EFactionRole.INVADERS;
	}

	protected bool HasLocation(string id)
	{
		AROR_LocationManagerComponent locations = AROR_LocationManagerComponent.GetInstance();
		return !id.IsEmpty() && locations && locations.GetLocationById(id);
	}

	protected int GetNextRequestTime(AROR_EFactionRole role, AROR_EOperationType type)
	{
		if (type == AROR_EOperationType.ATTACK)
		{
			if (role == AROR_EFactionRole.OCCUPIERS)
				return m_iOccupiersNextAttack;
			return m_iInvadersNextAttack;
		}
		if (role == AROR_EFactionRole.OCCUPIERS)
			return m_iOccupiersNextQRF;
		return m_iInvadersNextQRF;
	}

	protected string RequestOperation_S(AROR_EFactionRole role, AROR_EOperationType type, string targetId, int delaySeconds)
	{
		if (!IsReady() || !IsEnemyRole(role) || delaySeconds < 0 || delaySeconds > 86400 || !HasLocation(targetId))
			return string.Empty;

		int count;
		foreach (AROR_Operation operation : m_aOperations)
		{
			if (operation.IsTerminal() || operation.GetRole() != role)
				continue;
			if (operation.GetTargetLocationId() == targetId)
			{
				if (operation.GetType() == type)
					return operation.GetId();
				return string.Empty;
			}
			if (operation.GetType() == type)
				count++;
		}
		int limit = m_iMaxAttacks;
		if (type == AROR_EOperationType.QRF)
			limit = m_iMaxQRFs;
		if (count >= limit || m_iClock < GetNextRequestTime(role, type))
			return string.Empty;

		m_iNextId++;
		string id = string.Format("operation_%1", m_iNextId);
		AROR_Operation created = new AROR_Operation(id, role, type, targetId, m_iClock, delaySeconds);
		m_aOperations.Insert(created);
		if (type == AROR_EOperationType.ATTACK)
		{
			if (role == AROR_EFactionRole.OCCUPIERS)
				m_iOccupiersNextAttack = m_iClock + m_iAttackInterval;
			else
				m_iInvadersNextAttack = m_iClock + m_iAttackInterval;
		}
		else
		{
			if (role == AROR_EFactionRole.OCCUPIERS)
				m_iOccupiersNextQRF = m_iClock + m_iQRFCooldown;
			else
				m_iInvadersNextQRF = m_iClock + m_iQRFCooldown;
		}
		NotifyChanged(created);
		return id;
	}

	protected bool ChangeState_S(AROR_Operation operation, AROR_EOperationState state, string reason)
	{
		if (!Replication.IsServer() || s_Instance != this || !operation || !operation.Transition_S(state, m_iClock, reason))
			return false;
		NotifyChanged(operation);
		return true;
	}

	protected void NotifyChanged(AROR_Operation operation)
	{
		Print(string.Format("[AROR] Operation %1 role=%2 type=%3 target=%4 state=%5: %6", operation.GetId(), operation.GetRole(), operation.GetType(), operation.GetTargetLocationId(), operation.GetState(), operation.GetResult()), LogLevel.NORMAL);
		m_aPendingNotifications.Insert(operation);
		// An executor may resolve an operation inside its dispatch callback. ScriptInvoker
		// forbids recursive Invoke; deliver nested changes after the current invocation returns.
		if (m_bNotifying)
			return;
		m_bNotifying = true;
		while (m_aPendingNotifications.Count() > 0)
		{
			AROR_Operation next = m_aPendingNotifications[0];
			m_aPendingNotifications.Remove(0);
			m_OnOperationChanged.Invoke(next);
		}
		m_bNotifying = false;
	}

	protected void OnCommanderEvaluation(AROR_Commander commander)
	{
		if (!IsReady() || !commander || commander.IsPlayerDirected())
			return;
		string targetId;
		if (commander.GetRole() == AROR_EFactionRole.OCCUPIERS)
			targetId = m_sOccupiersAttackTargetId;
		else if (commander.GetRole() == AROR_EFactionRole.INVADERS)
			targetId = m_sInvadersAttackTargetId;
		if (!targetId.IsEmpty())
			ScheduleAttack_S(commander.GetRole(), targetId);
	}

	protected void ProcessOperations(AROR_EOperationType type)
	{
		// Snapshot allows synchronous executor callbacks to schedule or resolve other requests safely.
		array<AROR_Operation> snapshot = {};
		GetOperations(snapshot);
		foreach (AROR_Operation operation : snapshot)
		{
			if (operation.IsTerminal() || operation.GetType() != type)
				continue;
			if (!HasLocation(operation.GetTargetLocationId()))
				ChangeState_S(operation, AROR_EOperationState.CANCELLED, "Target location no longer registered");
			else if (operation.GetState() == AROR_EOperationState.PLANNED && m_iClock >= operation.GetDueTime())
				ChangeState_S(operation, AROR_EOperationState.SPAWNING, "Executor requested");
			else if (operation.GetState() == AROR_EOperationState.SPAWNING && m_iClock - operation.GetStateSince() >= m_iSpawnTimeout)
				ChangeState_S(operation, AROR_EOperationState.FAILED, "Spawn acknowledgement timed out");
			else if (operation.GetState() == AROR_EOperationState.ACTIVE && m_iClock - operation.GetStateSince() >= m_iActiveTimeout)
				ChangeState_S(operation, AROR_EOperationState.FAILED, "Operation timed out");
		}
	}

	protected void Tick()
	{
		if (!Replication.IsServer() || s_Instance != this)
			return;
		if (!m_Commanders)
		{
			m_Commanders = AROR_CommanderManagerComponent.GetInstance();
			if (m_Commanders)
				m_Commanders.GetOnCommanderEvaluation().Insert(OnCommanderEvaluation);
		}
		if (!IsReady())
			return;
		m_iClock++;
		ProcessOperations(AROR_EOperationType.QRF);
		ProcessOperations(AROR_EOperationType.ATTACK);

		int historyCount;
		for (int i = m_aOperations.Count() - 1; i >= 0; i--)
		{
			if (!m_aOperations[i].IsTerminal())
				continue;
			historyCount++;
			if (historyCount > m_iHistoryLimit)
				m_aOperations.Remove(i);
		}
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;
		if (s_Instance || m_iAttackInterval < 1 || m_iAttackPreparation < 0 || m_iQRFReaction < 0 || m_iQRFCooldown < 1 || m_iMaxAttacks < 1 || m_iMaxQRFs < 1 || m_iSpawnTimeout < 1 || m_iActiveTimeout < 1 || m_iHistoryLimit < 0)
		{
			Print("[AROR] Operation manager duplicate or invalid configuration; disabled", LogLevel.ERROR);
			return;
		}
		s_Instance = this;
		GetGame().GetCallqueue().CallLater(Tick, 1000, true);
		Print("[AROR] Operation scheduler initialised for Occupiers and Invaders", LogLevel.NORMAL);
		Print("[AROR] Operation notifications: nested changes queued (recursive Invoke protection enabled)", LogLevel.NORMAL);
	}

	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(Tick);
		if (m_Commanders)
			m_Commanders.GetOnCommanderEvaluation().Remove(OnCommanderEvaluation);
		if (s_Instance == this)
			s_Instance = null;
		super.OnDelete(owner);
	}
}
