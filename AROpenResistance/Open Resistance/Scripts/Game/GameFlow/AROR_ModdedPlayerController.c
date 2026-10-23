//! Network bridge for the game flow. Clients can only send RPCs through entities they own,
//! so setup requests and replies travel through the player's controller and are handled by AROR_GameFlowComponent.
modded class SCR_PlayerController
{
	//! Separator for the save list, sent as a single string
	protected static const string AROR_SAVE_LIST_SEPARATOR = "\n";

	//------------------------------------------------------------------------------------------------
	// Client -> server
	//------------------------------------------------------------------------------------------------
	void AROR_RequestNewPlaythrough(vector hqPosition, string playthroughName)
	{
		if (Replication.IsServer())
			AROR_RpcAsk_NewPlaythrough(hqPosition, playthroughName);
		else
			Rpc(AROR_RpcAsk_NewPlaythrough, hqPosition, playthroughName);
	}

	//------------------------------------------------------------------------------------------------
	void AROR_RequestSaveList()
	{
		if (Replication.IsServer())
			AROR_RpcAsk_SaveList();
		else
			Rpc(AROR_RpcAsk_SaveList);
	}

	//------------------------------------------------------------------------------------------------
	void AROR_RequestLoad(int saveIndex)
	{
		if (Replication.IsServer())
			AROR_RpcAsk_Load(saveIndex);
		else
			Rpc(AROR_RpcAsk_Load, saveIndex);
	}

	//------------------------------------------------------------------------------------------------
	//! Admin menu: change support of the town or city nearest to this player
	void AROR_RequestAdminSupport(AROR_EFactionRole role, float delta)
	{
		if (Replication.IsServer())
			AROR_RpcAsk_AdminSupport(role, delta);
		else
			Rpc(AROR_RpcAsk_AdminSupport, role, delta);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void AROR_RpcAsk_AdminSupport(int role, float delta)
	{
		AROR_AdminMenuComponent adminMenu = AROR_AdminMenuComponent.GetInstance();
		if (adminMenu)
			adminMenu.AdjustNearestSupport_S(GetPlayerId(), role, delta);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void AROR_RpcAsk_NewPlaythrough(vector hqPosition, string playthroughName)
	{
		AROR_GameFlowComponent gameFlow = AROR_GameFlowComponent.GetInstance();
		if (gameFlow)
			gameFlow.RequestNewPlaythrough_S(GetPlayerId(), hqPosition, playthroughName);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void AROR_RpcAsk_SaveList()
	{
		AROR_GameFlowComponent gameFlow = AROR_GameFlowComponent.GetInstance();
		if (gameFlow)
			gameFlow.RequestSaveList_S(GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void AROR_RpcAsk_Load(int saveIndex)
	{
		AROR_GameFlowComponent gameFlow = AROR_GameFlowComponent.GetInstance();
		if (gameFlow)
			gameFlow.RequestLoad_S(GetPlayerId(), saveIndex);
	}

	//------------------------------------------------------------------------------------------------
	// Server -> owning client
	//------------------------------------------------------------------------------------------------
	void AROR_OpenSetup_S()
	{
		if (AROR_IsLocal())
			AROR_RpcDo_OpenSetup();
		else
			Rpc(AROR_RpcDo_OpenSetup);
	}

	//------------------------------------------------------------------------------------------------
	void AROR_SendSaveList_S(notnull array<string> saveLabels)
	{
		string joined;
		foreach (int i, string label : saveLabels)
		{
			if (i > 0)
				joined += AROR_SAVE_LIST_SEPARATOR;

			joined += label;
		}

		if (AROR_IsLocal())
			AROR_RpcDo_SaveList(joined);
		else
			Rpc(AROR_RpcDo_SaveList, joined);
	}

	//------------------------------------------------------------------------------------------------
	void AROR_SendSetupResult_S(bool success, string message)
	{
		if (AROR_IsLocal())
			AROR_RpcDo_SetupResult(success, message);
		else
			Rpc(AROR_RpcDo_SetupResult, success, message);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void AROR_RpcDo_OpenSetup()
	{
		AROR_GameFlowComponent gameFlow = AROR_GameFlowComponent.GetInstance();
		if (gameFlow)
			gameFlow.OnSetupRequestedReceived();
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void AROR_RpcDo_SaveList(string joinedLabels)
	{
		array<string> saveLabels = {};
		if (!joinedLabels.IsEmpty())
			joinedLabels.Split(AROR_SAVE_LIST_SEPARATOR, saveLabels, false);

		AROR_GameFlowComponent gameFlow = AROR_GameFlowComponent.GetInstance();
		if (gameFlow)
			gameFlow.OnSaveListReceived(saveLabels);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void AROR_RpcDo_SetupResult(bool success, string message)
	{
		AROR_GameFlowComponent gameFlow = AROR_GameFlowComponent.GetInstance();
		if (gameFlow)
			gameFlow.OnSetupResultReceived(success, message);
	}

	//------------------------------------------------------------------------------------------------
	//! True if this controller belongs to the local player (e.g. the host or Workbench player)
	protected bool AROR_IsLocal()
	{
		return GetGame().GetPlayerController() == this;
	}
}
