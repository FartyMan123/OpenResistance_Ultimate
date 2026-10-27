modded class AROR_OperationManagerComponent
{
	[Attribute("0", desc: "Enable the MARS phone debugger. Requests require the entry credential, an active campaign, Rebel target territory and a friendly military launch origin. Disable outside test scenarios.")]
	protected bool m_bMarsEnabled;
	protected ref map<int, int> m_mMarsNextRequest = new map<int, int>();
	[RplProp()]
	protected string m_sMarsStatus;

	string GetMarsStatus()
	{
		return m_sMarsStatus;
	}

	override protected void NotifyChanged(AROR_Operation operation)
	{
		super.NotifyChanged(operation);
		if (Replication.IsServer() && operation && operation.m_bDebugRequest)
		{
			string state = typename.EnumToString(AROR_EOperationState, operation.GetState());
			m_sMarsStatus = string.Format("%1 | target %2 | %3 | %4", operation.GetId(), operation.GetTargetLocationId(), state, operation.GetResult());
			Replication.BumpMe();
		}
	}

	//! Password checking is repeated here even if a caller bypasses the phone UI.
	void RequestMars_S(int playerId, string credential, AROR_EFactionRole role, string selection, int tier, int readinessLevel = 0)
	{
		if (!Replication.IsServer() || !m_bMarsEnabled || !IsReady() || playerId <= 0)
			return;
		int next;
		m_mMarsNextRequest.Find(playerId, next);
		if (m_iClock < next)
			return;
		m_mMarsNextRequest.Set(playerId, m_iClock + 5);
		if (!AROR_MarsAccess.Accepts(credential))
		{
			MarsReply(playerId, "Access denied");
			return;
		}
		if (!IsEnemyRole(role) || tier < 1 || tier > 5 || selection.Length() > 128)
			return;
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!AROR_QRFSpotting.IsResistance(player))
		{
			MarsReply(playerId, "A living player is required");
			return;
		}
		AROR_LocationManagerComponent locations = AROR_LocationManagerComponent.GetInstance();
		AROR_OperationExecutorComponent executor = AROR_OperationExecutorComponent.Cast(GetOwner().FindComponent(AROR_OperationExecutorComponent));
		if (!locations || !executor)
			return;
		AROR_LocationComponent target = AROR_MarsTargets.NearestRebelTerritory(locations, player.GetOrigin());
		if (!target)
		{
			MarsReply(playerId, "No Rebel-owned territory is available");
			return;
		}
		array<ref AROR_MarsEntry> entries = {};
		executor.GetMarsEntries(role, tier, entries);
		AROR_MarsEntry selected;
		bool useMetrics = selection.IndexOf("type:") == 0;
		if (useMetrics)
		{
			int type = selection.Substring(5, selection.Length() - 5).ToInt();
			if (selection != "type:" + type.ToString() || readinessLevel < 1 || readinessLevel > 5)
				return;
			AROR_ResponseSelection preview = executor.GetResponsePreview(role, type, readinessLevel, tier);
			if (!preview.IsAvailable())
			{
				MarsReply(playerId, preview.m_sReason);
				return;
			}
			selected = new AROR_MarsEntry();
			selected.m_sProfileId = preview.m_sProfileId;
			selected.m_sRecipeId = preview.m_sRecipeId;
			selected.m_iTier = tier;
			selected.m_sLabel = AROR_ResponsePolicy.TypeName(type) + " / R" + readinessLevel + " C" + tier;
		}
		else
		{
			foreach (AROR_MarsEntry entry : entries)
			{
				if (entry.m_sKey == selection)
					selected = entry;
			}
		}
		if (!selected)
		{
			MarsReply(playerId, "This selection is not available for that army");
			return;
		}
		if (!executor.HasMarsOrigin_S(role, target.GetId()))
		{
			MarsReply(playerId, "No safe military launch territory is owned by that army");
			return;
		}
		foreach (AROR_Operation existing : m_aOperations)
		{
			if (!existing.IsTerminal() && existing.GetRole() == role && existing.GetTargetLocationId() == target.GetId())
			{
				MarsReply(playerId, "An operation already targets " + target.GetName());
				return;
			}
		}
		string id = RequestOperation_S(role, AROR_EOperationType.QRF, target.GetId(), 0, true);
		AROR_Operation operation = GetOperation(id);
		if (!operation)
		{
			MarsReply(playerId, "Scheduler capacity or army availability blocks this request");
			return;
		}
		operation.m_sRequestedProfileId = selected.m_sProfileId;
		operation.m_sRequestedRecipeId = selected.m_sRecipeId;
		operation.m_iRequestedTier = selected.m_iTier;
		operation.m_bUseResponseMetrics = useMetrics;
		if (useMetrics)
			operation.m_iPoolReadinessLevel = readinessLevel;
		MarsReply(playerId, selected.m_sLabel + " queued for " + target.GetName() + " (" + id + ")");
	}

	protected void MarsReply(int playerId, string message)
	{
		AROR_NotificationManagerComponent.SendHint_S(playerId, "MARS: " + message);
	}
}
