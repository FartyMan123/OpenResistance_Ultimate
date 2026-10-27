//! Client-only ghost placement for garage retrieve (GM-like: move, rotate, place/cancel).
//! Selection already happened in the browse dialog — no build catalogue.
class AROR_GaragePlacementController
{
	protected static bool s_bActive;
	protected static int s_iEntryId = -1;
	protected static ResourceName s_sPrefab;
	protected static IEntity s_Anchor;
	protected static RplId s_AnchorRplId;
	protected static IEntity s_Ghost;
	protected static float s_fYaw;
	protected static bool s_bValid;
	protected static vector s_vPlacePos;
	protected static bool s_bPlaceHeld;
	protected static bool s_bCancelHeld;
	protected static bool s_bWasValid;

	//------------------------------------------------------------------------------------------------
	static bool IsActive()
	{
		return s_bActive;
	}

	//------------------------------------------------------------------------------------------------
	static void Start(int entryId, ResourceName prefab, IEntity anchor, RplId anchorRplId)
	{
		if (s_bActive)
			Stop();

		if (entryId <= 0 || prefab.IsEmpty() || !anchor)
			return;

		s_bActive = true;
		s_iEntryId = entryId;
		s_sPrefab = prefab;
		s_Anchor = anchor;
		s_AnchorRplId = anchorRplId;
		s_fYaw = 0;
		s_bValid = false;
		s_bWasValid = false;
		s_bPlaceHeld = false;
		s_bCancelHeld = false;

		SpawnGhost();
		AROR_NotificationManagerComponent.ShowHint("LMB place · RMB/Esc cancel · Q/E or Shift+Wheel rotate");
		GetGame().GetCallqueue().CallLater(Tick, 0, true);
	}

	//------------------------------------------------------------------------------------------------
	static void Stop()
	{
		GetGame().GetCallqueue().Remove(Tick);
		DeleteGhost();
		s_bActive = false;
		s_iEntryId = -1;
		ResourceName emptyPrefab;
		s_sPrefab = emptyPrefab;
		s_Anchor = null;
	}

	//------------------------------------------------------------------------------------------------
	protected static void SpawnGhost()
	{
		DeleteGhost();

		Resource resource = Resource.Load(s_sPrefab);
		if (!resource || !resource.IsValid())
			return;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = s_Anchor.GetOrigin();

		// Local-only ghost (not replicated). Editor preview APIs are editor-scoped; this stays player-safe.
		s_Ghost = GetGame().SpawnEntityPrefabLocal(resource, GetGame().GetWorld(), params);
	}

	//------------------------------------------------------------------------------------------------
	protected static void DeleteGhost()
	{
		if (!s_Ghost)
			return;

		SCR_EntityHelper.DeleteEntityAndChildren(s_Ghost);
		s_Ghost = null;
	}

	//------------------------------------------------------------------------------------------------
	protected static void Tick()
	{
		if (!s_bActive)
		{
			GetGame().GetCallqueue().Remove(Tick);
			return;
		}

		UpdateCursorTransform();
		HandleInput();
	}

