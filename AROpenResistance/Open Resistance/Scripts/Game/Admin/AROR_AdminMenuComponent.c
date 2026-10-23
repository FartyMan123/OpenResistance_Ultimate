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

	//! Support change per admin button press (percentage points)
	protected static const float SUPPORT_STEP = 10;

	protected static AROR_AdminMenuComponent s_Instance;

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
		dialog.SetMessage(GetNearestSupportText());
	}

	//------------------------------------------------------------------------------------------------
	//! Client: support of the town or city nearest to the local player (shown in the dialog)
	protected string GetNearestSupportText()
	{
		AROR_SettlementComponent support = FindNearestSupport(GetGame().GetPlayerController().GetControlledEntity());
		if (!support)
			return "No town or city nearby.";

		return string.Format("Nearest: %1\nResistance %2 | Occupiers %3 | Invaders %4",
			support.GetLocation().GetName(),
			Math.Round(support.GetSupport(AROR_EFactionRole.RESISTANCE)),
			Math.Round(support.GetSupport(AROR_EFactionRole.OCCUPIERS)),
			Math.Round(support.GetSupport(AROR_EFactionRole.INVADERS)));
	}

	//------------------------------------------------------------------------------------------------
	//! Nearest location to the entity that has support (towns and cities)
	static AROR_SettlementComponent FindNearestSupport(IEntity entity)
	{
		AROR_LocationManagerComponent locationManager = AROR_LocationManagerComponent.GetInstance();
		if (!entity || !locationManager)
			return null;

		array<AROR_LocationComponent> locations = {};
		locationManager.GetLocations(locations);

		AROR_SettlementComponent nearest;
		float nearestDistance = float.MAX;
		foreach (AROR_LocationComponent location : locations)
		{
			if (!location)
				continue;

			AROR_SettlementComponent support = AROR_SettlementComponent.Cast(location.GetOwner().FindComponent(AROR_SettlementComponent));
			if (!support)
				continue;

			float distance = vector.DistanceXZ(location.GetCenter(), entity.GetOrigin());
			if (distance < nearestDistance)
			{
				nearestDistance = distance;
				nearest = support;
			}
		}

		return nearest;
	}

	//------------------------------------------------------------------------------------------------
	//! Server: adjust support of the town or city nearest to the requesting player
	void AdjustNearestSupport_S(int playerId, AROR_EFactionRole role, float delta)
	{
		if (!Replication.IsServer() || !m_bEnabled)
			return;

		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		AROR_SettlementComponent support = FindNearestSupport(player);
		if (!support)
			return;

		support.AddSupport_S(role, delta);
		Print(string.Format("[AROR] Admin: %1 support at '%2' %3 by %4 (player %5)", typename.EnumToString(AROR_EFactionRole, role), support.GetLocation().GetName(), "changed", delta, playerId), LogLevel.NORMAL);
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
		{
			StartTeleportPicking();
			return;
		}

		AROR_EFactionRole role;
		if (tag == "support_resistance")
			role = AROR_EFactionRole.RESISTANCE;
		else if (tag == "support_occupiers")
			role = AROR_EFactionRole.OCCUPIERS;
		else if (tag == "support_invaders")
			role = AROR_EFactionRole.INVADERS;
		else
			return;

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
			playerController.AROR_RequestAdminSupport(role, SUPPORT_STEP);
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
	static AROR_AdminMenuComponent GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode())
			return;

		// The server instance handles admin requests sent through the player controller
		s_Instance = this;

		if (!m_bEnabled || RplSession.Mode() == RplMode.Dedicated)
			return;

		GetGame().GetCallqueue().CallLater(CheckHotkey, 0, true);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(CheckHotkey);
		GetGame().GetCallqueue().Remove(HandleAction);
		StopTeleportPicking();

		if (s_Instance == this)
			s_Instance = null;

		super.OnDelete(owner);
	}
}
