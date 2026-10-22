[ComponentEditorProps(category: "OpenResistance/GameFlow", description: "Admin setup UI (new / load campaign). Client only. Attach to the game mode entity next to AROR_GameFlowComponent.")]
class AROR_SetupUIComponentClass : SCR_BaseGameModeComponentClass
{
}

//! Client-side setup UI for the server admin (see ARCHITECTURE.md -> Game Flow -> Setup UI).
//! Reacts to AROR_GameFlowComponent client events and sends requests through the AROR_ methods on SCR_PlayerController.
//! New: opens the base-game map menu, the admin clicks the HQ position, a dialog confirms it.
//! Load: a dialog steps through the save list sent by the server.
class AROR_SetupUIComponent : SCR_BaseGameModeComponent
{
	[Attribute("{5BB4D0ED53225DDC}Configs/UI/AROR_SetupDialogs.conf", UIWidgets.ResourceNamePicker, desc: "Setup dialog presets", params: "conf")]
	protected ResourceName m_sDialogPresets;

	[Attribute("Open Resistance", desc: "Name given to new playthroughs (until a name field exists)")]
	protected string m_sDefaultPlaythroughName;

	protected static const string PRESET_MAIN = "aror_setup_main";
	protected static const string PRESET_HQ_CONFIRM = "aror_setup_hq_confirm";
	protected static const string PRESET_LOAD = "aror_setup_load";

	//! Delay between closing a dialog and opening the next menu
	protected static const int MENU_SWITCH_DELAY_MS = 250;

	protected AROR_GameFlowComponent m_GameFlow;
	protected bool m_bPickingHQ;
	protected vector m_vPickedPosition;

	//------------------------------------------------------------------------------------------------
	void OpenMainDialog()
	{
		OpenMainDialogWithMessage(string.Empty);
	}

	//------------------------------------------------------------------------------------------------
	void OpenMainDialogWithMessage(string message)
	{
		AROR_SetupMainDialog dialog = new AROR_SetupMainDialog(this);
		SCR_ConfigurableDialogUi.CreateFromPreset(m_sDialogPresets, PRESET_MAIN, dialog);

		if (!message.IsEmpty())
			dialog.SetMessage(message);
	}

	//------------------------------------------------------------------------------------------------
	// New campaign: pick the HQ on the map
	//------------------------------------------------------------------------------------------------
	void StartHQPicking()
	{
		// Dialog closing is queued by the menu manager; open the map once it has closed
		GetGame().GetCallqueue().CallLater(OpenHQMap, MENU_SWITCH_DELAY_MS);
	}

	//------------------------------------------------------------------------------------------------
	protected void OpenHQMap()
	{
		m_bPickingHQ = true;
		SCR_MapEntity.GetOnSelection().Insert(OnMapSelection);
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.MapMenu);
	}

	//------------------------------------------------------------------------------------------------
	void CancelHQPicking()
	{
		StopHQPicking();
		OpenMainDialog();
	}

	//------------------------------------------------------------------------------------------------
	void ConfirmHQ()
	{
		StopHQPicking();

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
			playerController.AROR_RequestNewPlaythrough(m_vPickedPosition, m_sDefaultPlaythroughName);
	}

	//------------------------------------------------------------------------------------------------
	protected void StopHQPicking()
	{
		m_bPickingHQ = false;
		SCR_MapEntity.GetOnSelection().Remove(OnMapSelection);
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.MapMenu);
	}

	//------------------------------------------------------------------------------------------------
	//! The selection event gives screen coordinates, so read the world position under the map cursor instead
	protected void OnMapSelection(vector screenPosition)
	{
		if (!m_bPickingHQ || !m_GameFlow)
			return;

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity)
			return;

		float worldX, worldZ;
		mapEntity.GetMapCursorWorldPosition(worldX, worldZ);
		vector position = Vector(worldX, 0, worldZ);

		// TEMP: verify the picked world position; remove once confirmed
		Print(string.Format("[AROR] Map selection: screen %1 -> world %2", screenPosition, position), LogLevel.NORMAL);

		m_vPickedPosition = position;

		string reason;
		bool valid = m_GameFlow.IsValidHQPosition(position, reason);

		AROR_SetupHQConfirmDialog dialog = new AROR_SetupHQConfirmDialog(this);
		SCR_ConfigurableDialogUi.CreateFromPreset(m_sDialogPresets, PRESET_HQ_CONFIRM, dialog);

		if (valid)
			dialog.SetPlacementValid(true, "Place the Resistance HQ here?");
		else
			dialog.SetPlacementValid(false, reason);
	}

	//------------------------------------------------------------------------------------------------
	// Load campaign
	//------------------------------------------------------------------------------------------------
	void RequestSaveList()
	{
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
			playerController.AROR_RequestSaveList();
	}

	//------------------------------------------------------------------------------------------------
	void RequestLoad(int saveIndex)
	{
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
			playerController.AROR_RequestLoad(saveIndex);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSaveListReceived(array<string> saveLabels)
	{
		AROR_SetupLoadDialog dialog = new AROR_SetupLoadDialog(this, saveLabels);
		SCR_ConfigurableDialogUi.CreateFromPreset(m_sDialogPresets, PRESET_LOAD, dialog);
		dialog.Refresh();
	}

	//------------------------------------------------------------------------------------------------
	//! Server rejected a request (e.g. invalid HQ position): back to the main dialog with the reason
	protected void OnSetupResult(bool success, string message)
	{
		if (!success)
			OpenMainDialogWithMessage(message);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode() || RplSession.Mode() == RplMode.Dedicated)
			return;

		m_GameFlow = AROR_GameFlowComponent.Cast(owner.FindComponent(AROR_GameFlowComponent));
		if (!m_GameFlow)
		{
			Print("[AROR] AROR_SetupUIComponent needs AROR_GameFlowComponent on the same entity", LogLevel.ERROR);
			return;
		}

		m_GameFlow.GetOnSetupRequested().Insert(OpenMainDialog);
		m_GameFlow.GetOnSaveListReceived().Insert(OnSaveListReceived);
		m_GameFlow.GetOnSetupResult().Insert(OnSetupResult);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(OpenHQMap);
		SCR_MapEntity.GetOnSelection().Remove(OnMapSelection);

		if (m_GameFlow)
		{
			m_GameFlow.GetOnSetupRequested().Remove(OpenMainDialog);
			m_GameFlow.GetOnSaveListReceived().Remove(OnSaveListReceived);
			m_GameFlow.GetOnSetupResult().Remove(OnSetupResult);
		}

		super.OnDelete(owner);
	}
}