	//------------------------------------------------------------------------------------------------
	protected static void UpdateCursorTransform()
	{
		vector worldPos;
		if (!GetCursorWorldPos(worldPos))
		{
			s_bValid = false;
			return;
		}

		s_vPlacePos = worldPos;
		float radius = 50;
		AROR_ResistanceGarageComponent garage = AROR_ResistanceGarageComponent.GetInstance();
		if (garage)
			radius = garage.GetPlaceRadius();

		string reason;
		bool inRange = s_Anchor && vector.DistanceXZ(worldPos, s_Anchor.GetOrigin()) <= radius;
		bool clear = garage && garage.IsPlacementClear(worldPos, reason);
		s_bValid = inRange && clear;

		if (s_bValid != s_bWasValid)
		{
			s_bWasValid = s_bValid;
			if (!s_bValid)
				AROR_NotificationManagerComponent.ShowHint("Invalid placement (range or blocked)");
		}

		if (!s_Ghost)
			return;

		vector mat[4];
		Math3D.AnglesToMatrix(Vector(s_fYaw, 0, 0), mat);
		mat[3] = Vector(worldPos[0], GetGame().GetWorld().GetSurfaceY(worldPos[0], worldPos[2]), worldPos[2]);
		s_Ghost.SetTransform(mat);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool GetCursorWorldPos(out vector worldPos)
	{
		ArmaReforgerScripted game = GetGame();
		if (!game)
			return false;

		WorkspaceWidget workspace = game.GetWorkspace();
		BaseWorld world = game.GetWorld();
		if (!workspace || !world)
			return false;

		int mouseX, mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);
		float sx = workspace.DPIUnscale(mouseX);
		float sy = workspace.DPIUnscale(mouseY);

		vector outDir;
		vector start = workspace.ProjScreenToWorld(sx, sy, outDir, world, -1);

		TraceParam trace = new TraceParam();
		trace.Start = start;
		trace.End = start + outDir * 2000;
		trace.Flags = TraceFlags.WORLD;

		float fraction = world.TraceMove(trace, null);
		if (fraction <= 0 || fraction >= 1)
		{
			// Fall back to surface under the far ray end projected onto XZ
			vector farPos = start + outDir * 200;
			worldPos = Vector(farPos[0], world.GetSurfaceY(farPos[0], farPos[2]), farPos[2]);
			return worldPos[1] > 0;
		}

		worldPos = start + outDir * (2000 * fraction);
		worldPos[1] = world.GetSurfaceY(worldPos[0], worldPos[2]);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static void HandleInput()
	{
		bool shift = Debug.KeyState(KeyCode.KC_LSHIFT) || Debug.KeyState(KeyCode.KC_RSHIFT);
		InputManager input = GetGame().GetInputManager();
		if (input && shift)
		{
			float wheel = input.GetActionValue("MouseWheel");
			if (wheel != 0)
				s_fYaw = WrapYaw(s_fYaw + wheel * 15);
		}

		if (Debug.KeyState(KeyCode.KC_Q))
			s_fYaw = WrapYaw(s_fYaw - 2);
		if (Debug.KeyState(KeyCode.KC_E))
			s_fYaw = WrapYaw(s_fYaw + 2);

		bool placeDown = IsConfirmDown();
		bool cancelDown = IsCancelDown();

		if (placeDown && !s_bPlaceHeld)
		{
			s_bPlaceHeld = true;
			TryConfirmPlace();
		}
		else if (!placeDown)
		{
			s_bPlaceHeld = false;
		}

		if (cancelDown && !s_bCancelHeld)
		{
			s_bCancelHeld = true;
			Cancel();
		}
		else if (!cancelDown)
		{
			s_bCancelHeld = false;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static float WrapYaw(float yaw)
	{
		while (yaw < 0)
			yaw += 360;
		while (yaw >= 360)
			yaw -= 360;
		return yaw;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsConfirmDown()
	{
		InputManager input = GetGame().GetInputManager();
		if (input && (input.GetActionTriggered("MouseLeft") || input.GetActionTriggered("MenuSelect")))
			return true;

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsCancelDown()
	{
		if (Debug.KeyState(KeyCode.KC_ESCAPE))
		{
			Debug.ClearKey(KeyCode.KC_ESCAPE);
			return true;
		}

		InputManager input = GetGame().GetInputManager();
		if (input && (input.GetActionTriggered("MouseRight") || input.GetActionTriggered("MenuBack")))
			return true;

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static void TryConfirmPlace()
	{
		if (!s_bValid)
		{
			AROR_NotificationManagerComponent.ShowHint("Cannot place here (need clear ground within range).");
			return;
		}

		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!pc)
			return;

		int entryId = s_iEntryId;
		vector pos = s_vPlacePos;
		float yaw = s_fYaw;
		RplId anchorId = s_AnchorRplId;
		Stop();
		pc.AROR_RequestGarageRetrieve(entryId, pos, yaw, anchorId);
	}

	//------------------------------------------------------------------------------------------------
	protected static void Cancel()
	{
		Stop();
		AROR_GarageRetrieveUI.OnPlacementCancelled();
	}
}
