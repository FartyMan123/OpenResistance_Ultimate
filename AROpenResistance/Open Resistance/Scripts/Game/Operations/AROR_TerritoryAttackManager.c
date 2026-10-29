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
	protected string m_sAttackHoldKey;	//!< Last logged reason no attack started; logged again only when it changes

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
		if (m_iClock < m_iAttackRestUntil)
		{
			HoldAttacks("rest", string.Format("resting after the last attack for another %1s", m_iAttackRestUntil - m_iClock));
			return;
		}
		AROR_Operation busy = OutstandingOperation();
		if (busy)
		{
			HoldAttacks("busy:" + busy.GetId(), string.Format("waiting for %1 (type=%2 role=%3 target=%4 state=%5) to end", busy.GetId(), busy.GetType(), busy.GetRole(), busy.GetTargetLocationId(), busy.GetState()));
			return;
		}
		// Pending capture responses also take precedence over attacks.
		if (!m_aQRFThreats.IsEmpty())
		{
			HoldAttacks("threats", string.Format("%1 queued capture response(s) take priority", m_aQRFThreats.Count()));
			return;
		}
		AROR_OperationExecutorComponent executor = AROR_OperationExecutorComponent.Cast(GetOwner().FindComponent(AROR_OperationExecutorComponent));
		if (!executor)
		{
			HoldAttacks("executor", "no operation executor on the game mode");
			return;
		}
		AROR_TerritoryAttackState selected;
		string waitKey;
		string waiting;
		int reported;
		foreach (AROR_TerritoryAttackState state : m_aAttackTerritories)
		{
			AROR_LocationComponent target = m_AttackLocations.GetLocationById(state.m_sLocationId);
			if (!IsAttackTerritory(target))
				continue;
			string reason = AttackWaitReason(state);
			string category = reason;
			if (reason.IsEmpty() && !executor.HasMarsOrigin_S(AROR_EFactionRole.OCCUPIERS, state.m_sLocationId))
			{
				reason = "no safe Occupier launch site";
				category = reason;
			}
			if (!reason.IsEmpty())
			{
				// Key on the kind of wait only, so a countdown alone does not log every 15 seconds
				int colon = category.IndexOf(":");
				if (colon >= 0)
					category = category.Substring(0, colon);
				waitKey += state.m_sLocationId + "=" + category + ";";
				if (reported < 4)
					waiting += state.m_sLocationId + " " + reason + "; ";
				reported++;
				continue;
			}
			if (!selected || state.m_iNextAttempt < selected.m_iNextAttempt)
				selected = state;
		}
		if (!selected)
		{
			if (reported == 0)
				HoldAttacks("none", "no Resistance-held military territory to attack");
			else
				HoldAttacks("sites:" + waitKey, string.Format("no site ready (%1 held): %2", reported, waiting));
			return;
		}
		selected.m_iNextAttempt = m_iClock + 30;
		int tier = GetCrackdownLevel(AROR_EFactionRole.OCCUPIERS);
		if (m_bQRFStrategicEconomy && !executor.CanFundQRFThreat_S(AROR_EFactionRole.OCCUPIERS, tier, false, selected.m_sLocationId))
		{
			HoldAttacks("funds:" + selected.m_sLocationId, string.Format("the Occupiers cannot fund a tier %1 attack on %2", tier, selected.m_sLocationId));
			return;
		}
		string id = ScheduleAttack_S(AROR_EFactionRole.OCCUPIERS, selected.m_sLocationId);
		AROR_Operation attack = GetOperation(id);
		if (!attack)
		{
			HoldAttacks("scheduler:" + selected.m_sLocationId, string.Format("the scheduler refused an attack on %1 (attack interval or capacity)", selected.m_sLocationId));
			return;
		}
		m_sAttackHoldKey = string.Empty;
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
		if (!location || location.GetControllingRole() != AROR_EFactionRole.RESISTANCE || location.IsControlFixed() || !AROR_CaptureLocationAction.IsCapturableType(location.GetLocationType()))
			return false;
		// A stronghold returns only to its own army: an Occupier attack on a Resistance-held Vostok could never succeed
		AROR_StrongholdComponent stronghold = AROR_StrongholdComponent.Cast(location.GetOwner().FindComponent(AROR_StrongholdComponent));
		return !stronghold || stronghold.AllowsCaptureBy(AROR_EFactionRole.OCCUPIERS);
	}

	//! Why a held site is not yet due, or empty when it is. Text before any colon names the kind of wait.
	protected string AttackWaitReason(AROR_TerritoryAttackState state)
	{
		if (state.m_iQRFCompletedAt < 0)
		{
			if (!state.m_sActiveQRF.IsEmpty())
				return "capture QRF " + state.m_sActiveQRF + " still deployed";
			return "capture QRF has not deployed yet (attacks follow a deployed capture QRF)";
		}
		int captureGrace = state.m_iCapturedAt + Math.Max(0, m_iTerritoryAttackGrace) - m_iClock;
		if (captureGrace > 0)
			return string.Format("capture grace: %1s left", captureGrace);
		int afterQRF = state.m_iQRFCompletedAt + Math.Max(0, m_iTerritoryAttackAfterQRF) - m_iClock;
		if (afterQRF > 0)
			return string.Format("recovery after capture QRF: %1s left", afterQRF);
		if (m_iClock < state.m_iNextAttempt)
			return string.Format("retry: %1s left", state.m_iNextAttempt - m_iClock);
		return string.Empty;
	}

	protected void HoldAttacks(string key, string message)
	{
		if (key == m_sAttackHoldKey)
			return;
		m_sAttackHoldKey = key;
		Print("[AROR ATTACK] no attack: " + message, LogLevel.NORMAL);
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
		return OutstandingOperation() != null;
	}

	//! The first operation still in progress (any army or type, including scripted and test responses)
	protected AROR_Operation OutstandingOperation()
	{
		foreach (AROR_Operation operation : m_aOperations)
		{
			if (!operation.IsTerminal())
				return operation;
		}
		return null;
	}

	override protected void NotifyChanged(AROR_Operation operation)
	{
		// Subscribers may synchronously resolve ACTIVE; record this transition first.
		RecordTerritoryOperationChange(operation);
		super.NotifyChanged(operation);
	}

	protected void RecordTerritoryOperationChange(AROR_Operation operation)
	{
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
