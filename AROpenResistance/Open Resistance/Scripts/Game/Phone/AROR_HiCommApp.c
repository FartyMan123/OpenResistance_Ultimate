//! HICOMM screens (see ARCHITECTURE.md -> High Command)
enum AROR_EHiCommScreen
{
	LOGIN,		//!< Clearance check: only Resistance Leaders get past it
	COMMAND,	//!< Resistance name and leaders (the host delegates here)
	STRATEGIC,	//!< Strategic Command: map and orders only
	UNITS		//!< Unit Management: raise squads, review the order of battle
}

//! Passes HICOMM clicks, with their screen position, to the app
class AROR_HiCommClickHandler : ScriptedWidgetEventHandler
{
	protected AROR_HiCommApp m_App;

	//------------------------------------------------------------------------------------------------
	void AROR_HiCommClickHandler(AROR_HiCommApp app)
	{
		m_App = app;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_App)
			m_App.OnClick(w, x, y);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		return m_App && m_App.OnMouseDown(w, x, y, button);
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		return m_App && m_App.OnMouseUp(w, x, y, button);
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseWheel(Widget w, int x, int y, int wheel)
	{
		return m_App && m_App.OnMouseWheel(w, x, y, wheel);
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (m_App)
			m_App.OnMouseLeave(w);

		return false;
	}
}

//! HICOMM: the Resistance Leaders' landscape phone app, in a phosphor-terminal style. Client only; every action is a
//! request to AROR_HighCommandComponent on the server, and everything shown comes from replicated state.
//! The layout (AROR_PhoneMenu.layout) declares fixed pools of panels, texts, buttons and map labels; each frame the
//! current screen takes what it needs from the pools (in landscape screen pixels, 1700 x 900, times the phone scale)
//! and the rest are hidden. Buttons carry an action string for that frame.
class AROR_HiCommApp
{
	protected static const float WIDTH = 1700;
	protected static const float HEIGHT = 900;

	protected static const int PANEL_POOL = 24;
	protected static const int TEXT_POOL = 80;
	protected static const int BUTTON_POOL = 40;
	protected static const int MAP_LABEL_POOL = 128;

	protected static const string BUTTON_PREFIX = "HiCommBtn";

	// Button looks
	protected static const int STYLE_NORMAL = 0;	//!< Own / available
	protected static const int STYLE_SELECTED = 1;
	protected static const int STYLE_PRIMARY = 2;	//!< Amber call to action
	protected static const int STYLE_DISABLED = 3;	//!< Not clickable
	protected static const int STYLE_DANGER = 4;
	protected static const int STYLE_MUTED = 5;	//!< Someone else's (view only)
	protected static const int STYLE_PANEL = 6;	//!< Read-out box (tooltips, order preview); not clickable

	// Strategic Command columns (app units)
	protected static const float BODY_TOP = 104;
	protected static const float BODY_HEIGHT = 782;
	protected static const float LEFT_LEFT = 20;
	protected static const float LEFT_WIDTH = 270;
	protected static const float MAP_LEFT = 302;
	protected static const float MAP_WIDTH = 1083;
	protected static const float RIGHT_LEFT = 1395;
	protected static const float RIGHT_WIDTH = 285;

	// Map palette (packed RGB, see AROR_HiCommMap.Rgb)
	protected static const int PAL_GREEN = 0x86F0AC;
	protected static const int PAL_GREEN_DIM = 0x4E966A;
	protected static const int PAL_AMBER = 0xF6BA40;
	protected static const int PAL_RED = 0xF05C52;
	protected static const int PAL_OCCUPIERS = 0x6096F0;
	protected static const int PAL_INVADERS = 0xF6963C;
	protected static const int PAL_MOVE = 0x7CD8F0;	//!< Move orders

	//! Spacing (m) between squads given one Move point together
	protected static const float MOVE_SPACING = 35;

	//! Map: pick radius and drag threshold (pixels), order ping length (ms), keyboard pan (px/s), wheel zoom step
	protected static const float PICK_RADIUS = 18;
	protected static const float DRAG_THRESHOLD = 6;
	protected static const int PING_MS = 900;
	protected static const float PAN_SPEED = 700;
	protected static const float WHEEL_ZOOM = 1.2;

	//! Rough width of a character as a fraction of the font size, used to centre and right-align text
	protected static const float CHAR_WIDTH_FACTOR = 0.58;
	protected static const float WRAP_CHAR_FACTOR = 0.62;	//!< Slightly wider when wrapping, so lines never overrun

	protected Widget m_wRoot;
	protected ImageWidget m_wBackground;
	protected CanvasWidget m_wMapCanvas;
	protected CanvasWidget m_wMapOverlay;
	protected Widget m_wMapButton;
	protected EditBoxWidget m_wNameEdit;

	protected ref array<ImageWidget> m_aPanels = {};
	protected ref array<RichTextWidget> m_aTexts = {};
	protected ref array<string> m_aTextCache = {};
	protected ref array<ImageWidget> m_aButtonBackgrounds = {};
	protected ref array<RichTextWidget> m_aButtonLabels = {};
	protected ref array<string> m_aButtonLabelCache = {};
	protected ref array<Widget> m_aButtons = {};
	protected ref array<string> m_aButtonActions = {};
	protected ref array<RichTextWidget> m_aMapLabels = {};
	protected ref array<string> m_aMapLabelCache = {};

	protected int m_iPanelCount;
	protected int m_iTextCount;
	protected int m_iButtonCount;
	protected int m_iMapLabelCount;

	protected ref AROR_HiCommMap m_Map;
	protected ref AROR_HiCommClickHandler m_ClickHandler;

	// Palette
	protected ref Color m_Background = AROR_HiCommMap.Srgb(5, 11, 8, 1);
	protected ref Color m_PanelFill = AROR_HiCommMap.Srgb(9, 19, 13, 1);
	protected ref Color m_Border = AROR_HiCommMap.Srgb(44, 92, 64, 1);
	protected ref Color m_Green = AROR_HiCommMap.Srgb(134, 240, 172, 1);
	protected ref Color m_GreenDim = AROR_HiCommMap.Srgb(78, 150, 106, 1);
	protected ref Color m_GreenFaint = AROR_HiCommMap.Srgb(48, 92, 66, 1);
	protected ref Color m_Amber = AROR_HiCommMap.Srgb(246, 186, 64, 1);
	protected ref Color m_Red = AROR_HiCommMap.Srgb(240, 92, 82, 1);
	protected ref Color m_Dark = AROR_HiCommMap.Srgb(6, 12, 8, 1);
	protected ref Color m_Occupiers = AROR_HiCommMap.Srgb(96, 150, 240, 1);
	protected ref Color m_Invaders = AROR_HiCommMap.Srgb(246, 150, 60, 1);

	// State (reset each time the phone opens)
	protected AROR_EHiCommScreen m_eScreen = AROR_EHiCommScreen.LOGIN;
	protected ref array<int> m_aSelected = {};	//!< Selected squad IDs
	protected AROR_EHighCommandOrder m_eSelectedOrder = AROR_EHighCommandOrder.ATTACK;
	protected string m_sSelectedTarget;
	protected bool m_bNameEditFilled;

	// Map input (screen pixels unless noted)
	protected bool m_bLeftDown;
	protected bool m_bRightDown;
	protected bool m_bDragging;
	protected int m_iPressX;
	protected int m_iPressY;
	protected int m_iLastMouseX;
	protected int m_iLastMouseY;
	protected int m_iLastFrameMs;
	protected int m_iLastMapClickMs;
	protected vector m_vCursor;	//!< Canvas pixels
	protected bool m_bCursorInside;
	protected string m_sHoverLocation;
	protected int m_iHoverSquad = -1;
	protected vector m_vPing;
	protected int m_iPingTime;
	protected int m_iPingColor;	//!< Packed RGB
	protected vector m_vMoveTarget;	//!< Planned Move point (world)
	protected bool m_bHasMoveTarget;

	// Map: geometry this frame (app units) and what this frame's overlay put where (canvas pixels)
	protected float m_fScale = 1;
	protected float m_fMapX;
	protected float m_fMapY;
	protected float m_fMapWidth;
	protected float m_fMapHeight;
	protected ref array<int> m_aHitSquadIds = {};
	protected ref array<vector> m_aHitSquadPositions = {};
	protected ref array<string> m_aHitLocationIds = {};
	protected ref array<vector> m_aHitLocationPositions = {};
	protected ref array<string> m_aLabelTexts = {};
	protected ref array<vector> m_aLabelPositions = {};
	protected ref array<ref Color> m_aLabelColors = {};
	protected ref array<float> m_aLabelSizes = {};

