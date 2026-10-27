//! Territory policy only. Existing operations/executor/tactical commander own the physical attack.
modded class AROR_OperationManagerComponent
{
	[Attribute("0", desc: "Schedule Occupier attacks on Resistance-held military territory after its capture QRF finishes. Settlements remain support-only and HQ is excluded.")]
	protected bool m_bTerritoryAttacksEnabled;
	[Attribute("900", desc: "Minimum seconds from capture before an automatic attack can be planned", params: "0 7200 30")]
	protected int m_iTerritoryAttackGrace;
	[Attribute("300", desc: "Minimum recovery time after a physically activated capture QRF ends", params: "0 3600 30")]
	protected int m_iTerritoryAttackAfterQRF;
	[Attribute("900", desc: "Global recovery time after any attack ends, before another automatic attack", params: "60 7200 30")]
	protected int m_iTerritoryAttackRest;
	protected ref array<ref AROR_TerritoryAttackState> m_aAttackTerritories = {};
	protected AROR_LocationManagerComponent m_AttackLocations;
	protected int m_iAttackRestUntil;
	protected int m_iNextTerritoryAttackTick;

	override protected void Tick()
	{
		super.Tick();
		if (!Replication.IsServer() || !m_bTerritoryAttacksEnabled || !IsReady())
			return;
		if (!m_AttackLocations)
		{
			m_AttackLocations = AROR_LocationManagerComponent.GetInstance();
			if (!m_AttackLocations)
				return;
			array<AROR_LocationComponent> all = {};
			m_AttackLocations.GetLocations(all);
			foreach (AROR_LocationComponent location : all)
				OnAttackTerritoryChanged(location);
			m_AttackLocations.GetOnLocationControlChanged().Insert(OnAttackTerritoryChanged);
			m_AttackLocations.GetOnLocationRegistered().Insert(OnAttackTerritoryChanged);
			m_AttackLocations.GetOnLocationUnregistered().Insert(OnAttackTerritoryRemoved);
		}
		if (m_iClock < m_iNextTerritoryAttackTick)
			return;
		m_iNextTerritoryAttackTick = m_iClock + 15;
		if (m_iClock < m_iAttackRestUntil || HasOutstandingAttackOrQRF())
			return;
		// Pending capture responses also take precedence over attacks.
		if (!m_aQRFThreats.IsEmpty())
			return;
		AROR_OperationExecutorComponent executor = AROR_OperationExecutorComponent.Cast(GetOwner().FindComponent(AROR_OperationExecutorComponent));
		if (!executor)
			return;
		AROR_TerritoryAttackState selected;
		foreach (AROR_TerritoryAttackState state : m_aAttackTerritories)
		{
			if (!state.IsDue(m_iClock, Math.Max(0, m_iTerritoryAttackGrace), Math.Max(0, m_iTerritoryAttackAfterQRF)))
				continue;
			AROR_LocationComponent target = m_AttackLocations.GetLocationById(state.m_sLocationId);
			if (!IsAttackTerritory(target))
				continue;
			if (!executor.HasMarsOrigin_S(AROR_EFactionRole.OCCUPIERS, state.m_sLocationId))
				continue;
			if (!selected || state.m_iNextAttempt < selected.m_iNextAttempt)
				selected = state;
		}
		if (!selected)
			return;
		selected.m_iNextAttempt = m_iClock + 30;
		int tier = GetCrackdownLevel(AROR_EFactionRole.OCCUPIERS);
		if (m_bQRFStrategicEconomy && !executor.CanFundQRFThreat_S(AROR_EFactionRole.OCCUPIERS, tier, false, selected.m_sLocationId))
			return;
		string id = ScheduleAttack_S(AROR_EFactionRole.OCCUPIERS, selected.m_sLocationId);
		AROR_Operation attack = GetOperation(id);
		if (!attack)
			return;
		attack.m_iRequestedTier = tier;
		attack.m_iPoolReadinessLevel = GetReadinessLevel(AROR_EFactionRole.OCCUPIERS);
		selected.m_iNextAttempt = m_iClock + Math.Max(60, m_iTerritoryAttackRest);
		AnnounceTerritoryAttack(selected.m_sLocationId);
		Print(string.Format("[AROR ATTACK] %1 target=%2 captureAge=%3 QRFEnded=%4 readiness=%5 crackdownTier=%6", id, selected.m_sLocationId, m_iClock - selected.m_iCapturedAt, selected.m_iQRFCompletedAt, attack.m_iPoolReadinessLevel, tier), LogLevel.NORMAL);
	}

	protected void AnnounceTerritoryAttack(string locationId)
	{
		AROR_LocationComponent target = m_AttackLocations.GetLocationById(locationId);
		if (target)
			AROR_NotificationManagerComponent.Send_S(new AROR_Notification(AROR_ENotificationType.HEADLINE, "ENEMY ATTACK INBOUND / " + target.GetName(), string.Empty, AROR_UIColors.Get(AROR_UIColors.WARN), "Enemy movement"));
	}

	protected bool IsAttackTerritory(AROR_LocationComponent location)
	{
		return location && location.GetControllingRole() == AROR_EFactionRole.RESISTANCE && !location.IsControlFixed() && AROR_CaptureLocationAction.IsCapturableType(location.GetLocationType());
	}

	protected void OnAttackTerritoryChanged(AROR_LocationComponent location)
	{
		if (!Replication.IsServer() || !location)
			return;
		OnAttackTerritoryRemoved(location);
		if (!IsAttackTerritory(location))
			return;
		AROR_TerritoryAttackState state = new AROR_TerritoryAttackState();
		state.m_sLocationId = location.GetId();
		state.m_iCapturedAt = m_iClock;
		state.m_iNextAttempt = m_iClock;
		m_aAttackTerritories.Insert(state);
	}

	protected void OnAttackTerritoryRemoved(AROR_LocationComponent location)
	{
		if (!location)
			return;
		for (int i = m_aAttackTerritories.Count() - 1; i >= 0; i--)
		{
			if (m_aAttackTerritories[i].m_sLocationId == location.GetId())
				m_aAttackTerritories.Remove(i);
		}
	}

	protected bool HasOutstandingAttackOrQRF()
	{
		foreach (AROR_Operation operation : m_aOperations)
		{
			if (!operation.IsTerminal())
				return true;
		}
		return false;
	}

	override protected void NotifyChanged(AROR_Operation operation)
	{
		super.NotifyChanged(operation);
		if (!Replication.IsServer() || !m_bTerritoryAttacksEnabled || !operation || operation.m_bDebugRequest)
			return;
		if (operation.GetType() == AROR_EOperationType.ATTACK)
		{
			if (operation.IsTerminal())
				m_iAttackRestUntil = m_iClock + Math.Max(60, m_iTerritoryAttackRest);
			return;
		}
		if (!operation.m_bCaptureThreatRecorded)
			return;
		foreach (AROR_TerritoryAttackState state : m_aAttackTerritories)
		{
			if (state.m_sLocationId != operation.GetTargetLocationId())
				continue;
			if (operation.GetState() == AROR_EOperationState.ACTIVE)
			{
				state.m_sActiveQRF = operation.GetId();
				state.m_iQRFCompletedAt = -1;
			}
			else if (operation.IsTerminal() && state.m_sActiveQRF == operation.GetId())
			{
				state.m_iQRFCompletedAt = m_iClock;
				state.m_sActiveQRF = string.Empty;
			}
		}
	}

	override void OnDelete(IEntity owner)
	{
		if (m_AttackLocations)
		{
			m_AttackLocations.GetOnLocationControlChanged().Remove(OnAttackTerritoryChanged);
			m_AttackLocations.GetOnLocationRegistered().Remove(OnAttackTerritoryChanged);
			m_AttackLocations.GetOnLocationUnregistered().Remove(OnAttackTerritoryRemoved);
		}
		m_AttackLocations = null;
		m_aAttackTerritories.Clear();
		super.OnDelete(owner);
	}
}
