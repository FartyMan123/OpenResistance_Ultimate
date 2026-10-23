[ComponentEditorProps(category: "OpenResistance/Admin", description: "Admin/testing menu (Ctrl+T). Client only. Attach to the game mode entity.")]
class AROR_AdminMenuComponentClass : SCR_BaseGameModeComponentClass
{
}

//! Admin/testing menu, opened with Ctrl+T. Currently available to every player (switch off with Enabled).
//! Teleport: opens the base-game map menu; the clicked position teleports the local player there.
//! The hotkey is read directly from the keyboard to avoid overriding the base-game input config.
class AROR_AdminMenuComponent : SCR_BaseGameModeComponent
{
	[Attribute("1", desc: "Menu available (currently to every player). Turn off for public servers.")]
	protected bool m_bEnabled;

	[Attribute("{5BB4D0ED53225DDC}Configs/UI/AROR_SetupDialogs.conf", UIWidgets.ResourceNamePicker, desc: "Dialog presets (uses the 'aror_admin' preset)", params: "conf")]
	protected ResourceName m_sDialogPresets;

	protected static const string PRESET_ADMIN = "aror_admin";

	protected bool m_bPickingTeleport;

	//------------------------------------------------------------------------------------------------
	//! Polled every frame on the client while enabled
	protected void CheckHotkey()
	{
		if (!Debug.KeyState(KeyCode.KC_T))
			return;

		if (!Debug.KeyState(KeyCode.KC_LCONTROL) && !Debug.KeyState(KeyCode.KC_RCONTROL))
			return;

		Debug.ClearKey(KeyCode.KC_T);

		// Not while another dialog is open or a position is being picked
		if (m_bPickingTeleport || SCR_ConfigurableDialogUi.GetCurrentDialog())
			return;

		OpenDialog();
	}

	//------------------------------------------------------------------------------------------------
	protected void OpenDialog()
	{
		AROR_AdminDialog dialog = new AROR_AdminDialog(this);
		SCR_ConfigurableDialogUi.CreateFromPreset(m_sDialogPresets, PRESET_ADMIN, dialog);
	}

	//------------------------------------------------------------------------------------------------
	//! Called by the dialog once it has closed; handled next frame, outside the menu manager's close sequence
	void QueueAction(string tag)
	{
		GetGame().GetCallqueue().CallLater(HandleAction, 0, false, tag);
	}

	//------------------------------------------------------------------------------------------------
	protected void HandleAction(string tag)
	{
		if (tag == "teleport")
			StartTeleportPicking();
	}

	//------------------------------------------------------------------------------------------------
	// Teleport
	//------------------------------------------------------------------------------------------------
	protected void StartTeleportPicking()
	{
		if (m_bPickingTeleport)
			return;

		m_bPickingTeleport = true;
		SCR_MapEntity.GetOnSelection().Insert(OnMapSelection);
		SCR_MapEntity.GetOnMapOpen().Insert(OnMapOpened);
		SCR_MapEntity.GetOnMapClose().Insert(OnMapClosed);
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.MapMenu);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnMapOpened(MapConfiguration config)
	{
		SCR_MapEntity.GetOnMapOpen().Remove(OnMapOpened);

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (mapEntity)
			mapEntity.ZoomOut();
	}

	//------------------------------------------------------------------------------------------------
	//! The selection event gives screen coordinates; read the world position under the map cursor
	protected void OnMapSelection(vector screenPosition)
	{
		if (!m_bPickingTeleport)
			return;

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity)
			return;

		float worldX, worldZ;
		mapEntity.GetMapCursorWorldPosition(worldX, worldZ);
		vector target = Vector(worldX, GetGame().GetWorld().GetSurfaceY(worldX, worldZ), worldZ);

		StopTeleportPicking();
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.MapMenu);

		// The owning client moves its own character; movement replicates to the server
		if (!SCR_Global.TeleportLocalPlayer(target))
			Print(string.Format("[AROR] Teleport to %1 failed (not spawned?)", target), LogLevel.WARNING);
	}

	//------------------------------------------------------------------------------------------------
	//! Closing the map without clicking cancels the teleport
	protected void OnMapClosed(MapConfiguration config)
	{
		StopTeleportPicking();
	}

	//------------------------------------------------------------------------------------------------
	protected void StopTeleportPicking()
	{
		m_bPickingTeleport = false;
		SCR_MapEntity.GetOnSelection().Remove(OnMapSelection);
		SCR_MapEntity.GetOnMapOpen().Remove(OnMapOpened);
		SCR_MapEntity.GetOnMapClose().Remove(OnMapClosed);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!m_bEnabled || !GetGame().InPlayMode() || RplSession.Mode() == RplMode.Dedicated)
			return;

		GetGame().GetCallqueue().CallLater(CheckHotkey, 0, true);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(CheckHotkey);
		GetGame().GetCallqueue().Remove(HandleAction);
		StopTeleportPicking();

		super.OnDelete(owner);
	}
}
