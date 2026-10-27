[ComponentEditorProps(category: "OpenResistance/GameFlow", description: "Admin setup UI (new / load campaign). Client only. Attach to the game mode entity next to AROR_GameFlowComponent.")]
class AROR_SetupUIComponentClass : SCR_BaseGameModeComponentClass
{
}

//! Client-side setup UI for the server admin (see ARCHITECTURE.md -> Game Flow -> Setup UI).
//! Reacts to AROR_GameFlowComponent client events and sends requests through the AROR_ methods on SCR_PlayerController.
//! New: opens the base-game map menu, the admin clicks the HQ position, a dialog confirms it.
//! Start and Load: the setup menu (AROR_SetupMenu) shows the options and the save list sent by the server.
//! Dialog button actions arrive through QueueDialogAction once the dialog has closed (see AROR_SetupDialogs.c).
class AROR_SetupUIComponent : SCR_BaseGameModeComponent
{
	[Attribute("{5BB4D0ED53225DDC}Configs/UI/AROR_SetupDialogs.conf", UIWidgets.ResourceNamePicker, desc: "Setup dialog presets", params: "conf")]
	protected ResourceName m_sDialogPresets;

	[Attribute("Open Resistance", desc: "Name given to new playthroughs (until a name field exists)")]
	protected string m_sDefaultPlaythroughName;

	static const string DIALOG_MAIN = "main";
	static const string DIALOG_FACTIONS = "factions";
	static const string DIALOG_HQ_CONFIRM = "hq_confirm";
	static const string DIALOG_LOAD = "load";

	protected static const string PRESET_FACTIONS = "aror_setup_factions";
	protected static const string PRESET_HQ_CONFIRM = "aror_setup_hq_confirm";

	protected AROR_GameFlowComponent m_GameFlow;
	protected bool m_bPickingHQ;

	//! New campaign: faction preset ID chosen per role, indexed by AROR_EFactionRole
	protected ref array<string> m_aChosenPresetIds = {};

	// Base-game presets used by the "Quick start" testing button
	protected static const string BASE_PRESET_RESISTANCE = "FIA";
	protected static const string BASE_PRESET_OCCUPIERS = "US_ARMY";
	protected static const string BASE_PRESET_INVADERS = "USSR_ARMY";
	protected static const string BASE_PRESET_CIVILIANS = "EVERON_CIV";

	protected static AROR_SetupUIComponent s_Instance;

	// State the setup menu shows (it reads it when it opens): screen, save list and any message from the server
	protected int m_iMenuScreen = AROR_SetupMenu.SCREEN_START;
	protected ref array<string> m_aSaveLabels = {};
	protected string m_sMenuMessage;
	protected vector m_vPickedPosition;

	//------------------------------------------------------------------------------------------------
	static AROR_SetupUIComponent GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	//! Which setup menu screen to show (AROR_SetupMenu.SCREEN_*)
	int GetMenuScreen()
	{
		return m_iMenuScreen;
	}

	//------------------------------------------------------------------------------------------------
	array<string> GetSaveLabels()
	{
		return m_aSaveLabels;
	}

	//------------------------------------------------------------------------------------------------
	//! The last message from the server for the start screen (e.g. why a request was rejected)
	string GetMenuMessage()
	{
		return m_sMenuMessage;
	}

	//------------------------------------------------------------------------------------------------
	void OpenMainDialog()
	{
		OpenMainDialogWithMessage(string.Empty);
	}

