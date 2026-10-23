void AROR_OnCommanderEvaluation(AROR_Commander commander);
typedef func AROR_OnCommanderEvaluation;
typedef ScriptInvokerBase<AROR_OnCommanderEvaluation> AROR_OnCommanderEvaluationInvoker;

[ComponentEditorProps(category: "OpenResistance/Commanders", description: "Server-owned Resistance, Occupier and Invader command contexts. Attach once to the game mode.")]
class AROR_CommanderManagerComponentClass : SCR_BaseGameModeComponentClass
{
}

class AROR_CommanderManagerComponent : SCR_BaseGameModeComponent
{
	[Attribute("FIA", desc: "Resistance faction key. Must match the scenario faction and player/HQ configuration.")]
	protected string m_sResistanceFactionKey;

	[Attribute("US", desc: "Occupier faction key. Campaign logic uses the OCCUPIERS role regardless of this faction.")]
	protected string m_sOccupiersFactionKey;

	[Attribute("USSR", desc: "Invader faction key. Campaign logic uses the INVADERS role regardless of this faction.")]
	protected string m_sInvadersFactionKey;

	[Attribute("30", UIWidgets.Slider, desc: "Seconds between enemy strategic evaluations after campaign setup", params: "1 600 1")]
	protected float m_fEvaluationInterval;

	protected static AROR_CommanderManagerComponent s_Instance;
	protected ref AROR_Commander m_ResistanceCommander;
	protected ref AROR_Commander m_OccupiersCommander;
	protected ref AROR_Commander m_InvadersCommander;
	protected ref AROR_OnCommanderEvaluationInvoker m_OnCommanderEvaluation = new AROR_OnCommanderEvaluationInvoker();
	protected bool m_bCampaignActive;

	//! Server only; clients do not create strategic command contexts.
	static AROR_CommanderManagerComponent GetInstance()
	{
		return s_Instance;
	}

	AROR_Commander GetCommander(AROR_EFactionRole role)
	{
		switch (role)
		{
			case AROR_EFactionRole.RESISTANCE:
				return m_ResistanceCommander;
			case AROR_EFactionRole.OCCUPIERS:
				return m_OccupiersCommander;
			case AROR_EFactionRole.INVADERS:
				return m_InvadersCommander;
		}

		return null;
	}

	//! Server subscribers implement strategy here. No subscriber means the commander stays idle.
	//! Receives only the two AI commanders, never the player-directed Resistance.
	AROR_OnCommanderEvaluationInvoker GetOnCommanderEvaluation()
	{
		return m_OnCommanderEvaluation;
	}

	bool IsCampaignActive()
	{
		return m_bCampaignActive;
	}

	protected bool IsConfigurationValid()
	{
		if (m_sResistanceFactionKey.IsEmpty() || m_sOccupiersFactionKey.IsEmpty() || m_sInvadersFactionKey.IsEmpty())
		{
			Print("[AROR] Commander faction keys must not be empty; commanders disabled", LogLevel.ERROR);
			return false;
		}

		if (m_sResistanceFactionKey == m_sOccupiersFactionKey || m_sResistanceFactionKey == m_sInvadersFactionKey || m_sOccupiersFactionKey == m_sInvadersFactionKey)
		{
			Print("[AROR] Each commander must use a different faction key; commanders disabled", LogLevel.ERROR);
			return false;
		}

		if (m_fEvaluationInterval < 1)
		{
			Print("[AROR] Commander evaluation interval must be at least one second; commanders disabled", LogLevel.ERROR);
			return false;
		}

		return true;
	}

	//! Temporary read-only integration with the existing game-flow gate.
	//! Replace with campaign-ready lifecycle events when those are provided by Game Flow.
	protected bool IsPlaythroughReady()
	{
		AROR_GameFlowComponent gameFlow = AROR_GameFlowComponent.GetInstance();
		return gameFlow && gameFlow.CanAdvanceState(SCR_EGameModeState.GAME);
	}

	protected void CheckCampaignReady()
	{
		if (!Replication.IsServer() || !IsPlaythroughReady())
			return;

		m_bCampaignActive = true;
		GetGame().GetCallqueue().Remove(CheckCampaignReady);
		int intervalMs = m_fEvaluationInterval * 1000;
		GetGame().GetCallqueue().CallLater(EvaluateCommanders, intervalMs, true);
		Print("[AROR] Commanders active: Resistance player-directed; Occupiers and Invaders AI-directed", LogLevel.NORMAL);
	}

	protected void EvaluateCommanders()
	{
		if (!Replication.IsServer() || !m_bCampaignActive || !IsPlaythroughReady())
			return;

		// Decision hooks only. Operations, spawning and spending belong to their respective systems.
		m_OnCommanderEvaluation.Invoke(m_OccupiersCommander);
		m_OnCommanderEvaluation.Invoke(m_InvadersCommander);
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;

		if (s_Instance)
		{
			Print("[AROR] Only one commander manager may run; duplicate ignored", LogLevel.ERROR);
			return;
		}

		if (!IsConfigurationValid())
			return;

		m_ResistanceCommander = new AROR_Commander(AROR_EFactionRole.RESISTANCE, m_sResistanceFactionKey, true);
		m_OccupiersCommander = new AROR_Commander(AROR_EFactionRole.OCCUPIERS, m_sOccupiersFactionKey, false);
		m_InvadersCommander = new AROR_Commander(AROR_EFactionRole.INVADERS, m_sInvadersFactionKey, false);
		s_Instance = this;

		Print(string.Format("[AROR] Commanders initialised: Resistance=%1, Occupiers=%2, Invaders=%3; waiting for campaign", m_sResistanceFactionKey, m_sOccupiersFactionKey, m_sInvadersFactionKey), LogLevel.NORMAL);
		GetGame().GetCallqueue().CallLater(CheckCampaignReady, 1000, true);
	}

	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(CheckCampaignReady);
		GetGame().GetCallqueue().Remove(EvaluateCommanders);
		m_bCampaignActive = false;
		if (s_Instance == this)
			s_Instance = null;

		super.OnDelete(owner);
	}
}