	//------------------------------------------------------------------------------------------------
	bool Init(Widget phoneRoot)
	{
		m_wRoot = phoneRoot.FindAnyWidget("HiCommApp");
		m_wBackground = ImageWidget.Cast(phoneRoot.FindAnyWidget("HiCommBackground"));
		m_wMapCanvas = CanvasWidget.Cast(phoneRoot.FindAnyWidget("HiCommMapCanvas"));
		m_wMapOverlay = CanvasWidget.Cast(phoneRoot.FindAnyWidget("HiCommMapOverlay"));
		m_wMapButton = phoneRoot.FindAnyWidget("HiCommMapButton");
		m_wNameEdit = EditBoxWidget.Cast(phoneRoot.FindAnyWidget("HiCommNameEdit"));
		if (!m_wRoot || !m_wBackground || !m_wMapCanvas || !m_wMapOverlay || !m_wMapButton || !m_wNameEdit)
			return false;

		m_ClickHandler = new AROR_HiCommClickHandler(this);
		m_wMapButton.AddHandler(m_ClickHandler);
		m_wMapButton.SetColor(AROR_HiCommMap.Srgb(0, 0, 0, 0));
		m_wBackground.SetColor(m_Background);
		m_Map = new AROR_HiCommMap(m_wMapCanvas, m_wMapOverlay);

		for (int i = 0; i < PANEL_POOL; i++)
		{
			ImageWidget panel = ImageWidget.Cast(phoneRoot.FindAnyWidget("HiCommPanel" + i));
			if (!panel)
				return false;

			m_aPanels.Insert(panel);
		}

		for (int i = 0; i < TEXT_POOL; i++)
		{
			RichTextWidget text = RichTextWidget.Cast(phoneRoot.FindAnyWidget("HiCommText" + i));
			if (!text)
				return false;

			m_aTexts.Insert(text);
			m_aTextCache.Insert(string.Empty);
		}

		for (int i = 0; i < BUTTON_POOL; i++)
		{
			ImageWidget background = ImageWidget.Cast(phoneRoot.FindAnyWidget("HiCommBtnBg" + i));
			RichTextWidget label = RichTextWidget.Cast(phoneRoot.FindAnyWidget("HiCommBtnLabel" + i));
			Widget button = phoneRoot.FindAnyWidget(BUTTON_PREFIX + i);
			if (!background || !label || !button)
				return false;

			button.AddHandler(m_ClickHandler);
			button.SetColor(AROR_HiCommMap.Srgb(0, 0, 0, 0));
			m_aButtonBackgrounds.Insert(background);
			m_aButtonLabels.Insert(label);
			m_aButtonLabelCache.Insert(string.Empty);
			m_aButtons.Insert(button);
			m_aButtonActions.Insert(string.Empty);
		}

		for (int i = 0; i < MAP_LABEL_POOL; i++)
		{
			RichTextWidget mapLabel = RichTextWidget.Cast(phoneRoot.FindAnyWidget("HiCommMapLabel" + i));
			if (!mapLabel)
				return false;

			m_aMapLabels.Insert(mapLabel);
			m_aMapLabelCache.Insert(string.Empty);
		}

		SetVisible(false, 1);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	void SetVisible(bool visible, float opacity)
	{
		m_wRoot.SetVisible(visible);
		m_wRoot.SetOpacity(opacity);
	}

	//------------------------------------------------------------------------------------------------
	//! Every frame while shown: left/top is the landscape screen's corner, scale the phone's current scale
	void Layout(float left, float top, float scale)
	{
		m_fScale = scale;
		Place(m_wRoot, left, top, WIDTH * scale, HEIGHT * scale);
		Place(m_wBackground, 0, 0, WIDTH * scale, HEIGHT * scale);

		m_iPanelCount = 0;
		m_iTextCount = 0;
		m_iButtonCount = 0;
		m_iMapLabelCount = 0;

		// Losing clearance (revoked, host changed) drops back to the login screen
		AROR_HighCommandComponent highCommand = AROR_HighCommandComponent.GetInstance();
		if (m_eScreen != AROR_EHiCommScreen.LOGIN && (!highCommand || !highCommand.IsLeader(GetLocalPlayerId())))
			m_eScreen = AROR_EHiCommScreen.LOGIN;

		bool showMap = m_eScreen == AROR_EHiCommScreen.STRATEGIC;
		bool showNameEdit = false;

		if (m_eScreen == AROR_EHiCommScreen.LOGIN)
		{
			DrawLogin(highCommand);
		}
		else
		{
			DrawTopBar();
			if (m_eScreen == AROR_EHiCommScreen.COMMAND)
				showNameEdit = DrawCommand(highCommand);
			else if (m_eScreen == AROR_EHiCommScreen.STRATEGIC)
				DrawStrategic(highCommand);
			else
				DrawUnits(highCommand);
		}

		m_wMapCanvas.SetVisible(showMap);
		m_wMapOverlay.SetVisible(showMap);
		m_wMapButton.SetVisible(showMap);
		if (!showMap)
		{
			m_bLeftDown = false;
			m_bRightDown = false;
			m_bDragging = false;
		}
		m_wNameEdit.SetVisible(showNameEdit);
		if (!showNameEdit)
			m_bNameEditFilled = false;

		HideUnused();
	}

	//------------------------------------------------------------------------------------------------
	// Screens
	//------------------------------------------------------------------------------------------------
	protected void DrawLogin(AROR_HighCommandComponent highCommand)
	{
		Box(430, 120, 840, 660);
		Text(490, 170, 720, 110, 96, "HICOMM", m_Green, true);
		Text(495, 285, 720, 40, 24, "RESISTANCE COMMAND NETWORK // SECURE TERMINAL", m_GreenDim);
		Panel(490, 340, 720, 2, m_Border);

		int localId = GetLocalPlayerId();
		Text(495, 370, 720, 44, 30, "OPERATOR    " + Upper(GetPlayerName(localId)), m_Green);

		if (!highCommand)
		{
			Text(495, 430, 720, 44, 30, "STATUS      OFFLINE", m_Red);
			Text(495, 500, 700, 120, 24, "HICOMM is not available in this scenario.", m_GreenDim);
			return;
		}

		string clearance = "NONE";
		Color clearanceColor = m_Red;
		if (highCommand.IsHost(localId))
		{
			clearance = "RESISTANCE LEADER (HOST)";
			clearanceColor = m_Amber;
		}
		else if (highCommand.IsLeader(localId))
		{
			clearance = "RESISTANCE LEADER";
			clearanceColor = m_Amber;
		}

		Text(495, 430, 720, 44, 30, "CLEARANCE   " + clearance, clearanceColor);
		Text(495, 490, 720, 44, 26, "RESISTANCE  " + Upper(highCommand.GetResistanceName()), m_GreenDim);

		if (highCommand.IsLeader(localId))
		{
			Text(495, 560, 720, 40, 24, "Identity confirmed. Log in to take command.", m_GreenDim);
			Button(495, 650, 710, 90, "LOG IN", "login", STYLE_PRIMARY, 38);
		}
		else
		{
			Text(495, 560, 720, 80, 24, "ACCESS DENIED: only a Resistance Leader can use HICOMM. Ask the host to delegate you.", m_Red);
			Button(495, 650, 710, 90, "ACCESS DENIED", string.Empty, STYLE_DISABLED, 38);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void DrawTopBar()
	{
		Text(30, 22, 200, 60, 48, "HICOMM", m_Green, true);

		TabButton(240, "COMMAND", AROR_EHiCommScreen.COMMAND, 190);
		TabButton(440, "STRATEGIC COMMAND", AROR_EHiCommScreen.STRATEGIC, 320);
		TabButton(770, "UNIT MANAGEMENT", AROR_EHiCommScreen.UNITS, 290);

		AROR_EconomyComponent economy = AROR_EconomyComponent.GetInstance();
		string funds = "$ -";
		string manpower = "-";
		if (economy)
		{
			funds = AROR_EconomyComponent.FormatMoney(economy.GetFactionFunds(AROR_EFactionRole.RESISTANCE));
			manpower = economy.GetManpower().ToString();
		}

		TextRight(1675, 24, 22, "CR " + funds + "    MP " + manpower, m_Amber);
		TextRight(1675, 56, 20, GetWarSummary(), m_GreenDim);
		Panel(30, 92, 1645, 2, m_Border);
	}

	//------------------------------------------------------------------------------------------------
	protected void TabButton(float x, string label, AROR_EHiCommScreen screen, float width)
	{
		int style = STYLE_NORMAL;
		if (m_eScreen == screen)
			style = STYLE_SELECTED;

		Button(x, 24, width, 54, label, "screen:" + screen, style, 22);
	}

	//------------------------------------------------------------------------------------------------
	//! Resistance identity and leaders; returns true if the name edit box is shown
	protected bool DrawCommand(AROR_HighCommandComponent highCommand)
	{
		int localId = GetLocalPlayerId();
		bool isHost = highCommand.IsHost(localId);

		// Left: the resistance
		Box(30, 110, 790, 770);
		Header(55, 125, "// THE RESISTANCE");
		Text(55, 175, 740, 70, 52, Upper(highCommand.GetResistanceName()), m_Amber, true);

		bool showNameEdit = false;
		if (isHost)
		{
			Text(55, 265, 740, 34, 22, "RENAME (HOST ONLY)", m_GreenDim);
			Panel(55, 305, 520, 64, m_Border);
			Panel(57, 307, 516, 60, m_Dark);
			Place(m_wNameEdit, 70 * m_fScale, 315 * m_fScale, 495 * m_fScale, 46 * m_fScale);
			if (!m_bNameEditFilled)
			{
				m_wNameEdit.SetText(highCommand.GetResistanceName());
				m_bNameEditFilled = true;
			}

			showNameEdit = true;
			Button(595, 305, 200, 64, "APPLY", "rename", STYLE_PRIMARY, 26);
		}
		else
		{
			Text(55, 280, 740, 60, 22, "Only the host can rename the resistance.", m_GreenDim);
		}

		// Campaign summary
		int held, total;
		CountSites(held, total);
		Panel(55, 410, 740, 2, m_Border);
		Stat(55, 430, "SITES HELD", string.Format("%1 / %2", held, total));
		array<AROR_HighCommandSquadInfo> squads = {};
		int squadCount = highCommand.GetSquads(squads);
		Stat(55, 480, "SQUADS IN THE FIELD", squadCount.ToString());
		Stat(55, 530, "YOUR SQUADS", string.Format("%1 / %2", highCommand.CountSquadsOwnedBy(localId), highCommand.GetMaxSquadsPerLeader()));

		string clearance = "RESISTANCE LEADER";
		if (isHost)
			clearance = "RESISTANCE LEADER (HOST)";

		Stat(55, 580, "YOUR CLEARANCE", clearance);
		Text(55, 650, 740, 120, 20, "Strategic Command gives orders to your squads. Unit Management raises new squads with Resistance funds and manpower. Squads are refunded in full when the campaign ends.", m_GreenFaint);
		Button(55, 800, 240, 60, "LOG OUT", "logout", STYLE_NORMAL, 22);

		// Right: leaders
		Box(850, 110, 825, 770);
		Header(875, 125, "// RESISTANCE LEADERS");
		if (isHost)
			Text(875, 165, 780, 34, 20, "Delegate players to give them their own HICOMM command.", m_GreenDim);
		else
			Text(875, 165, 780, 34, 20, "The host appoints Resistance Leaders.", m_GreenDim);

		Text(875, 215, 300, 30, 20, "PLAYER", m_GreenFaint);
		Text(1180, 215, 200, 30, 20, "ROLE", m_GreenFaint);
		Text(1340, 215, 120, 30, 20, "SQUADS", m_GreenFaint);

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		float y = 255;
		foreach (int playerId : playerIds)
		{
			if (y > 820)
				break;

			Color rowColor = m_GreenDim;
			string role = "-";
			if (highCommand.IsHost(playerId))
			{
				role = "HOST";
				rowColor = m_Amber;
			}
			else if (highCommand.IsDelegate(playerId))
			{
				role = "LEADER";
				rowColor = m_Green;
			}

			string name = Upper(GetPlayerName(playerId));
			if (playerId == localId)
				name += " (YOU)";

			Text(875, y + 8, 300, 36, 22, name, rowColor);
			Text(1180, y + 8, 160, 36, 22, role, rowColor);
			Text(1340, y + 8, 100, 36, 22, highCommand.CountSquadsOwnedBy(playerId).ToString(), rowColor);

			if (isHost && !highCommand.IsHost(playerId))
			{
				if (highCommand.IsDelegate(playerId))
					Button(1460, y, 190, 48, "REVOKE", string.Format("leader:%1:0", playerId), STYLE_DANGER, 20);
				else
					Button(1460, y, 190, 48, "DELEGATE", string.Format("leader:%1:1", playerId), STYLE_NORMAL, 20);
			}

			Panel(875, y + 54, 775, 1, m_GreenFaint);
			y += 60;
		}

		return showNameEdit;
	}

	//------------------------------------------------------------------------------------------------
	//! Raise squads; the order of battle with strengths and orders
	protected void DrawUnits(AROR_HighCommandComponent highCommand)
	{
		int localId = GetLocalPlayerId();
		AROR_EconomyComponent economy = AROR_EconomyComponent.GetInstance();
		int funds, manpower;
		if (economy)
		{
			funds = economy.GetFactionFunds(AROR_EFactionRole.RESISTANCE);
			manpower = economy.GetManpower();
		}

		int ownedCount = highCommand.CountSquadsOwnedBy(localId);
		int maximum = highCommand.GetMaxSquadsPerLeader();

		// Left: catalogue
		Box(30, 110, 790, 770);
		Header(55, 125, "// RAISE SQUADS");
		Text(55, 165, 740, 34, 20, "Squads form up at the HQ. Cost: Resistance funds AND manpower.", m_GreenDim);

		array<AROR_HighCommandSquadType> types = {};
		highCommand.GetSquadTypes(types);
		float y = 215;
		foreach (int index, AROR_HighCommandSquadType type : types)
		{
			if (y > 700)
				break;

			Panel(55, y, 740, 110, m_Border);
			Panel(57, y + 2, 736, 106, m_PanelFill);
			Text(80, y + 14, 420, 44, 32, Upper(type.m_sName), m_Green, true);
			Text(80, y + 62, 420, 30, 20, string.Format("%1 MEN  //  CALLSIGN %2", type.GetSize(), type.m_sCallsign), m_GreenDim);

			bool affordable = funds >= type.m_iFundsCost && manpower >= type.m_iManpowerCost;
			Color costColor = m_Amber;
			if (!affordable)
				costColor = m_Red;

			Text(470, y + 20, 160, 30, 22, AROR_EconomyComponent.FormatMoney(type.m_iFundsCost), costColor);
			Text(470, y + 56, 160, 30, 22, type.m_iManpowerCost.ToString() + " MP", costColor);

			if (affordable && ownedCount < maximum)
				Button(620, y + 25, 155, 60, "RAISE", "raise:" + index, STYLE_PRIMARY, 24);
			else
				Button(620, y + 25, 155, 60, "RAISE", string.Empty, STYLE_DISABLED, 24);

			y += 125;
		}

		if (types.IsEmpty())
			Text(55, 220, 740, 60, 22, "No squad types are configured on AROR_HighCommandComponent.", m_Red);

		Panel(55, 790, 740, 1, m_Border);
		Text(55, 805, 740, 30, 22, string.Format("AVAILABLE  %1  //  %2 MP", AROR_EconomyComponent.FormatMoney(funds), manpower), m_Amber);
		Text(55, 838, 740, 30, 20, string.Format("YOUR SQUADS  %1 / %2", ownedCount, maximum), m_GreenDim);

		// Right: order of battle
		Box(850, 110, 825, 770);
		Header(875, 125, "// ORDER OF BATTLE");
		Text(875, 170, 200, 30, 18, "SQUAD", m_GreenFaint);
		Text(1035, 170, 120, 30, 18, "MEN", m_GreenFaint);
		Text(1130, 170, 300, 30, 18, "ORDER", m_GreenFaint);
		Text(1440, 170, 220, 30, 18, "COMMANDER", m_GreenFaint);

		array<AROR_HighCommandSquadInfo> squads = {};
		highCommand.GetSquads(squads);
		y = 205;
		for (int pass = 0; pass < 2; pass++)
		{
			foreach (AROR_HighCommandSquadInfo squad : squads)
			{
				bool own = highCommand.CanCommand(localId, squad.m_iOwnerId);
				if ((pass == 0) != own || y > 830)
					continue;

				Color rowColor = m_GreenFaint;
				if (own)
					rowColor = m_Green;

				Text(875, y, 160, 32, 22, squad.m_sCallsign, rowColor, own);
				Text(1035, y, 100, 32, 22, string.Format("%1/%2", squad.m_iAlive, squad.m_iSize), rowColor);
				Text(1130, y, 310, 32, 22, LongOrder(squad), rowColor);
				Text(1440, y, 220, 32, 22, OwnerTag(squad.m_iOwnerId), rowColor);
				Panel(875, y + 38, 775, 1, m_GreenFaint);
				y += 46;
			}
		}

		if (squads.IsEmpty())
			Text(875, 210, 780, 40, 22, "No squads in the field.", m_GreenDim);
	}

	//------------------------------------------------------------------------------------------------
	// Strategic Command
	//------------------------------------------------------------------------------------------------
	//! Orders only: order of battle, zoomable radar map, selection and orders
	protected void DrawStrategic(AROR_HighCommandComponent highCommand)
	{
		int localId = GetLocalPlayerId();
		array<AROR_HighCommandSquadInfo> squads = {};
		highCommand.GetSquads(squads);
		PruneSelection(highCommand);

		DrawOrderOfBattle(highCommand, squads, localId);

		// Centre: the map, framed
		m_fMapX = MAP_LEFT;
		m_fMapY = BODY_TOP;
		m_fMapWidth = MAP_WIDTH;
		m_fMapHeight = BODY_HEIGHT;
		Panel(m_fMapX - 3, m_fMapY - 3, m_fMapWidth + 6, m_fMapHeight + 6, m_Border);
		Place(m_wMapCanvas, m_fMapX * m_fScale, m_fMapY * m_fScale, m_fMapWidth * m_fScale, m_fMapHeight * m_fScale);
		Place(m_wMapOverlay, m_fMapX * m_fScale, m_fMapY * m_fScale, m_fMapWidth * m_fScale, m_fMapHeight * m_fScale);
		Place(m_wMapButton, m_fMapX * m_fScale, m_fMapY * m_fScale, m_fMapWidth * m_fScale, m_fMapHeight * m_fScale);

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		vector min, max;
		highCommand.GetMapBounds(min, max);
		m_Map.SetView(min, max, workspace.DPIScale(m_fMapWidth * m_fScale), workspace.DPIScale(m_fMapHeight * m_fScale));

		HandleMapInput();
		m_Map.UpdateBase();
		DrawMapOverlay(highCommand, squads, localId);
		DrawMapLabels();
		DrawMapHud(highCommand);

		// Right: selection and orders
		Box(RIGHT_LEFT, BODY_TOP, RIGHT_WIDTH, BODY_HEIGHT);
		DrawSelection(highCommand, squads, localId);
	}

	//------------------------------------------------------------------------------------------------
	//! Left column: every squad, own first (selectable; shift adds), plus selection shortcuts
	protected void DrawOrderOfBattle(AROR_HighCommandComponent highCommand, array<AROR_HighCommandSquadInfo> squads, int localId)
	{
		Box(LEFT_LEFT, BODY_TOP, LEFT_WIDTH, BODY_HEIGHT);
		Header(LEFT_LEFT + 16, BODY_TOP + 12, "// ORDER OF BATTLE");

		float y = BODY_TOP + 56;
		for (int pass = 0; pass < 2; pass++)
		{
			foreach (AROR_HighCommandSquadInfo squad : squads)
			{
				bool own = highCommand.CanCommand(localId, squad.m_iOwnerId);
				if ((pass == 0) != own || y > BODY_TOP + BODY_HEIGHT - 190)
					continue;

				int style = STYLE_MUTED;
				if (m_aSelected.Contains(squad.m_iId))
					style = STYLE_SELECTED;
				else if (own)
					style = STYLE_NORMAL;

				string detail = ShortOrder(squad);
				if (!own)
					detail = OwnerTag(squad.m_iOwnerId);

				// One line per row: shorten long target names
				int maxDetail = 22;
				if (detail.Length() > maxDetail)
					detail = detail.Substring(0, maxDetail - 2) + "..";

				string label = string.Format("%1   %2/%3", squad.m_sCallsign, squad.m_iAlive, squad.m_iSize) + "\n" + detail;
				Button(LEFT_LEFT + 12, y, LEFT_WIDTH - 24, 56, label, "squad:" + squad.m_iId, style, 17, false);
				y += 62;
			}
		}

		if (squads.IsEmpty())
			Text(LEFT_LEFT + 16, BODY_TOP + 60, LEFT_WIDTH - 32, 90, 18, "No squads yet. Raise one in Unit Management.", m_GreenDim);

		float bottom = BODY_TOP + BODY_HEIGHT;
		Button(LEFT_LEFT + 12, bottom - 172, (LEFT_WIDTH - 32) * 0.5, 44, "ALL MINE", "selall", STYLE_NORMAL, 16);
		Button(LEFT_LEFT + 20 + (LEFT_WIDTH - 32) * 0.5, bottom - 172, (LEFT_WIDTH - 32) * 0.5, 44, "NONE", "selnone", STYLE_NORMAL, 16);

		int held, total;
		CountSites(held, total);
		Panel(LEFT_LEFT + 14, bottom - 116, LEFT_WIDTH - 28, 1, m_Border);
		Text(LEFT_LEFT + 16, bottom - 106, LEFT_WIDTH - 32, 28, 18, string.Format("SITES HELD  %1 / %2", held, total), m_GreenDim);
		Text(LEFT_LEFT + 16, bottom - 76, LEFT_WIDTH - 32, 60, 15, "SHIFT+CLICK ADDS TO THE SELECTION", m_GreenFaint);
	}

	//------------------------------------------------------------------------------------------------
	//! Right column: what is selected, the order and its target
	protected void DrawSelection(AROR_HighCommandComponent highCommand, array<AROR_HighCommandSquadInfo> squads, int localId)
	{
		float x = RIGHT_LEFT + 16;
		float width = RIGHT_WIDTH - 32;
		Header(x, BODY_TOP + 12, "// SELECTED");

		if (m_aSelected.IsEmpty())
		{
			Text(x, BODY_TOP + 56, width, 60, 18, "Select squads in the order of battle or on the map.", m_GreenDim);
			Header(x, BODY_TOP + 150, "// CONTROLS");
			Text(x, BODY_TOP + 190, width, 360, 16, "LMB  select squad / set target\nDRAG  box-select squads\nSHIFT  add to selection\nRMB location  attack / defend\nRMB ground  move there\nHOLD  stand by in place\nRMB DRAG / WASD  pan\nWHEEL / Q E  zoom\nHOVER  intel", m_GreenFaint);
			DrawIntel(x, BODY_TOP + BODY_HEIGHT - 110);
			return;
		}

		int commandable = CountCommandable(highCommand, localId);
		if (m_aSelected.Count() == 1)
		{
			AROR_HighCommandSquadInfo squad = highCommand.GetSquadInfo(m_aSelected[0]);
			AROR_HighCommandSquadType type = highCommand.GetSquadType(squad.m_iType);
			string typeName = "SQUAD";
			if (type)
				typeName = Upper(type.m_sName);

			Text(x, BODY_TOP + 50, width, 56, 44, squad.m_sCallsign, m_Amber, true);
			Text(x, BODY_TOP + 108, width, 26, 17, string.Format("%1  //  %2 / %3 MEN", typeName, squad.m_iAlive, squad.m_iSize), m_GreenDim);
			Text(x, BODY_TOP + 134, width, 26, 17, LongOrder(squad), m_Green);
			Text(x, BODY_TOP + 160, width, 26, 17, "CMDR  " + OwnerTag(squad.m_iOwnerId), m_GreenDim);
		}
		else
		{
			string callsigns;
			foreach (int i, int squadId : m_aSelected)
			{
				AROR_HighCommandSquadInfo selected = highCommand.GetSquadInfo(squadId);
				if (!selected)
					continue;

				if (i > 0)
					callsigns += "  ";

				callsigns += selected.m_sCallsign;
			}

			Text(x, BODY_TOP + 50, width, 56, 40, string.Format("%1 SQUADS", m_aSelected.Count()), m_Amber, true);
			Text(x, BODY_TOP + 108, width, 76, 17, callsigns, m_GreenDim);
		}

		if (commandable == 0)
		{
			Text(x, BODY_TOP + 200, width, 90, 18, "VIEW ONLY: none of the selected squads are under your command.", m_Red);
			DrawIntel(x, BODY_TOP + BODY_HEIGHT - 110);
			return;
		}

		// Orders: Move / Attack / Defend need a target; Hold stands the squads by where they are, at once
		Header(x, BODY_TOP + 196, "// ORDER");
		float half = (width - 10) * 0.5;
		OrderButton(x, BODY_TOP + 232, half, "MOVE", AROR_EHighCommandOrder.MOVE);
		OrderButton(x + half + 10, BODY_TOP + 232, half, "ATTACK", AROR_EHighCommandOrder.ATTACK);
		OrderButton(x, BODY_TOP + 284, half, "DEFEND", AROR_EHighCommandOrder.DEFEND);
		Button(x + half + 10, BODY_TOP + 284, half, 46, "HOLD", "hold", STYLE_NORMAL, 20);

		Header(x, BODY_TOP + 346, "// TARGET");
		string targetText;
		if (m_eSelectedOrder == AROR_EHighCommandOrder.MOVE)
		{
			if (m_bHasMoveTarget)
			{
				targetText = "GRID " + AROR_HighCommandComponent.GridReference(m_vMoveTarget);
				Text(x, BODY_TOP + 382, width, 36, 26, targetText, m_Amber, true);
				Text(x, BODY_TOP + 418, width, 26, 15, "Left-click the map to change it.", m_GreenDim);
			}
			else
			{
				Text(x, BODY_TOP + 382, width, 60, 17, "Left-click open ground to pick a point, or right-click it to move at once.", m_GreenDim);
			}
		}
		else
		{
			AROR_LocationComponent target = FindLocation(m_sSelectedTarget);
			if (target)
			{
				targetText = Upper(target.GetName());
				Text(x, BODY_TOP + 382, width, 36, 24, targetText, RoleColor(target.GetControllingRole()), true);
				Text(x, BODY_TOP + 418, width, 26, 15, string.Format("%1  //  %2", TypeName(target.GetLocationType()), RoleName(target.GetControllingRole())), m_GreenDim);
			}
			else
			{
				Text(x, BODY_TOP + 382, width, 60, 17, "Click a location on the map, or right-click it to order at once.", m_GreenDim);
			}
		}

		string problem = GetPlannedOrderProblem(highCommand);
		string who = string.Format("%1 SQUADS", commandable);
		if (m_aSelected.Count() == 1)
			who = highCommand.GetSquadInfo(m_aSelected[0]).m_sCallsign;

		if (problem.IsEmpty())
			Button(x, BODY_TOP + 456, width, 58, string.Format("> %1 %2 %3", who, ShortVerb(m_eSelectedOrder), targetText), string.Empty, STYLE_PANEL, 17, false);
		else
			Button(x, BODY_TOP + 456, width, 58, problem, string.Empty, STYLE_PANEL, 15, false);

		Button(x, BODY_TOP + 524, 100, 64, "CLEAR", "clear", STYLE_NORMAL, 20);
		if (problem.IsEmpty())
			Button(x + 110, BODY_TOP + 524, width - 110, 64, "EXECUTE", "execute", STYLE_PRIMARY, 26);
		else
			Button(x + 110, BODY_TOP + 524, width - 110, 64, "EXECUTE", string.Empty, STYLE_DISABLED, 26);

		DrawIntel(x, BODY_TOP + BODY_HEIGHT - 110);
	}

	//------------------------------------------------------------------------------------------------
	//! Why the planned order cannot be executed yet, or empty if it can
	protected string GetPlannedOrderProblem(AROR_HighCommandComponent highCommand)
	{
		if (m_eSelectedOrder == AROR_EHighCommandOrder.MOVE)
		{
			if (!m_bHasMoveTarget)
				return "Pick a point on the map to move to";

			return highCommand.GetMoveProblem(m_vMoveTarget);
		}

		return AROR_HighCommandComponent.GetOrderTargetProblem(m_eSelectedOrder, FindLocation(m_sSelectedTarget));
	}

	//------------------------------------------------------------------------------------------------
	protected void OrderButton(float x, float y, float width, string label, AROR_EHighCommandOrder order)
	{
		int style = STYLE_NORMAL;
		if (m_eSelectedOrder == order)
			style = STYLE_SELECTED;

		Button(x, y, width, 46, label, "order:" + order, style, 20);
	}

	//------------------------------------------------------------------------------------------------
	//! Short enemy status read from the public operations summary (as RebIntel shows it)
	protected void DrawIntel(float x, float y)
	{
		Header(x, y, "// REBINTEL");
		Text(x, y + 38, RIGHT_WIDTH - 32, 60, 15, GetWarSummary(), m_GreenDim);
	}

	//------------------------------------------------------------------------------------------------
	// Map: input
	//------------------------------------------------------------------------------------------------
	//! Mouse drag (pan / box select), keyboard pan and zoom, hover
	protected void HandleMapInput()
	{
		int now = System.GetTickCount();
		float deltaTime = Math.Clamp((now - m_iLastFrameMs) / 1000.0, 0, 0.1);
		m_iLastFrameMs = now;

		int mouseX, mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);
		vector cursor = ToCanvas(mouseX, mouseY);
		m_vCursor = cursor;
		m_bCursorInside = cursor[0] >= 0 && cursor[1] >= 0 && cursor[0] <= m_Map.GetWidth() && cursor[1] <= m_Map.GetHeight();

		float moved = Math.Sqrt((mouseX - m_iPressX) * (mouseX - m_iPressX) + (mouseY - m_iPressY) * (mouseY - m_iPressY));
		if ((m_bLeftDown || m_bRightDown) && !m_bDragging && moved > GetGame().GetWorkspace().DPIScale(DRAG_THRESHOLD))
			m_bDragging = true;

		if (m_bRightDown && m_bDragging)
			m_Map.Pan(mouseX - m_iLastMouseX, mouseY - m_iLastMouseY);

		m_iLastMouseX = mouseX;
		m_iLastMouseY = mouseY;

		// Keyboard: arrows/WASD pan, Q/E zoom about the centre
		float step = PAN_SPEED * deltaTime * GetGame().GetWorkspace().DPIScale(1);
		float panX, panY;
		if (Debug.KeyState(KeyCode.KC_LEFT) || Debug.KeyState(KeyCode.KC_A))
			panX += step;

		if (Debug.KeyState(KeyCode.KC_RIGHT) || Debug.KeyState(KeyCode.KC_D))
			panX -= step;

		if (Debug.KeyState(KeyCode.KC_UP) || Debug.KeyState(KeyCode.KC_W))
			panY += step;

		if (Debug.KeyState(KeyCode.KC_DOWN) || Debug.KeyState(KeyCode.KC_S))
			panY -= step;

		if (panX != 0 || panY != 0)
			m_Map.Pan(panX, panY);

		float zoomRate = 1 + deltaTime * 2.5;
		if (Debug.KeyState(KeyCode.KC_E))
			m_Map.ZoomAt(zoomRate, m_Map.GetWidth() * 0.5, m_Map.GetHeight() * 0.5);

		if (Debug.KeyState(KeyCode.KC_Q))
			m_Map.ZoomAt(1 / zoomRate, m_Map.GetWidth() * 0.5, m_Map.GetHeight() * 0.5);
	}

	//------------------------------------------------------------------------------------------------
	//! Screen pixels -> map canvas pixels
	protected vector ToCanvas(float x, float y)
	{
		float screenX, screenY;
		m_wMapButton.GetScreenPos(screenX, screenY);
		return Vector(x - screenX, y - screenY, 0);
	}

	//------------------------------------------------------------------------------------------------
	bool OnMouseDown(Widget w, int x, int y, int button)
	{
		if (w != m_wMapButton || !m_wRoot.IsVisible())
			return false;

		m_iPressX = x;
		m_iPressY = y;
		m_iLastMouseX = x;
		m_iLastMouseY = y;
		m_bDragging = false;
		if (button == 0)
			m_bLeftDown = true;
		else
			m_bRightDown = true;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Up can arrive on any HICOMM widget (the cursor may have left the map while dragging)
	bool OnMouseUp(Widget w, int x, int y, int button)
	{
		bool wasDragging = m_bDragging;
		bool handled;
		if (button == 0 && m_bLeftDown)
		{
			m_bLeftDown = false;
			m_iLastMapClickMs = System.GetTickCount();
			if (wasDragging)
				BoxSelect(ToCanvas(m_iPressX, m_iPressY), ToCanvas(x, y));
			else
				MapLeftClick(ToCanvas(x, y));

			handled = true;
		}
		else if (button != 0 && m_bRightDown)
		{
			m_bRightDown = false;
			if (!wasDragging && button == 1)
				MapRightClick(ToCanvas(x, y));

			handled = true;
		}

		m_bDragging = false;
		return handled;
	}

	//------------------------------------------------------------------------------------------------
	bool OnMouseWheel(Widget w, int x, int y, int wheel)
	{
		if (w != m_wMapButton || wheel == 0)
			return false;

		vector anchor = ToCanvas(x, y);
		if (wheel > 0)
			m_Map.ZoomAt(WHEEL_ZOOM, anchor[0], anchor[1]);
		else
			m_Map.ZoomAt(1 / WHEEL_ZOOM, anchor[0], anchor[1]);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Leaving the map ends a drag (its button-up may never arrive)
	void OnMouseLeave(Widget w)
	{
		if (w != m_wMapButton)
			return;

		m_bRightDown = false;
		if (m_bLeftDown && m_bDragging)
			m_bLeftDown = false;

		m_bDragging = false;
	}

	//------------------------------------------------------------------------------------------------
	//! Left click: a squad selects it (shift adds), a location becomes the target; open ground becomes the Move point
	//! while MOVE is the chosen order, otherwise it clears the selection
	protected void MapLeftClick(vector point)
	{
		bool additive = IsShiftDown();
		int squadId = PickSquad(point);
		if (squadId >= 0)
		{
			if (additive)
				ToggleSelected(squadId);
			else
				SelectOnly(squadId);

			return;
		}

		string locationId = PickLocation(point);
		if (!locationId.IsEmpty())
		{
			SetTarget(locationId);
			return;
		}

		if (m_eSelectedOrder == AROR_EHighCommandOrder.MOVE && !m_aSelected.IsEmpty())
		{
			m_vMoveTarget = m_Map.ScreenToWorld(point[0], point[1]);
			m_bHasMoveTarget = true;
			return;
		}

		if (!additive)
			m_aSelected.Clear();
	}

	//------------------------------------------------------------------------------------------------
	//! RTS quick orders for the selection: right-click a location to attack it (enemy) or defend it (ours),
	//! right-click open ground to move there
	protected void MapRightClick(vector point)
	{
		string locationId = PickLocation(point);
		AROR_LocationComponent target = FindLocation(locationId);
		if (m_aSelected.IsEmpty())
		{
			if (target)
				SetTarget(locationId);

			return;
		}

		if (!target)
		{
			IssueOrder(AROR_EHighCommandOrder.MOVE, string.Empty, m_Map.ScreenToWorld(point[0], point[1]));
			return;
		}

		AROR_EHighCommandOrder order = AROR_EHighCommandOrder.ATTACK;
		if (target.GetControllingRole() == AROR_EFactionRole.RESISTANCE)
			order = AROR_EHighCommandOrder.DEFEND;

		SetTarget(locationId);
		IssueOrder(order, locationId, target.GetCenter());
	}

	//------------------------------------------------------------------------------------------------
	protected void BoxSelect(vector from, vector to)
	{
		float left = Math.Min(from[0], to[0]);
		float right = Math.Max(from[0], to[0]);
		float top = Math.Min(from[1], to[1]);
		float bottom = Math.Max(from[1], to[1]);

		AROR_HighCommandComponent highCommand = AROR_HighCommandComponent.GetInstance();
		if (!highCommand)
			return;

		if (!IsShiftDown())
			m_aSelected.Clear();

		int localId = GetLocalPlayerId();
		foreach (int i, vector point : m_aHitSquadPositions)
		{
			if (point[0] < left || point[0] > right || point[1] < top || point[1] > bottom)
				continue;

			AROR_HighCommandSquadInfo squad = highCommand.GetSquadInfo(m_aHitSquadIds[i]);
			if (squad && highCommand.CanCommand(localId, squad.m_iOwnerId) && !m_aSelected.Contains(squad.m_iId))
				m_aSelected.Insert(squad.m_iId);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected int PickSquad(vector point)
	{
		float best = GetGame().GetWorkspace().DPIScale(PICK_RADIUS);
		int picked = -1;
		foreach (int i, vector squadPoint : m_aHitSquadPositions)
		{
			float distance = vector.Distance(point, squadPoint);
			if (distance < best)
			{
				best = distance;
				picked = m_aHitSquadIds[i];
			}
		}

		return picked;
	}

	//------------------------------------------------------------------------------------------------
	protected string PickLocation(vector point)
	{
		float best = GetGame().GetWorkspace().DPIScale(PICK_RADIUS * 1.4);
		string picked;
		foreach (int i, vector locationPoint : m_aHitLocationPositions)
		{
			float distance = vector.Distance(point, locationPoint);
			if (distance < best)
			{
				best = distance;
				picked = m_aHitLocationIds[i];
			}
		}

		return picked;
	}

	//------------------------------------------------------------------------------------------------
	//! Target, with the order that fits it: attack enemy ground, defend our own
	protected void SetTarget(string locationId)
	{
		m_sSelectedTarget = locationId;
		m_bHasMoveTarget = false;
		AROR_LocationComponent target = FindLocation(locationId);
		if (target && target.GetControllingRole() == AROR_EFactionRole.RESISTANCE)
			m_eSelectedOrder = AROR_EHighCommandOrder.DEFEND;
		else
			m_eSelectedOrder = AROR_EHighCommandOrder.ATTACK;
	}

	//------------------------------------------------------------------------------------------------
	//! Sends the order for every selected squad under our command; the server re-checks each one.
	//! Attack/defend use locationId; move uses position, spreading several squads around it; hold needs neither.
	protected void IssueOrder(AROR_EHighCommandOrder order, string locationId, vector position)
	{
		AROR_HighCommandComponent highCommand = AROR_HighCommandComponent.GetInstance();
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!highCommand || !controller)
			return;

		string problem;
		if (order == AROR_EHighCommandOrder.MOVE)
		{
			problem = highCommand.GetMoveProblem(position);
		}
		else if (order != AROR_EHighCommandOrder.HOLD)
		{
			AROR_LocationComponent target = FindLocation(locationId);
			problem = AROR_HighCommandComponent.GetOrderTargetProblem(order, target);
			if (target)
				position = target.GetCenter();
		}

		if (!problem.IsEmpty())
		{
			AROR_NotificationManagerComponent.ShowHint(problem);
			return;
		}

		int localId = GetLocalPlayerId();
		int sent;
		foreach (int squadId : m_aSelected)
		{
			AROR_HighCommandSquadInfo squad = highCommand.GetSquadInfo(squadId);
			if (!squad || !highCommand.CanCommand(localId, squad.m_iOwnerId))
				continue;

			// Several squads moving together take spaced points (a spiral round the target) instead of stacking
			vector destination = position;
			if (order == AROR_EHighCommandOrder.MOVE && sent > 0)
			{
				float angle = sent * 2.4;
				float distance = MOVE_SPACING * Math.Sqrt(sent);
				vector spread = Vector(position[0] + Math.Cos(angle) * distance, 0, position[2] + Math.Sin(angle) * distance);
				string spreadProblem = highCommand.GetMoveProblem(spread);
				if (spreadProblem.IsEmpty())
					destination = spread;
			}

			controller.AROR_RequestHiCommOrder(squadId, order, locationId, destination);
			sent++;
		}

		if (sent == 0)
		{
			AROR_NotificationManagerComponent.ShowHint("None of the selected squads are under your command");
			return;
		}

		// Map acknowledgement ping (hold has no single point)
		if (order != AROR_EHighCommandOrder.HOLD)
		{
			m_vPing = position;
			m_iPingTime = System.GetTickCount();
			m_iPingColor = PAL_GREEN;
			if (order == AROR_EHighCommandOrder.ATTACK)
				m_iPingColor = PAL_RED;
			else if (order == AROR_EHighCommandOrder.MOVE)
				m_iPingColor = PAL_MOVE;
		}

		m_sSelectedTarget = string.Empty;
		m_bHasMoveTarget = false;
	}

	//------------------------------------------------------------------------------------------------
	protected void SelectOnly(int squadId)
	{
		m_aSelected.Clear();
		m_aSelected.Insert(squadId);
	}

	//------------------------------------------------------------------------------------------------
	protected void ToggleSelected(int squadId)
	{
		if (m_aSelected.Contains(squadId))
			m_aSelected.RemoveItem(squadId);
		else
			m_aSelected.Insert(squadId);
	}

	//------------------------------------------------------------------------------------------------
	protected void PruneSelection(AROR_HighCommandComponent highCommand)
	{
		for (int i = m_aSelected.Count() - 1; i >= 0; i--)
		{
			if (!highCommand.GetSquadInfo(m_aSelected[i]))
				m_aSelected.Remove(i);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected int CountCommandable(AROR_HighCommandComponent highCommand, int localId)
	{
		int count;
		foreach (int squadId : m_aSelected)
		{
			AROR_HighCommandSquadInfo squad = highCommand.GetSquadInfo(squadId);
			if (squad && highCommand.CanCommand(localId, squad.m_iOwnerId))
				count++;
		}

		return count;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsShiftDown()
	{
		return Debug.KeyState(KeyCode.KC_LSHIFT) || Debug.KeyState(KeyCode.KC_RSHIFT);
	}

	//------------------------------------------------------------------------------------------------
	// Map: drawing
	//------------------------------------------------------------------------------------------------
	//! Everything that moves, redrawn every frame on the overlay canvas
	protected void DrawMapOverlay(AROR_HighCommandComponent highCommand, array<AROR_HighCommandSquadInfo> squads, int localId)
	{
		m_aHitSquadIds.Clear();
		m_aHitSquadPositions.Clear();
		m_aHitLocationIds.Clear();
		m_aHitLocationPositions.Clear();
		m_aLabelTexts.Clear();
		m_aLabelPositions.Clear();
		m_aLabelColors.Clear();
		m_aLabelSizes.Clear();

		int now = System.GetTickCount();
		float unit = GetGame().GetWorkspace().DPIScale(1);
		float zoom = m_Map.GetZoom();
		float width = m_Map.GetWidth();
		float height = m_Map.GetHeight();
		float markerScale = Math.Min(1 + (zoom - 1) * 0.06, 1.5) * unit;

		m_Map.BeginOverlay();

		// Cursor crosshair
		if (m_bCursorInside)
		{
			Color crosshair = AROR_HiCommMap.Rgb(AROR_HiCommMap.COAST, 0.12);
			m_Map.DrawLine(Vector(0, m_vCursor[1], 0), Vector(width, m_vCursor[1], 0), crosshair, 1);
			m_Map.DrawLine(Vector(m_vCursor[0], 0, 0), Vector(m_vCursor[0], height, 0), crosshair, 1);
		}

		DrawGridLabels(width, height);
		DrawLocations(highCommand, markerScale, zoom, width, height);
		DrawSquads(highCommand, squads, localId, markerScale, zoom, unit);

		// Order acknowledgement ping
		int pingAge = now - m_iPingTime;
		if (m_iPingTime > 0 && pingAge < PING_MS)
		{
			float progress = pingAge / (PING_MS * 1.0);
			vector pingPoint = m_Map.WorldToScreen(m_vPing);
			m_Map.DrawCircle(pingPoint, (8 + 34 * progress) * unit, AROR_HiCommMap.Rgb(m_iPingColor, 1 - progress), false, 2.5, 28);
			m_Map.DrawCircle(pingPoint, (4 + 18 * progress) * unit, AROR_HiCommMap.Rgb(m_iPingColor, (1 - progress) * 0.6), false, 1.5, 20);
		}

		// Box selection
		if (m_bLeftDown && m_bDragging)
		{
			vector from = ToCanvas(m_iPressX, m_iPressY);
			array<float> box = {from[0], from[1], m_vCursor[0], from[1], m_vCursor[0], m_vCursor[1], from[0], m_vCursor[1]};
			m_Map.DrawPolygon(box, AROR_HiCommMap.Rgb(PAL_AMBER, 0.08));
			m_Map.DrawPolyline(box, AROR_HiCommMap.Rgb(PAL_AMBER, 0.9), 1.5, true);
		}

		DrawScaleBar(width, height, unit);
		DrawLegend(height, unit);
		m_Map.DrawFrameCorners(AROR_HiCommMap.Rgb(AROR_HiCommMap.COAST, 0.6));
		m_Map.CommitOverlay();
	}

	//------------------------------------------------------------------------------------------------
	//! Grid references along the top and left edges, matching the base layer's adaptive spacing
	protected void DrawGridLabels(float width, float height)
	{
		float spacing = m_Map.GetGridSpacing();
		float minX, minZ, maxX, maxZ;
		m_Map.GetVisibleBounds(minX, minZ, maxX, maxZ);
		Color color = AROR_HiCommMap.Rgb(AROR_HiCommMap.COAST, 0.55);

		for (float x = Math.Ceil(minX / spacing) * spacing; x <= maxX; x += spacing)
		{
			float screenX = m_Map.WorldToScreen(Vector(x, 0, 0))[0];
			if (screenX > 40)
				AddMapLabel(GridReference(x, spacing), Vector(screenX + 3, 4, 0), color, 12);
		}

		for (float z = Math.Ceil(minZ / spacing) * spacing; z <= maxZ; z += spacing)
		{
			float screenY = m_Map.WorldToScreen(Vector(0, 0, z))[1];
			if (screenY > 24 && screenY < height - 20)
				AddMapLabel(GridReference(z, spacing), Vector(6, screenY + 2, 0), color, 12);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Every location, coloured by who holds it: Resistance filled, Occupier hollow squares, Invader diamonds
	protected void DrawLocations(AROR_HighCommandComponent highCommand, float markerScale, float zoom, float width, float height)
	{
		AROR_LocationManagerComponent locationManager = AROR_LocationManagerComponent.GetInstance();
		if (!locationManager)
			return;

		array<AROR_LocationComponent> locations = {};
		locationManager.GetLocations(locations);
		float metres = m_Map.PixelsPerMetre();

		foreach (AROR_LocationComponent location : locations)
		{
			if (!location)
				continue;

			vector point = m_Map.WorldToScreen(location.GetCenter());
			if (point[0] < -40 || point[1] < -40 || point[0] > width + 40 || point[1] > height + 40)
				continue;

			AROR_EFactionRole role = location.GetControllingRole();
			int packed = RolePacked(role);
			Color color = AROR_HiCommMap.Rgb(packed, 1);
			AROR_ELocationType type = location.GetLocationType();
			bool settlement = type == AROR_ELocationType.TOWN || type == AROR_ELocationType.CITY;

			// Zoomed in: the location's area
			if (zoom >= 2.5 && type != AROR_ELocationType.HQ)
				m_Map.DrawCircle(point, location.GetRadius() * metres, AROR_HiCommMap.Rgb(packed, 0.22), false, 1, 36);

			if (type == AROR_ELocationType.HQ)
			{
				m_Map.DrawSquare(point, 6 * markerScale, color, true);
				m_Map.DrawSquare(point, 10 * markerScale, color, false, 1.5);
				m_Map.DrawLine(point + Vector(0, -10 * markerScale, 0), point + Vector(0, -22 * markerScale, 0), color, 1.5);
				m_Map.DrawPolygon({point[0], point[1] - 22 * markerScale, point[0] + 9 * markerScale, point[1] - 19 * markerScale, point[0], point[1] - 16 * markerScale}, color);
			}
			else if (settlement)
			{
				float radius = 3.5 * markerScale;
				if (type == AROR_ELocationType.CITY)
					radius = 5 * markerScale;

				m_Map.DrawCircle(point, radius, color, true, 1, 12);
			}
			else if (type == AROR_ELocationType.SUPPORT_CORRIDOR)
			{
				m_Map.DrawDiamond(point, 7 * markerScale, color, false);
			}
			else if (role == AROR_EFactionRole.INVADERS)
			{
				m_Map.DrawDiamond(point, 6.5 * markerScale, color, true);
			}
			else if (role == AROR_EFactionRole.RESISTANCE)
			{
				m_Map.DrawSquare(point, 5 * markerScale, color, true);
			}
			else
			{
				m_Map.DrawSquare(point, 5.5 * markerScale, color, false, 1.8);
			}

			string id = location.GetId();
			m_aHitLocationIds.Insert(id);
			m_aHitLocationPositions.Insert(point);

			if (id == m_sHoverLocation)
				m_Map.DrawCircle(point, 14 * markerScale, AROR_HiCommMap.Rgb(PAL_AMBER, 0.7), false, 1.5, 24);

			// Names: important sites always, everything once zoomed in
			bool important = type == AROR_ELocationType.HQ || type == AROR_ELocationType.AIRPORT || type == AROR_ELocationType.MILITARY_BASE || type == AROR_ELocationType.CITY;
			if (zoom >= 1.6 || important || id == m_sSelectedTarget || id == m_sHoverLocation)
			{
				string name = Upper(location.GetName());
				if (id == m_sSelectedTarget)
					name = "OBJ " + name;

				AddMapLabel(name, point + Vector(9 * markerScale, -9 * markerScale, 0), AROR_HiCommMap.Rgb(packed, 0.85), 13);
			}
		}

		// Chosen target
		AROR_LocationComponent target = FindLocation(m_sSelectedTarget);
		if (target)
			m_Map.DrawReticle(m_Map.WorldToScreen(target.GetCenter()), 13 * markerScale, AROR_HiCommMap.Rgb(PAL_AMBER, 1));
	}

	//------------------------------------------------------------------------------------------------
	//! Squads with their current orders, and the planned order for the selection
	protected void DrawSquads(AROR_HighCommandComponent highCommand, array<AROR_HighCommandSquadInfo> squads, int localId, float markerScale, float zoom, float unit)
	{
		AROR_LocationComponent plannedTarget = FindLocation(m_sSelectedTarget);
		vector plannedPoint;
		if (m_bHasMoveTarget)
		{
			plannedTarget = null;
			plannedPoint = m_Map.WorldToScreen(m_vMoveTarget);
		}
		else if (plannedTarget)
		{
			plannedPoint = m_Map.WorldToScreen(plannedTarget.GetCenter());
		}

		// Order lines first, under every symbol
		foreach (AROR_HighCommandSquadInfo squad : squads)
		{
			vector point = m_Map.WorldToScreen(squad.m_vPosition);
			AROR_LocationComponent target = FindLocation(squad.m_sTargetId);
			if (squad.m_eOrder == AROR_EHighCommandOrder.MOVE)
			{
				// Move: cyan dashed route to a cross at the destination
				vector movePoint = m_Map.WorldToScreen(squad.m_vTarget);
				m_Map.DrawDashedLine(point, movePoint, AROR_HiCommMap.Rgb(PAL_MOVE, 0.85), 2 * unit, 6 * unit, 5 * unit);
				DrawCross(movePoint, 6 * unit, AROR_HiCommMap.Rgb(PAL_MOVE, 1));
			}
			else if (squad.m_eOrder == AROR_EHighCommandOrder.HOLD)
			{
				// Hold: the area the squad stands by in
				vector holdPoint = m_Map.WorldToScreen(squad.m_vTarget);
				float holdRadius = Math.Max(30 * m_Map.PixelsPerMetre(), 10 * unit);
				m_Map.DrawCircle(holdPoint, holdRadius, AROR_HiCommMap.Rgb(PAL_GREEN, 0.5), false, 1.5, 24);
			}
			else if (target && squad.m_eOrder != AROR_EHighCommandOrder.NONE)
			{
				vector targetPoint = m_Map.WorldToScreen(target.GetCenter());
				if (squad.m_eOrder == AROR_EHighCommandOrder.ATTACK)
				{
					m_Map.DrawGlowLine(point, targetPoint, AROR_HiCommMap.Rgb(PAL_RED, 0.85), AROR_HiCommMap.Rgb(PAL_RED, 0.15), 2 * unit);
					m_Map.DrawArrowHead(point, targetPoint, 11 * unit, AROR_HiCommMap.Rgb(PAL_RED, 0.95));
				}
				else
				{
					m_Map.DrawDashedLine(point, targetPoint, AROR_HiCommMap.Rgb(PAL_GREEN, 0.8), 2 * unit, 8 * unit, 6 * unit);
					m_Map.DrawArrowHead(point, targetPoint, 9 * unit, AROR_HiCommMap.Rgb(PAL_GREEN, 0.9));
				}
			}

			bool planning = (plannedTarget || m_bHasMoveTarget) && m_aSelected.Contains(squad.m_iId) && highCommand.CanCommand(localId, squad.m_iOwnerId);
			if (planning)
			{
				m_Map.DrawDashedLine(point, plannedPoint, AROR_HiCommMap.Rgb(PAL_AMBER, 0.9), 2.5 * unit, 10 * unit, 6 * unit);
				m_Map.DrawArrowHead(point, plannedPoint, 12 * unit, AROR_HiCommMap.Rgb(PAL_AMBER, 1));
			}
		}

		if (m_bHasMoveTarget)
			DrawCross(plannedPoint, 9 * unit, AROR_HiCommMap.Rgb(PAL_AMBER, 1));

		Color fill = AROR_HiCommMap.Srgb(6, 16, 10, 0.92);
		foreach (AROR_HighCommandSquadInfo squad : squads)
		{
			vector point = m_Map.WorldToScreen(squad.m_vPosition);
			bool selected = m_aSelected.Contains(squad.m_iId);
			bool own = highCommand.CanCommand(localId, squad.m_iOwnerId);

			int packed = PAL_GREEN_DIM;
			if (selected)
				packed = PAL_AMBER;
			else if (own)
				packed = PAL_GREEN;

			Color color = AROR_HiCommMap.Rgb(packed, 1);
			if (squad.m_iId == m_iHoverSquad)
				m_Map.DrawCircle(point, 18 * markerScale, AROR_HiCommMap.Rgb(packed, 0.35), true, 1, 20);

			m_Map.DrawUnit(point, 9 * markerScale, color, fill, 1, selected);
			m_aHitSquadIds.Insert(squad.m_iId);
			m_aHitSquadPositions.Insert(point);

			string label = squad.m_sCallsign;
			if (zoom >= 2)
				label += string.Format("  %1/%2", squad.m_iAlive, squad.m_iSize);

			AddMapLabel(label, point + Vector(14 * markerScale, -2 * markerScale, 0), AROR_HiCommMap.Rgb(packed, 1), 14);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Bottom-right scale bar with a round length of about 80-160 pixels
	protected void DrawScaleBar(float width, float height, float unit)
	{
		array<float> lengths = {25, 50, 100, 200, 500, 1000, 2000, 5000};
		float metres = m_Map.PixelsPerMetre();
		float length = lengths[0];
		foreach (float candidate : lengths)
		{
			if (candidate * metres <= 160 * unit)
				length = candidate;
		}

		float pixels = length * metres;
		float right = width - 24 * unit;
		float y = height - 26 * unit;
		Color color = AROR_HiCommMap.Rgb(AROR_HiCommMap.COAST, 0.85);
		m_Map.DrawLine(Vector(right - pixels, y, 0), Vector(right, y, 0), color, 2);
		m_Map.DrawLine(Vector(right - pixels, y - 6 * unit, 0), Vector(right - pixels, y + 2 * unit, 0), color, 2);
		m_Map.DrawLine(Vector(right, y - 6 * unit, 0), Vector(right, y + 2 * unit, 0), color, 2);

		string text = string.Format("%1 M", length);
		if (length >= 1000)
			text = string.Format("%1 KM", length / 1000);

		AddMapLabel(text, Vector(right - pixels, y - 24 * unit, 0), color, 13);
	}

	//------------------------------------------------------------------------------------------------
	//! Bottom-left legend, drawn on the overlay with pooled labels
	protected void DrawLegend(float height, float unit)
	{
		float left = 12 * unit;
		float top = height - 92 * unit;
		float boxWidth = 330 * unit;
		float boxHeight = 52 * unit;
		array<float> box = {left, top, left + boxWidth, top, left + boxWidth, top + boxHeight, left, top + boxHeight};
		m_Map.DrawPolygon(box, AROR_HiCommMap.Srgb(4, 10, 7, 0.84));
		m_Map.DrawPolyline(box, AROR_HiCommMap.Rgb(AROR_HiCommMap.GRID, 0.9), 1, true);

		float row1 = top + 15 * unit;
		float row2 = top + 37 * unit;
		Color label = AROR_HiCommMap.Rgb(PAL_GREEN_DIM, 1);

		m_Map.DrawUnit(Vector(left + 16 * unit, row1, 0), 7 * unit, AROR_HiCommMap.Rgb(PAL_GREEN, 1), AROR_HiCommMap.Srgb(6, 16, 10, 0.92), 0, false);
		AddMapLabel("SQUAD", Vector(left + 30 * unit, row1 - 9 * unit, 0), label, 12);
		m_Map.DrawSquare(Vector(left + 120 * unit, row1, 0), 5 * unit, AROR_HiCommMap.Rgb(PAL_GREEN, 1), true);
		AddMapLabel("OUR SITE", Vector(left + 132 * unit, row1 - 9 * unit, 0), label, 12);
		m_Map.DrawCircle(Vector(left + 230 * unit, row1, 0), 4 * unit, AROR_HiCommMap.Rgb(PAL_GREEN, 1), true, 1, 12);
		AddMapLabel("TOWN", Vector(left + 242 * unit, row1 - 9 * unit, 0), label, 12);

		m_Map.DrawSquare(Vector(left + 16 * unit, row2, 0), 5 * unit, AROR_HiCommMap.Rgb(PAL_OCCUPIERS, 1), false, 1.8);
		AddMapLabel("OCC SITE", Vector(left + 30 * unit, row2 - 9 * unit, 0), label, 12);
		m_Map.DrawDiamond(Vector(left + 120 * unit, row2, 0), 6 * unit, AROR_HiCommMap.Rgb(PAL_INVADERS, 1), true);
		AddMapLabel("INV SITE", Vector(left + 132 * unit, row2 - 9 * unit, 0), label, 12);
		m_Map.DrawLine(Vector(left + 222 * unit, row2, 0), Vector(left + 240 * unit, row2, 0), AROR_HiCommMap.Rgb(PAL_RED, 1), 2);
		AddMapLabel("ATTACK", Vector(left + 246 * unit, row2 - 9 * unit, 0), label, 12);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddMapLabel(string text, vector canvasPoint, Color color, float size)
	{
		m_aLabelTexts.Insert(text);
		m_aLabelPositions.Insert(canvasPoint);
		m_aLabelColors.Insert(color);
		m_aLabelSizes.Insert(size);
	}

	//------------------------------------------------------------------------------------------------
	//! Labels from this frame's overlay, placed over the canvas (canvas pixels -> app units)
	protected void DrawMapLabels()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		foreach (int i, string text : m_aLabelTexts)
		{
			if (m_iMapLabelCount >= MAP_LABEL_POOL)
				break;

			RichTextWidget label = m_aMapLabels[m_iMapLabelCount];
			if (m_aMapLabelCache[m_iMapLabelCount] != text)
			{
				label.SetText(text);
				m_aMapLabelCache[m_iMapLabelCount] = text;
			}

			label.SetColor(m_aLabelColors[i]);
			label.SetExactFontSize(Math.Max(1, Math.Round(m_aLabelSizes[i] * m_fScale)));
			// Keep labels inside the map: skip ones whose anchor is outside, flip long ones to the left near the edge
			vector point = m_aLabelPositions[i];
			float labelWidth = workspace.DPIScale(text.Length() * m_aLabelSizes[i] * CHAR_WIDTH_FACTOR * m_fScale);
			float labelHeight = workspace.DPIScale(m_aLabelSizes[i] * 1.3 * m_fScale);
			if (point[0] < 0 || point[1] < 0 || point[0] > m_Map.GetWidth() || point[1] + labelHeight > m_Map.GetHeight())
				continue;

			float labelX = point[0];
			if (labelX + labelWidth > m_Map.GetWidth() - 4)
				labelX = Math.Max(m_Map.GetWidth() - 4 - labelWidth, 0);

			Place(label, m_fMapX * m_fScale + workspace.DPIUnscale(labelX), m_fMapY * m_fScale + workspace.DPIUnscale(point[1]), workspace.DPIUnscale(labelWidth) + 8, 24 * m_fScale);
			label.SetVisible(true);
			m_iMapLabelCount++;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Map controls, zoom/grid readout, cursor grid reference and the hover tooltip (pooled widgets over the canvas)
	protected void DrawMapHud(AROR_HighCommandComponent highCommand)
	{
		float right = m_fMapX + m_fMapWidth - 14;
		float top = m_fMapY + 12;
		Button(right - 60, top, 60, 40, "HQ", "hq", STYLE_NORMAL, 16);
		Button(right - 126, top, 60, 40, "SEL", "centresel", STYLE_NORMAL, 16);
		Button(right - 192, top, 60, 40, "FIT", "fit", STYLE_NORMAL, 16);
		Button(right - 240, top, 42, 40, "-", "zoomout", STYLE_NORMAL, 22);
		Button(right - 288, top, 42, 40, "+", "zoomin", STYLE_NORMAL, 22);

		// Read-out along the bottom strip, between the legend and the scale bar
		string readout = string.Format("ZOOM %1X", Math.Round(m_Map.GetZoom() * 10) / 10) + "  //  GRID " + SpacingText(m_Map.GetGridSpacing());
		if (m_bCursorInside)
		{
			vector world = m_Map.ScreenToWorld(m_vCursor[0], m_vCursor[1]);
			readout += "  //  CURSOR " + GridReference(world[0], 100) + " " + GridReference(world[2], 100);
		}

		Text(m_fMapX + 360, m_fMapY + m_fMapHeight - 40, 560, 28, 15, readout, m_GreenDim);

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		m_sHoverLocation = string.Empty;
		m_iHoverSquad = -1;
		if (!m_bCursorInside)
			return;

		// Hover tooltip: squads first, then locations
		string tooltip;
		m_iHoverSquad = PickSquad(m_vCursor);
		if (m_iHoverSquad >= 0)
		{
			AROR_HighCommandSquadInfo squad = highCommand.GetSquadInfo(m_iHoverSquad);
			if (squad)
				tooltip = string.Format("%1   %2/%3 MEN", squad.m_sCallsign, squad.m_iAlive, squad.m_iSize) + "\n" + LongOrder(squad) + "\nCMDR " + OwnerTag(squad.m_iOwnerId);
		}
		else
		{
			m_sHoverLocation = PickLocation(m_vCursor);
			AROR_LocationComponent location = FindLocation(m_sHoverLocation);
			if (location)
			{
				tooltip = Upper(location.GetName()) + "\n" + TypeName(location.GetLocationType()) + "  //  " + RoleName(location.GetControllingRole());
				AROR_LocationComponent hq = AROR_HighCommandComponent.FindHQ();
				if (hq && hq != location)
					tooltip += string.Format("\n%1 KM FROM HQ", Math.Round(vector.DistanceXZ(hq.GetCenter(), location.GetCenter()) / 100) / 10);
				else
					tooltip += "\nRESISTANCE HQ";
			}
		}

		if (tooltip.IsEmpty())
			return;

		float tipX = m_fMapX + workspace.DPIUnscale(m_vCursor[0]) / m_fScale + 18;
		float tipY = m_fMapY + workspace.DPIUnscale(m_vCursor[1]) / m_fScale + 18;
		tipX = Math.Min(tipX, m_fMapX + m_fMapWidth - 330);
		tipY = Math.Min(tipY, m_fMapY + m_fMapHeight - 96);
		Button(tipX, tipY, 320, 86, tooltip, string.Empty, STYLE_PANEL, 15, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Arma-style grid digits: km (2), hectometres (3) or decametres (4), depending on the spacing
	protected static string GridReference(float metres, float spacing)
	{
		int value = Math.Floor(Math.Max(metres, 0) / 1000);
		int digits = 2;
		if (spacing < 100)
		{
			value = Math.Floor(Math.Max(metres, 0) / 10);
			digits = 4;
		}
		else if (spacing < 1000)
		{
			value = Math.Floor(Math.Max(metres, 0) / 100);
			digits = 3;
		}

		return value.ToString(digits);
	}

	//------------------------------------------------------------------------------------------------
	protected static string SpacingText(float spacing)
	{
		if (spacing >= 1000)
			return string.Format("%1 KM", spacing / 1000);

		return string.Format("%1 M", spacing);
	}

	//------------------------------------------------------------------------------------------------
	// Input
	//------------------------------------------------------------------------------------------------
	void OnClick(Widget w, int x, int y)
	{
		if (!w || !m_wRoot.IsVisible())
			return;

		// Map clicks are handled on button up; this is the fallback if a button swallows the up event
		if (w == m_wMapButton)
		{
			if (m_bLeftDown && System.GetTickCount() - m_iLastMapClickMs > 150)
				OnMouseUp(w, x, y, 0);

			return;
		}

		string name = w.GetName();
		if (name.IndexOf(BUTTON_PREFIX) != 0)
			return;

		int index = name.Substring(BUTTON_PREFIX.Length(), name.Length() - BUTTON_PREFIX.Length()).ToInt();
		if (!m_aButtonActions.IsIndexValid(index) || index >= m_iButtonCount)
			return;

		DoAction(m_aButtonActions[index]);
	}

	//------------------------------------------------------------------------------------------------
	protected void DoAction(string action)
	{
		if (action.IsEmpty())
			return;

		array<string> parts = {};
		action.Split(":", parts, false);
		string verb = parts[0];

		AROR_HighCommandComponent highCommand = AROR_HighCommandComponent.GetInstance();
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!highCommand || !controller)
			return;

		if (verb == "login")
		{
			if (highCommand.IsLeader(GetLocalPlayerId()))
				m_eScreen = AROR_EHiCommScreen.COMMAND;
		}
		else if (verb == "logout")
		{
			m_eScreen = AROR_EHiCommScreen.LOGIN;
		}
		else if (verb == "screen" && parts.Count() > 1)
		{
			m_eScreen = parts[1].ToInt();
		}
		else if (verb == "squad" && parts.Count() > 1)
		{
			if (IsShiftDown())
				ToggleSelected(parts[1].ToInt());
			else
				SelectOnly(parts[1].ToInt());
		}
		else if (verb == "selall")
		{
			m_aSelected.Clear();
			array<AROR_HighCommandSquadInfo> squads = {};
			highCommand.GetSquads(squads);
			foreach (AROR_HighCommandSquadInfo squad : squads)
			{
				if (highCommand.CanCommand(GetLocalPlayerId(), squad.m_iOwnerId))
					m_aSelected.Insert(squad.m_iId);
			}
		}
		else if (verb == "selnone")
		{
			m_aSelected.Clear();
		}
		else if (verb == "order" && parts.Count() > 1)
		{
			// Move targets a point, the others a location: switching drops the other kind of target
			m_eSelectedOrder = parts[1].ToInt();
			if (m_eSelectedOrder == AROR_EHighCommandOrder.MOVE)
				m_sSelectedTarget = string.Empty;
			else
				m_bHasMoveTarget = false;
		}
		else if (verb == "clear")
		{
			m_sSelectedTarget = string.Empty;
			m_bHasMoveTarget = false;
		}
		else if (verb == "execute")
		{
			string problem = GetPlannedOrderProblem(highCommand);
			if (!m_aSelected.IsEmpty() && problem.IsEmpty())
				IssueOrder(m_eSelectedOrder, m_sSelectedTarget, m_vMoveTarget);
		}
		else if (verb == "hold")
		{
			if (!m_aSelected.IsEmpty())
				IssueOrder(AROR_EHighCommandOrder.HOLD, string.Empty, vector.Zero);
		}
		else if (verb == "zoomin")
		{
			m_Map.ZoomAt(WHEEL_ZOOM * WHEEL_ZOOM, m_Map.GetWidth() * 0.5, m_Map.GetHeight() * 0.5);
		}
		else if (verb == "zoomout")
		{
			m_Map.ZoomAt(1 / (WHEEL_ZOOM * WHEEL_ZOOM), m_Map.GetWidth() * 0.5, m_Map.GetHeight() * 0.5);
		}
		else if (verb == "fit")
		{
			m_Map.Fit();
		}
		else if (verb == "hq")
		{
			AROR_LocationComponent hq = AROR_HighCommandComponent.FindHQ();
			if (hq)
			{
				m_Map.CentreOn(hq.GetCenter());
				if (m_Map.GetZoom() < 3)
					m_Map.ZoomAt(3 / m_Map.GetZoom(), m_Map.GetWidth() * 0.5, m_Map.GetHeight() * 0.5);
			}
		}
		else if (verb == "centresel")
		{
			CentreOnSelection(highCommand);
		}
		else if (verb == "raise" && parts.Count() > 1)
		{
			controller.AROR_RequestHiCommRaise(parts[1].ToInt());
		}
		else if (verb == "leader" && parts.Count() > 2)
		{
			controller.AROR_RequestHiCommSetLeader(parts[1].ToInt(), parts[2].ToInt() != 0);
		}
		else if (verb == "rename")
		{
			controller.AROR_RequestHiCommRename(m_wNameEdit.GetText());
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void CentreOnSelection(AROR_HighCommandComponent highCommand)
	{
		vector sum;
		int count;
		foreach (int squadId : m_aSelected)
		{
			AROR_HighCommandSquadInfo squad = highCommand.GetSquadInfo(squadId);
			if (!squad)
				continue;

			sum = sum + squad.m_vPosition;
			count++;
		}

		if (count > 0)
			m_Map.CentreOn(sum * (1.0 / count));
	}

	//------------------------------------------------------------------------------------------------
	// Pooled widgets (app units: landscape screen pixels)
	//------------------------------------------------------------------------------------------------
	protected void Panel(float x, float y, float width, float height, Color color)
	{
		if (m_iPanelCount >= PANEL_POOL)
			return;

		ImageWidget panel = m_aPanels[m_iPanelCount];
		panel.SetColor(color);
		Place(panel, x * m_fScale, y * m_fScale, width * m_fScale, height * m_fScale);
		panel.SetVisible(true);
		m_iPanelCount++;
	}

	//------------------------------------------------------------------------------------------------
	//! Bordered panel
	protected void Box(float x, float y, float width, float height)
	{
		Panel(x, y, width, height, m_Border);
		Panel(x + 2, y + 2, width - 4, height - 4, m_PanelFill);
	}

	//------------------------------------------------------------------------------------------------
	protected void Text(float x, float y, float width, float height, float size, string content, Color color, bool bold = false)
	{
		if (m_iTextCount >= TEXT_POOL)
			return;

		content = Wrap(content, width, size);
		RichTextWidget text = m_aTexts[m_iTextCount];
		if (m_aTextCache[m_iTextCount] != content)
		{
			text.SetText(content);
			m_aTextCache[m_iTextCount] = content;
		}

		text.SetColor(color);
		text.SetBold(bold);
		text.SetExactFontSize(Math.Max(1, Math.Round(size * m_fScale)));
		Place(text, x * m_fScale, y * m_fScale, width * m_fScale, height * m_fScale);
		text.SetVisible(true);
		m_iTextCount++;
	}

	//------------------------------------------------------------------------------------------------
	//! Single line ending at right (estimated width)
	protected void TextRight(float right, float y, float size, string content, Color color)
	{
		float width = content.Length() * size * WRAP_CHAR_FACTOR;
		Text(right - width, y, width + size, size * 1.5, size, content, color);
	}

	//------------------------------------------------------------------------------------------------
	protected void Header(float x, float y, string content)
	{
		Text(x, y, 600, 34, 22, content, m_GreenDim, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void Stat(float x, float y, string label, string value)
	{
		Text(x, y, 340, 34, 22, label, m_GreenDim);
		Text(x + 340, y, 400, 34, 22, value, m_Green, true);
	}

	//------------------------------------------------------------------------------------------------
	//! action empty = not clickable
	protected void Button(float x, float y, float width, float height, string label, string action, int style, float size, bool centred = true)
	{
		if (m_iButtonCount >= BUTTON_POOL)
			return;

		Color fill = AROR_HiCommMap.Srgb(20, 44, 30, 1);
		Color labelColor = m_Green;
		if (style == STYLE_SELECTED)
		{
			fill = m_Green;
			labelColor = m_Dark;
		}
		else if (style == STYLE_PRIMARY)
		{
			fill = m_Amber;
			labelColor = m_Dark;
		}
		else if (style == STYLE_DISABLED)
		{
			fill = AROR_HiCommMap.Srgb(14, 24, 18, 1);
			labelColor = m_GreenFaint;
			action = string.Empty;
		}
		else if (style == STYLE_DANGER)
		{
			fill = AROR_HiCommMap.Srgb(70, 22, 20, 1);
			labelColor = m_Red;
		}
		else if (style == STYLE_MUTED)
		{
			fill = AROR_HiCommMap.Srgb(12, 24, 17, 1);
			labelColor = m_GreenDim;
		}
		else if (style == STYLE_PANEL)
		{
			fill = AROR_HiCommMap.Srgb(4, 12, 8, 0.93);
			labelColor = m_Green;
			action = string.Empty;
		}

		// Read-outs wrap to the box; multi-line labels ("\n") are measured by their longest line
		if (!centred)
			label = Wrap(label, width - 28, size);

		array<string> lines = {};
		label.Split("\n", lines, false);
		int longest;
		foreach (string line : lines)
		{
			longest = Math.Max(longest, line.Length());
		}

		int lineCount = Math.Max(lines.Count(), 1);
		int index = m_iButtonCount;
		ImageWidget background = m_aButtonBackgrounds[index];
		background.SetColor(fill);
		Place(background, x * m_fScale, y * m_fScale, width * m_fScale, height * m_fScale);
		background.SetVisible(true);

		RichTextWidget text = m_aButtonLabels[index];
		if (m_aButtonLabelCache[index] != label)
		{
			text.SetText(label);
			m_aButtonLabelCache[index] = label;
		}

		text.SetColor(labelColor);
		text.SetBold(style != STYLE_PANEL);
		text.SetExactFontSize(Math.Max(1, Math.Round(size * m_fScale)));
		float labelX = x + 14;
		if (centred)
			labelX = x + Math.Max((width - longest * size * CHAR_WIDTH_FACTOR) * 0.5, 8);

		float labelHeight = size * 1.3 * lineCount;
		Place(text, labelX * m_fScale, (y + (height - labelHeight) * 0.5) * m_fScale, (width - (labelX - x)) * m_fScale, (labelHeight + size * 0.2) * m_fScale);
		text.SetVisible(true);

		Widget button = m_aButtons[index];
		Place(button, x * m_fScale, y * m_fScale, width * m_fScale, height * m_fScale);
		button.SetVisible(!action.IsEmpty());
		m_aButtonActions[index] = action;
		m_iButtonCount++;
	}

	//------------------------------------------------------------------------------------------------
	protected void HideUnused()
	{
		for (int i = m_iPanelCount; i < PANEL_POOL; i++)
		{
			m_aPanels[i].SetVisible(false);
		}

		for (int i = m_iTextCount; i < TEXT_POOL; i++)
		{
			m_aTexts[i].SetVisible(false);
		}

		for (int i = m_iButtonCount; i < BUTTON_POOL; i++)
		{
			m_aButtonBackgrounds[i].SetVisible(false);
			m_aButtonLabels[i].SetVisible(false);
			m_aButtons[i].SetVisible(false);
			m_aButtonActions[i] = string.Empty;
		}

		for (int i = m_iMapLabelCount; i < MAP_LABEL_POOL; i++)
		{
			m_aMapLabels[i].SetVisible(false);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Data helpers
	//------------------------------------------------------------------------------------------------
	protected static int GetLocalPlayerId()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller)
			return 0;

		return controller.GetPlayerId();
	}

	//------------------------------------------------------------------------------------------------
	protected static string GetPlayerName(int playerId)
	{
		string name = GetGame().GetPlayerManager().GetPlayerName(playerId);
		if (name.IsEmpty())
			return "UNKNOWN";

		return name;
	}

	//------------------------------------------------------------------------------------------------
	//! Inserts line breaks so each line fits width (app units) at size, by estimated character width;
	//! existing line breaks are kept. The layout's text widgets do not wrap on their own.
	protected static string Wrap(string content, float width, float size)
	{
		int maxChars = Math.Max(Math.Floor(width / (size * WRAP_CHAR_FACTOR)), 4);
		if (content.Length() <= maxChars)
			return content;

		array<string> paragraphs = {};
		content.Split("\n", paragraphs, false);
		string result;
		foreach (int p, string paragraph : paragraphs)
		{
			if (p > 0)
				result += "\n";

			array<string> words = {};
			paragraph.Split(" ", words, false);
			string line;
			foreach (int w, string word : words)
			{
				if (w == 0)
					line = word;
				else if (line.Length() + 1 + word.Length() <= maxChars)
					line += " " + word;
				else
				{
					result += line + "\n";
					line = word;
				}
			}

			result += line;
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! string.ToUpper changes the string in place; this returns an upper-case copy
	protected static string Upper(string text)
	{
		text.ToUpper();
		return text;
	}

	//------------------------------------------------------------------------------------------------
	protected static string OwnerTag(int ownerId)
	{
		if (ownerId == 0)
			return "UNASSIGNED";

		return Upper(GetPlayerName(ownerId));
	}

	//------------------------------------------------------------------------------------------------
	protected static AROR_LocationComponent FindLocation(string locationId)
	{
		if (locationId.IsEmpty())
			return null;

		AROR_LocationManagerComponent locationManager = AROR_LocationManagerComponent.GetInstance();
		if (!locationManager)
			return null;

		return locationManager.GetLocationById(locationId);
	}

	//------------------------------------------------------------------------------------------------
	//! Flag-capturable military sites held by the Resistance, out of all of them
	protected static void CountSites(out int held, out int total)
	{
		held = 0;
		total = 0;
		AROR_LocationManagerComponent locationManager = AROR_LocationManagerComponent.GetInstance();
		if (!locationManager)
			return;

		array<AROR_LocationComponent> locations = {};
		locationManager.GetLocations(locations);
		foreach (AROR_LocationComponent location : locations)
		{
			if (!location)
				continue;

			AROR_ELocationType type = location.GetLocationType();
			if (type == AROR_ELocationType.HQ || type == AROR_ELocationType.SUPPORT_CORRIDOR)
				continue;

			total++;
			if (location.GetControllingRole() == AROR_EFactionRole.RESISTANCE)
				held++;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected string GetWarSummary()
	{
		AROR_OperationManagerComponent operations;
		IEntity gameMode = GetGame().GetGameMode();
		if (gameMode)
			operations = AROR_OperationManagerComponent.Cast(gameMode.FindComponent(AROR_OperationManagerComponent));

		if (!operations || !operations.HasWarState())
			return "OCC W-  //  INV W-";

		string occupiers = string.Format("OCC W%1 AGG %2", operations.GetWarLevel(AROR_EFactionRole.OCCUPIERS), operations.GetAggressionPercent(AROR_EFactionRole.OCCUPIERS)) + "%";
		string invaders = string.Format("INV W%1 AGG %2", operations.GetWarLevel(AROR_EFactionRole.INVADERS), operations.GetAggressionPercent(AROR_EFactionRole.INVADERS)) + "%";
		return occupiers + "  //  " + invaders;
	}

	//------------------------------------------------------------------------------------------------
	protected Color RoleColor(AROR_EFactionRole role)
	{
		if (role == AROR_EFactionRole.OCCUPIERS)
			return m_Occupiers;

		if (role == AROR_EFactionRole.INVADERS)
			return m_Invaders;

		return m_Green;
	}

	//------------------------------------------------------------------------------------------------
	//! Map colour of a role, packed for AROR_HiCommMap.Rgb
	protected static int RolePacked(AROR_EFactionRole role)
	{
		if (role == AROR_EFactionRole.OCCUPIERS)
			return PAL_OCCUPIERS;

		if (role == AROR_EFactionRole.INVADERS)
			return PAL_INVADERS;

		return PAL_GREEN;
	}

	//------------------------------------------------------------------------------------------------
	protected static string RoleName(AROR_EFactionRole role)
	{
		if (role == AROR_EFactionRole.OCCUPIERS)
			return "OCCUPIERS";

		if (role == AROR_EFactionRole.INVADERS)
			return "INVADERS";

		return "RESISTANCE";
	}

	//------------------------------------------------------------------------------------------------
	protected static string TypeName(AROR_ELocationType type)
	{
		string name = typename.EnumToString(AROR_ELocationType, type);
		name.Replace("_", " ");
		return name;
	}

	//------------------------------------------------------------------------------------------------
	protected static string ShortVerb(AROR_EHighCommandOrder order)
	{
		if (order == AROR_EHighCommandOrder.ATTACK)
			return "ATK";

		if (order == AROR_EHighCommandOrder.DEFEND)
			return "DEF";

		if (order == AROR_EHighCommandOrder.MOVE)
			return "MOVE";

		return "HOLD";
	}

	//------------------------------------------------------------------------------------------------
	protected static string TargetName(AROR_HighCommandSquadInfo squad)
	{
		AROR_LocationComponent target = FindLocation(squad.m_sTargetId);
		if (!target)
			return string.Empty;

		return Upper(target.GetName());
	}

	//------------------------------------------------------------------------------------------------
	protected static string ShortOrder(AROR_HighCommandSquadInfo squad)
	{
		if (!squad.m_bFormed)
			return "FORMING";

		if (squad.m_eOrder == AROR_EHighCommandOrder.NONE)
			return "AT HQ";

		if (squad.m_eOrder == AROR_EHighCommandOrder.MOVE || squad.m_eOrder == AROR_EHighCommandOrder.HOLD)
			return ShortVerb(squad.m_eOrder) + " " + AROR_HighCommandComponent.GridReference(squad.m_vTarget);

		return ShortVerb(squad.m_eOrder) + " " + TargetName(squad);
	}

	//------------------------------------------------------------------------------------------------
	protected static string LongOrder(AROR_HighCommandSquadInfo squad)
	{
		if (!squad.m_bFormed)
			return "FORMING UP AT HQ";

		if (squad.m_eOrder == AROR_EHighCommandOrder.ATTACK)
			return "ATTACKING " + TargetName(squad);

		if (squad.m_eOrder == AROR_EHighCommandOrder.DEFEND)
			return "DEFENDING " + TargetName(squad);

		if (squad.m_eOrder == AROR_EHighCommandOrder.MOVE)
			return "MOVING TO GRID " + AROR_HighCommandComponent.GridReference(squad.m_vTarget);

		if (squad.m_eOrder == AROR_EHighCommandOrder.HOLD)
			return "STANDING BY AT GRID " + AROR_HighCommandComponent.GridReference(squad.m_vTarget);

		return "HOLDING AT HQ";
	}

	//------------------------------------------------------------------------------------------------
	//! Destination marker for move orders
	protected void DrawCross(vector centre, float half, Color color)
	{
		m_Map.DrawLine(centre + Vector(-half, -half, 0), centre + Vector(half, half, 0), color, 2);
		m_Map.DrawLine(centre + Vector(-half, half, 0), centre + Vector(half, -half, 0), color, 2);
	}

	//------------------------------------------------------------------------------------------------
	protected static void Place(Widget widget, float x, float y, float width, float height)
	{
		FrameSlot.SetPos(widget, x, y);
		FrameSlot.SetSize(widget, width, height);
	}
}