	//------------------------------------------------------------------------------------------------
	void OpenMainDialogWithMessage(string message)
	{
		// The setup menu replaces the base-game main dialog; one instance, updated in place when open
		m_iMenuScreen = AROR_SetupMenu.SCREEN_START;
		m_sMenuMessage = message;
		AROR_SetupMenu menu = AROR_SetupMenu.GetOpen();
		if (menu)
			menu.ShowStart(message);
		else
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.AROR_SetupMenu);
	}

	//------------------------------------------------------------------------------------------------
	// Dialog actions
	//------------------------------------------------------------------------------------------------
	//! Called by a dialog once it has closed; handled next frame, outside the menu manager's close sequence
	void QueueDialogAction(string dialogId, string tag, int value)
	{
		GetGame().GetCallqueue().CallLater(HandleDialogAction, 0, false, dialogId, tag, value);
	}

	//------------------------------------------------------------------------------------------------
	protected void HandleDialogAction(string dialogId, string tag, int value)
	{
		if (dialogId == DIALOG_MAIN)
		{
			if (tag == "new")
				OpenFactionsDialog();
			else if (tag == "quick")
				QuickStart();
			else if (tag == "load")
				RequestSaveList();
		}
		else if (dialogId == DIALOG_FACTIONS)
		{
			if (tag == "confirm")
				StartHQPicking();
			else if (tag == "back")
				OpenMainDialog();
		}
		else if (dialogId == DIALOG_HQ_CONFIRM)
		{
			if (tag == "start")
				ConfirmHQ();
			else if (tag == "back")
				CancelHQPicking();
			// "repick": nothing, the map stays open
		}
		else if (dialogId == DIALOG_LOAD)
		{
			if (tag == "load")
				RequestLoad(value);
			else if (tag == "back")
				OpenMainDialog();
		}
	}

	//------------------------------------------------------------------------------------------------
	// New campaign: choose faction presets, then pick the HQ on the map
	//------------------------------------------------------------------------------------------------
	protected void OpenFactionsDialog()
	{
		m_aChosenPresetIds.Clear();

		AROR_SetupFactionsDialog dialog = new AROR_SetupFactionsDialog(this);
		SCR_ConfigurableDialogUi.CreateFromPreset(m_sDialogPresets, PRESET_FACTIONS, dialog);
		dialog.ShowRole();
	}

	//------------------------------------------------------------------------------------------------
	//! Testing shortcut: skip the Factions step with the base-game presets selected, then pick the HQ
	protected void QuickStart()
	{
		m_aChosenPresetIds.Clear();
		SetChosenPresetId(AROR_EFactionRole.RESISTANCE, BASE_PRESET_RESISTANCE);
		SetChosenPresetId(AROR_EFactionRole.OCCUPIERS, BASE_PRESET_OCCUPIERS);
		SetChosenPresetId(AROR_EFactionRole.INVADERS, BASE_PRESET_INVADERS);
		SetChosenPresetId(AROR_EFactionRole.CIVILIANS, BASE_PRESET_CIVILIANS);
		StartHQPicking();
	}

	//------------------------------------------------------------------------------------------------
	string GetChosenPresetId(AROR_EFactionRole role)
	{
		if (role < m_aChosenPresetIds.Count())
			return m_aChosenPresetIds[role];

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	void SetChosenPresetId(AROR_EFactionRole role, string presetId)
	{
		while (m_aChosenPresetIds.Count() <= role)
		{
			m_aChosenPresetIds.Insert(string.Empty);
		}

		m_aChosenPresetIds[role] = presetId;
	}

	//------------------------------------------------------------------------------------------------
	protected void StartHQPicking()
	{
		if (m_bPickingHQ)
			return;

		m_bPickingHQ = true;
		AROR_LocationMapIconsComponent.SetInfoPopupSuppressed(true);
		SCR_MapEntity.GetOnSelection().Insert(OnMapSelection);
		SCR_MapEntity.GetOnMapOpen().Insert(OnHQMapOpened);
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.MapMenu);
	}

	//------------------------------------------------------------------------------------------------
	//! Show the whole map when picking the HQ
	protected void OnHQMapOpened(MapConfiguration config)
	{
		SCR_MapEntity.GetOnMapOpen().Remove(OnHQMapOpened);

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (mapEntity)
			mapEntity.ZoomOut();
	}

	//------------------------------------------------------------------------------------------------
	protected void CancelHQPicking()
	{
		StopHQPicking();
		OpenMainDialog();
	}

	//------------------------------------------------------------------------------------------------
	protected void ConfirmHQ()
	{
		StopHQPicking();

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
			playerController.AROR_RequestNewPlaythrough(m_vPickedPosition, m_sDefaultPlaythroughName, m_aChosenPresetIds);
	}

	//------------------------------------------------------------------------------------------------
	protected void StopHQPicking()
	{
		m_bPickingHQ = false;
		AROR_LocationMapIconsComponent.SetInfoPopupSuppressed(false);
		SCR_MapEntity.GetOnSelection().Remove(OnMapSelection);
		SCR_MapEntity.GetOnMapOpen().Remove(OnHQMapOpened);
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.MapMenu);
	}

	//------------------------------------------------------------------------------------------------
	//! The selection event gives screen coordinates, so read the world position under the map cursor instead
	protected void OnMapSelection(vector screenPosition)
	{
		if (!m_bPickingHQ || !m_GameFlow)
			return;

		// One confirm dialog at a time
		if (SCR_ConfigurableDialogUi.GetCurrentDialogTag() == PRESET_HQ_CONFIRM && SCR_ConfigurableDialogUi.GetCurrentDialog())
			return;

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity)
			return;

		float worldX, worldZ;
		mapEntity.GetMapCursorWorldPosition(worldX, worldZ);
		m_vPickedPosition = Vector(worldX, 0, worldZ);

		string reason;
		bool valid = m_GameFlow.IsValidHQPosition(m_vPickedPosition, reason);

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
	//! Asks the server for the save list; the answer arrives in OnSaveListReceived
	void RequestSaveList()
	{
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
			playerController.AROR_RequestSaveList();
	}

	//------------------------------------------------------------------------------------------------
	protected void RequestLoad(int saveIndex)
	{
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
			playerController.AROR_RequestLoad(saveIndex);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSaveListReceived(array<string> saveLabels)
	{
		m_aSaveLabels.Copy(saveLabels);
		m_iMenuScreen = AROR_SetupMenu.SCREEN_LOAD;
		AROR_SetupMenu menu = AROR_SetupMenu.GetOpen();
		if (menu)
			menu.ShowLoad(m_aSaveLabels);
		else
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.AROR_SetupMenu);
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

		s_Instance = this;

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
		if (s_Instance == this)
			s_Instance = null;
		GetGame().GetCallqueue().Remove(HandleDialogAction);
		SCR_MapEntity.GetOnSelection().Remove(OnMapSelection);
		SCR_MapEntity.GetOnMapOpen().Remove(OnHQMapOpened);

		if (m_GameFlow)
		{
			m_GameFlow.GetOnSetupRequested().Remove(OpenMainDialog);
			m_GameFlow.GetOnSaveListReceived().Remove(OnSaveListReceived);
			m_GameFlow.GetOnSetupResult().Remove(OnSetupResult);
		}

		super.OnDelete(owner);
	}
}
