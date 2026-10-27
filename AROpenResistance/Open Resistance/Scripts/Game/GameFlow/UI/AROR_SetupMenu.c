//! Forwards clicks and hovers on the setup menu's buttons
class AROR_SetupButtonHandler : ScriptedWidgetEventHandler
{
	protected AROR_SetupMenu m_Menu;

	//------------------------------------------------------------------------------------------------
	void AROR_SetupButtonHandler(AROR_SetupMenu menu)
	{
		m_Menu = menu;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_Menu)
			m_Menu.OnButton(w.GetName());
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (m_Menu)
			m_Menu.SetHover(w.GetName());
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (m_Menu)
			m_Menu.SetHover(string.Empty);
		return false;
	}
}

//! A square control: border, inset fill, label and an invisible button on top
class AROR_SetupControl : Managed
{
	ImageWidget m_wBorder;
	ImageWidget m_wFill;
	RichTextWidget m_wLabel;
	Widget m_wButton;

	//------------------------------------------------------------------------------------------------
	bool Bind(Widget root, string prefix)
	{
		m_wBorder = ImageWidget.Cast(root.FindAnyWidget(prefix + "Border"));
		m_wFill = ImageWidget.Cast(root.FindAnyWidget(prefix + "Fill"));
		m_wLabel = RichTextWidget.Cast(root.FindAnyWidget(prefix + "Label"));
		m_wButton = root.FindAnyWidget(prefix + "Button");
		return m_wBorder && m_wFill && m_wLabel && m_wButton;
	}

	//------------------------------------------------------------------------------------------------
	void SetVisible(bool visible)
	{
		m_wBorder.SetVisible(visible);
		m_wFill.SetVisible(visible);
		m_wLabel.SetVisible(visible);
		m_wButton.SetVisible(visible);
	}
}

//! Campaign setup screen for the server admin (see ARCHITECTURE.md -> Game Flow -> Setup UI), replacing the base-game
//! dialog for its first steps. START: three option cards (New, Load, Quick start). LOAD: the saved campaigns as rows,
//! six per page, with Load and Back. Choosing an action closes the menu and hands it to AROR_SetupUIComponent next frame,
//! as the setup dialogs do; Load campaign stays open and asks the server for the list.
//! Laid out on a 1920x1080 reference, scaled to the screen; styling follows the UI design system (docs/UI_DESIGN.md).
class AROR_SetupMenu : ChimeraMenuBase
{
	static const int SCREEN_START = 0;
	static const int SCREEN_LOAD = 1;
	//! Between a save's fields as the server sends them: name | save point | date | compatible (1/0)
	static const string FIELD_SEPARATOR = "|";

	protected static const float REF_WIDTH = 1920;
	protected static const float REF_HEIGHT = 1080;
	protected static const float LEFT = 160;
	protected static const float CONTENT_WIDTH = 1600;
	protected static const int ROWS = 6;

	protected static AROR_SetupMenu s_Open;

	protected ref AROR_SetupButtonHandler m_Handler;
	protected ImageWidget m_wBackground;
	protected ImageWidget m_wRule;
	protected RichTextWidget m_wKicker;
	protected RichTextWidget m_wTitle;
	protected RichTextWidget m_wSubtitle;
	protected RichTextWidget m_wMessage;
	protected RichTextWidget m_wFooter;
	protected RichTextWidget m_wPage;

	// Cards: New, Load, Quick
	protected ref array<string> m_aCardIds = {"New", "Load", "Quick"};
	protected ref array<ImageWidget> m_aCardBorders = {};
	protected ref array<ImageWidget> m_aCardFills = {};
	protected ref array<ImageWidget> m_aCardAccents = {};
	protected ref array<RichTextWidget> m_aCardNumbers = {};
	protected ref array<RichTextWidget> m_aCardNames = {};
	protected ref array<RichTextWidget> m_aCardBodies = {};
	protected ref array<RichTextWidget> m_aCardActions = {};
	protected ref array<Widget> m_aCardButtons = {};

	// Save rows
	protected ref array<ImageWidget> m_aRowBorders = {};
	protected ref array<ImageWidget> m_aRowFills = {};
	protected ref array<RichTextWidget> m_aRowNames = {};
	protected ref array<RichTextWidget> m_aRowDetails = {};
	protected ref array<RichTextWidget> m_aRowWarnings = {};
	protected ref array<Widget> m_aRowButtons = {};

	protected ref AROR_SetupControl m_Back = new AROR_SetupControl();
	protected ref AROR_SetupControl m_Load = new AROR_SetupControl();
	protected ref AROR_SetupControl m_Prev = new AROR_SetupControl();
	protected ref AROR_SetupControl m_Next = new AROR_SetupControl();

	protected int m_iScreen;
	protected ref array<string> m_aSaves = {};
	protected int m_iPage;
	protected int m_iSelected = -1;
	protected string m_sHover;
	protected string m_sMessage;
	protected bool m_bWaitingForList;

	// Action handed to the setup component once the menu has closed
	protected string m_sPendingDialog;
	protected string m_sPendingTag;
	protected int m_iPendingValue;

	//------------------------------------------------------------------------------------------------
	//! The open setup menu, or null
	static AROR_SetupMenu GetOpen()
	{
		return s_Open;
	}

	//------------------------------------------------------------------------------------------------
	void ShowStart(string message)
	{
		m_iScreen = SCREEN_START;
		m_sMessage = message;
		m_bWaitingForList = false;
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	void ShowLoad(notnull array<string> saves)
	{
		m_iScreen = SCREEN_LOAD;
		m_aSaves.Copy(saves);
		m_iPage = 0;
		m_iSelected = -1;
		if (!m_aSaves.IsEmpty())
			m_iSelected = 0;
		m_sMessage = string.Empty;
		m_bWaitingForList = false;
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	void SetHover(string buttonName)
	{
		m_sHover = buttonName;
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	void OnButton(string buttonName)
	{
		if (!m_sPendingTag.IsEmpty())
			return;

		if (m_iScreen == SCREEN_START)
		{
			if (buttonName == "SetupCardNewButton")
				CloseWith(AROR_SetupUIComponent.DIALOG_MAIN, "new", 0);
			else if (buttonName == "SetupCardQuickButton")
				CloseWith(AROR_SetupUIComponent.DIALOG_MAIN, "quick", 0);
			else if (buttonName == "SetupCardLoadButton")
				RequestSaves();
			return;
		}

		if (buttonName == "SetupBackButton")
		{
			ShowStart(string.Empty);
			return;
		}
		if (buttonName == "SetupPrevButton" && m_iPage > 0)
			m_iPage--;
		else if (buttonName == "SetupNextButton" && (m_iPage + 1) * ROWS < m_aSaves.Count())
			m_iPage++;
		else if (buttonName == "SetupLoadButton" && m_iSelected >= 0)
			CloseWith(AROR_SetupUIComponent.DIALOG_LOAD, "load", m_iSelected);

		foreach (int i, Widget rowButton : m_aRowButtons)
		{
			if (buttonName == rowButton.GetName() && m_iPage * ROWS + i < m_aSaves.Count())
				m_iSelected = m_iPage * ROWS + i;
		}
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void RequestSaves()
	{
		AROR_SetupUIComponent setup = AROR_SetupUIComponent.GetInstance();
		if (!setup)
			return;
		m_bWaitingForList = true;
		m_sMessage = string.Empty;
		Refresh();
		setup.RequestSaveList();
	}

	//------------------------------------------------------------------------------------------------
	protected void CloseWith(string dialogId, string tag, int value)
	{
		m_sPendingDialog = dialogId;
		m_sPendingTag = tag;
		m_iPendingValue = value;
		Close();
	}

	//------------------------------------------------------------------------------------------------
	//! Esc on the save list returns to the start screen; setup itself cannot be dismissed
	protected void OnBack(float value = 0.0, EActionTrigger reason = 0)
	{
		if (m_iScreen == SCREEN_LOAD)
			ShowStart(string.Empty);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		Widget root = GetRootWidget();
		m_wBackground = ImageWidget.Cast(root.FindAnyWidget("SetupBackground"));
		m_wRule = ImageWidget.Cast(root.FindAnyWidget("SetupRule"));
		m_wKicker = RichTextWidget.Cast(root.FindAnyWidget("SetupKicker"));
		m_wTitle = RichTextWidget.Cast(root.FindAnyWidget("SetupTitle"));
		m_wSubtitle = RichTextWidget.Cast(root.FindAnyWidget("SetupSubtitle"));
		m_wMessage = RichTextWidget.Cast(root.FindAnyWidget("SetupMessage"));
		m_wFooter = RichTextWidget.Cast(root.FindAnyWidget("SetupFooter"));
		m_wPage = RichTextWidget.Cast(root.FindAnyWidget("SetupPage"));
		bool ready = m_wBackground && m_wRule && m_wKicker && m_wTitle && m_wSubtitle && m_wMessage && m_wFooter && m_wPage;

		foreach (string id : m_aCardIds)
		{
			string card = "SetupCard" + id;
			m_aCardBorders.Insert(ImageWidget.Cast(root.FindAnyWidget(card + "Border")));
			m_aCardFills.Insert(ImageWidget.Cast(root.FindAnyWidget(card + "Fill")));
			m_aCardAccents.Insert(ImageWidget.Cast(root.FindAnyWidget(card + "Accent")));
			m_aCardNumbers.Insert(RichTextWidget.Cast(root.FindAnyWidget(card + "Number")));
			m_aCardNames.Insert(RichTextWidget.Cast(root.FindAnyWidget(card + "Name")));
			m_aCardBodies.Insert(RichTextWidget.Cast(root.FindAnyWidget(card + "Body")));
			m_aCardActions.Insert(RichTextWidget.Cast(root.FindAnyWidget(card + "Action")));
			m_aCardButtons.Insert(root.FindAnyWidget(card + "Button"));
		}
		for (int i = 0; i < ROWS; i++)
		{
			string row = "SetupRow" + i;
			m_aRowBorders.Insert(ImageWidget.Cast(root.FindAnyWidget(row + "Border")));
			m_aRowFills.Insert(ImageWidget.Cast(root.FindAnyWidget(row + "Fill")));
			m_aRowNames.Insert(RichTextWidget.Cast(root.FindAnyWidget(row + "Name")));
			m_aRowDetails.Insert(RichTextWidget.Cast(root.FindAnyWidget(row + "Detail")));
			m_aRowWarnings.Insert(RichTextWidget.Cast(root.FindAnyWidget(row + "Warn")));
			m_aRowButtons.Insert(root.FindAnyWidget(row + "Button"));
		}
		ready = ready && m_Back.Bind(root, "SetupBack") && m_Load.Bind(root, "SetupLoad") && m_Prev.Bind(root, "SetupPrev") && m_Next.Bind(root, "SetupNext");
		ready = ready && !m_aCardButtons.Contains(null) && !m_aRowButtons.Contains(null) && !m_aCardNames.Contains(null) && !m_aRowNames.Contains(null);
		ready = ready && !m_aCardBorders.Contains(null) && !m_aCardFills.Contains(null) && !m_aCardAccents.Contains(null) && !m_aCardNumbers.Contains(null);
		ready = ready && !m_aCardBodies.Contains(null) && !m_aCardActions.Contains(null) && !m_aRowBorders.Contains(null) && !m_aRowFills.Contains(null);
		ready = ready && !m_aRowDetails.Contains(null) && !m_aRowWarnings.Contains(null);
		if (!ready)
		{
			Print("[AROR] Setup menu layout is missing widgets", LogLevel.ERROR);
			Close();
			return;
		}

		ApplyStyle();
		m_Handler = new AROR_SetupButtonHandler(this);
		array<Widget> buttons = {m_Back.m_wButton, m_Load.m_wButton, m_Prev.m_wButton, m_Next.m_wButton};
		foreach (Widget cardButton : m_aCardButtons)
		{
			buttons.Insert(cardButton);
		}
		foreach (Widget rowButton : m_aRowButtons)
		{
			buttons.Insert(rowButton);
		}
		foreach (Widget button : buttons)
		{
			// Buttons only catch clicks; the engine draws them solid white unless cleared
			button.SetColor(Color.FromRGBA(0, 0, 0, 0));
			button.AddHandler(m_Handler);
		}

		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
		s_Open = this;

		// Start where the setup flow is (start screen, or the save list it already has)
		AROR_SetupUIComponent setup = AROR_SetupUIComponent.GetInstance();
		if (setup && setup.GetMenuScreen() == SCREEN_LOAD)
			ShowLoad(setup.GetSaveLabels());
		else if (setup)
			ShowStart(setup.GetMenuMessage());
		else
			ShowStart(string.Empty);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		super.OnMenuClose();
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
		if (s_Open == this)
			s_Open = null;

		AROR_SetupUIComponent setup = AROR_SetupUIComponent.GetInstance();
		if (setup && !m_sPendingTag.IsEmpty())
			setup.QueueDialogAction(m_sPendingDialog, m_sPendingTag, m_iPendingValue);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		if (s_Open == this)
			Layout();
	}

	//------------------------------------------------------------------------------------------------
	// Content
	//------------------------------------------------------------------------------------------------
	protected void Refresh()
	{
		if (s_Open != this)
			return;

		bool start = m_iScreen == SCREEN_START;
		m_wKicker.SetText("EVERON  ·  CAMPAIGN SETUP");
		if (start)
		{
			m_wTitle.SetText("OPEN RESISTANCE");
			m_wSubtitle.SetText("Start a new insurgency or pick up a saved one. Only the server admin sees this screen.");
		}
		else
		{
			m_wTitle.SetText("LOAD CAMPAIGN");
			m_wSubtitle.SetText(LoadSubtitle());
		}

		string message = m_sMessage;
		if (m_bWaitingForList)
			message = "Requesting saved campaigns from the server...";
		m_wMessage.SetText(message);
		int messageColor = AROR_UIColors.WARN;
		if (m_bWaitingForList)
			messageColor = AROR_UIColors.TEXT_MUTED;
		m_wMessage.SetColor(AROR_UIColors.Get(messageColor));

		foreach (int c, Widget cardButton : m_aCardButtons)
		{
			SetCardVisible(c, start);
			bool hot = start && m_sHover == cardButton.GetName();
			int border = AROR_UIColors.LINE;
			int fill = AROR_UIColors.SURFACE_RAISED;
			if (hot)
			{
				border = AROR_UIColors.REBEL;
				fill = AROR_UIColors.REBEL_DIM;
			}
			m_aCardBorders[c].SetColor(AROR_UIColors.Get(border));
			m_aCardFills[c].SetColor(AROR_UIColors.Get(fill));
		}

		int firstRow = m_iPage * ROWS;
		foreach (int r, Widget rowButton : m_aRowButtons)
		{
			int index = firstRow + r;
			bool shown = !start && index < m_aSaves.Count();
			SetRowVisible(r, shown);
			if (!shown)
				continue;

			string name, detail;
			bool compatible;
			ParseSave(m_aSaves[index], name, detail, compatible);
			m_aRowNames[r].SetText(name);
			m_aRowDetails[r].SetText(detail);
			m_aRowWarnings[r].SetText("INCOMPATIBLE");
			m_aRowWarnings[r].SetVisible(!compatible);

			int border = AROR_UIColors.LINE;
			int fill = AROR_UIColors.SURFACE_RAISED;
			int nameColor = AROR_UIColors.TEXT;
			if (index == m_iSelected)
			{
				border = AROR_UIColors.REBEL;
				fill = AROR_UIColors.REBEL_DIM;
				nameColor = AROR_UIColors.REBEL;
			}
			else if (m_sHover == rowButton.GetName())
			{
				border = AROR_UIColors.LINE_STRONG;
			}
			m_aRowBorders[r].SetColor(AROR_UIColors.Get(border));
			m_aRowFills[r].SetColor(AROR_UIColors.Get(fill));
			m_aRowNames[r].SetColor(AROR_UIColors.Get(nameColor));
		}

		// Controls on the save list: Back, paging, Load (primary)
		m_Back.SetVisible(!start);
		m_Load.SetVisible(!start);
		int pages = Math.Max(1, (m_aSaves.Count() + ROWS - 1) / ROWS);
		bool paged = !start && m_aSaves.Count() > ROWS;
		m_Prev.SetVisible(paged);
		m_Next.SetVisible(paged);
		m_wPage.SetVisible(paged);
		m_wPage.SetText(string.Format("PAGE %1 OF %2", m_iPage + 1, pages));
		StyleControl(m_Back, false, true);
		StyleControl(m_Prev, false, m_iPage > 0);
		StyleControl(m_Next, false, (m_iPage + 1) * ROWS < m_aSaves.Count());
		StyleControl(m_Load, true, m_iSelected >= 0);
	}

	//------------------------------------------------------------------------------------------------
	protected string LoadSubtitle()
	{
		int count = m_aSaves.Count();
		if (count == 0)
			return "No saved campaigns on this map yet. Go back and start a new one.";
		if (count == 1)
			return "1 saved campaign on this map, newest first.";
		return string.Format("%1 saved campaigns on this map, newest first.", count);
	}

	//------------------------------------------------------------------------------------------------
	//! "name|save point|date|compatible" to display lines; older plain labels show as the name
	protected static void ParseSave(string fields, out string name, out string detail, out bool compatible)
	{
		array<string> parts = {};
		fields.Split(FIELD_SEPARATOR, parts, false);
		name = fields;
		detail = string.Empty;
		compatible = true;
		if (parts.Count() >= 4)
		{
			name = parts[0];
			detail = parts[1] + "  ·  " + parts[2];
			compatible = parts[3] == "1";
		}
		name.ToUpper();
	}

	//------------------------------------------------------------------------------------------------
	//! Primary: rebel fill with ink text. Secondary: line-strong outline. Disabled: faint.
	protected void StyleControl(AROR_SetupControl control, bool primary, bool enabled)
	{
		bool hot = enabled && m_sHover == control.m_wButton.GetName();
		int border = AROR_UIColors.LINE_STRONG;
		int fill = AROR_UIColors.SURFACE;
		int text = AROR_UIColors.REBEL;
		if (primary && enabled)
		{
			border = AROR_UIColors.REBEL;
			fill = AROR_UIColors.REBEL;
			text = AROR_UIColors.REBEL_INK;
		}
		else if (!enabled)
		{
			border = AROR_UIColors.LINE;
			text = AROR_UIColors.TEXT_FAINT;
		}
		if (hot && !primary)
			border = AROR_UIColors.REBEL;
		if (hot && primary)
			fill = AROR_UIColors.TEXT;
		control.m_wBorder.SetColor(AROR_UIColors.Get(border));
		control.m_wFill.SetColor(AROR_UIColors.Get(fill));
		control.m_wLabel.SetColor(AROR_UIColors.Get(text));
		control.m_wButton.SetEnabled(enabled);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetCardVisible(int index, bool visible)
	{
		m_aCardBorders[index].SetVisible(visible);
		m_aCardFills[index].SetVisible(visible);
		m_aCardAccents[index].SetVisible(visible);
		m_aCardNumbers[index].SetVisible(visible);
		m_aCardNames[index].SetVisible(visible);
		m_aCardBodies[index].SetVisible(visible);
		m_aCardActions[index].SetVisible(visible);
		m_aCardButtons[index].SetVisible(visible);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetRowVisible(int index, bool visible)
	{
		m_aRowBorders[index].SetVisible(visible);
		m_aRowFills[index].SetVisible(visible);
		m_aRowNames[index].SetVisible(visible);
		m_aRowDetails[index].SetVisible(visible);
		m_aRowWarnings[index].SetVisible(visible);
		m_aRowButtons[index].SetVisible(visible);
	}

	//------------------------------------------------------------------------------------------------
	//! Texts and colours that never change
	protected void ApplyStyle()
	{
		m_wBackground.SetColor(AROR_UIColors.Get(0xF50E1210));
		m_wRule.SetColor(AROR_UIColors.Get(AROR_UIColors.LINE));
		Style(m_wKicker, AROR_UIColors.REBEL, true);
		Style(m_wTitle, AROR_UIColors.TEXT, true);
		Style(m_wSubtitle, AROR_UIColors.TEXT_MUTED, false);
		Style(m_wMessage, AROR_UIColors.WARN, true);
		Style(m_wFooter, AROR_UIColors.TEXT_FAINT, false);
		Style(m_wPage, AROR_UIColors.TEXT_MUTED, true);
		m_wFooter.SetText("OPEN RESISTANCE  ·  AN INSURGENCY CAMPAIGN ON EVERON");

		array<string> names = {"NEW CAMPAIGN", "LOAD CAMPAIGN", "QUICK START"};
		array<string> bodies = {
			"Choose the army for each side, then place the Resistance HQ on the map.",
			"Continue a saved campaign on this map from where it was left.",
			"US Army occupies, Soviet Army invades, FIA resists. Pick the HQ and go."
		};
		array<string> actions = {"CHOOSE ARMIES  >", "BROWSE SAVES  >", "PICK HQ  >"};
		foreach (int c, RichTextWidget cardName : m_aCardNames)
		{
			Style(m_aCardNumbers[c], AROR_UIColors.TEXT_MUTED, true);
			m_aCardNumbers[c].SetText(string.Format("0%1", c + 1));
			Style(cardName, AROR_UIColors.TEXT, true);
			cardName.SetText(names[c]);
			Style(m_aCardBodies[c], AROR_UIColors.TEXT_MUTED, false);
			m_aCardBodies[c].SetText(bodies[c]);
			m_aCardBodies[c].SetTextWrapping(true);
			Style(m_aCardActions[c], AROR_UIColors.REBEL, true);
			m_aCardActions[c].SetText(actions[c]);
			m_aCardAccents[c].SetColor(AROR_UIColors.Get(AROR_UIColors.REBEL));
		}
		foreach (int r, RichTextWidget rowName : m_aRowNames)
		{
			Style(rowName, AROR_UIColors.TEXT, true);
			Style(m_aRowDetails[r], AROR_UIColors.TEXT_MUTED, false);
			Style(m_aRowWarnings[r], AROR_UIColors.WARN, true);
		}
		m_Back.m_wLabel.SetText("BACK");
		m_Load.m_wLabel.SetText("LOAD CAMPAIGN");
		m_Prev.m_wLabel.SetText("<");
		m_Next.m_wLabel.SetText(">");
		array<RichTextWidget> controlLabels = {m_Back.m_wLabel, m_Load.m_wLabel, m_Prev.m_wLabel, m_Next.m_wLabel};
		foreach (RichTextWidget controlLabel : controlLabels)
		{
			controlLabel.SetBold(true);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Layout (1920x1080 reference, centred and scaled to the screen)
	//------------------------------------------------------------------------------------------------
	protected float m_fScale = 1;
	protected float m_fOffsetX;
	protected float m_fOffsetY;

	//------------------------------------------------------------------------------------------------
	protected void Layout()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		float width = workspace.DPIUnscale(workspace.GetWidth());
		float height = workspace.DPIUnscale(workspace.GetHeight());
		m_fScale = Math.Min(width / REF_WIDTH, height / REF_HEIGHT);
		m_fOffsetX = (width - REF_WIDTH * m_fScale) * 0.5;
		m_fOffsetY = (height - REF_HEIGHT * m_fScale) * 0.5;

		FrameSlot.SetPos(m_wBackground, 0, 0);
		FrameSlot.SetSize(m_wBackground, width, height);

		Text(m_wKicker, LEFT, 150, CONTENT_WIDTH, 30, 20);
		Text(m_wTitle, LEFT, 178, CONTENT_WIDTH, 100, 84);
		Text(m_wSubtitle, LEFT, 288, CONTENT_WIDTH, 36, 24);
		Box(m_wRule, LEFT, 342, CONTENT_WIDTH, 2);
		Text(m_wMessage, LEFT, 360, CONTENT_WIDTH, 30, 22);

		// Start: three cards
		float cardWidth = 500;
		float cardGap = (CONTENT_WIDTH - cardWidth * 3) / 2;
		foreach (int c, Widget cardButton : m_aCardButtons)
		{
			float x = LEFT + c * (cardWidth + cardGap);
			float y = 420;
			Box(m_aCardBorders[c], x, y, cardWidth, 320);
			Box(m_aCardFills[c], x + 2, y + 2, cardWidth - 4, 316);
			Box(m_aCardAccents[c], x + 2, y + 2, cardWidth - 4, 6);
			Text(m_aCardNumbers[c], x + 32, y + 34, 100, 28, 20);
			Text(m_aCardNames[c], x + 32, y + 68, cardWidth - 64, 52, 42);
			Text(m_aCardBodies[c], x + 32, y + 136, cardWidth - 64, 110, 22);
			Text(m_aCardActions[c], x + 32, y + 264, cardWidth - 64, 30, 20);
			Box(cardButton, x, y, cardWidth, 320);
		}

		// Load: rows, then the controls
		foreach (int r, Widget rowButton : m_aRowButtons)
		{
			float rowY = 384 + r * 88;
			Box(m_aRowBorders[r], LEFT, rowY, CONTENT_WIDTH, 78);
			Box(m_aRowFills[r], LEFT + 2, rowY + 2, CONTENT_WIDTH - 4, 74);
			Text(m_aRowNames[r], LEFT + 28, rowY + 10, CONTENT_WIDTH - 300, 36, 28);
			Text(m_aRowDetails[r], LEFT + 28, rowY + 46, CONTENT_WIDTH - 300, 28, 20);
			Text(m_aRowWarnings[r], LEFT + CONTENT_WIDTH - 220, rowY + 26, 200, 28, 20);
			Box(rowButton, LEFT, rowY, CONTENT_WIDTH, 78);
		}
		ControlLayout(m_Back, LEFT, 932, 260, 72, 22);
		ControlLayout(m_Prev, 680, 932, 100, 72, 26);
		Text(m_wPage, 800, 954, 320, 30, 20);
		ControlLayout(m_Next, 1140, 932, 100, 72, 26);
		ControlLayout(m_Load, LEFT + CONTENT_WIDTH - 320, 932, 320, 72, 22);

		Text(m_wFooter, LEFT, 1030, CONTENT_WIDTH, 26, 16);
	}

	//------------------------------------------------------------------------------------------------
	protected void ControlLayout(AROR_SetupControl control, float x, float y, float width, float height, float fontSize)
	{
		Box(control.m_wBorder, x, y, width, height);
		Box(control.m_wFill, x + 2, y + 2, width - 4, height - 4);
		float textWidth = AROR_TextUtils.CharCount(control.m_wLabel.GetText()) * fontSize * 0.6;
		Text(control.m_wLabel, x + (width - textWidth) * 0.5, y + (height - fontSize * 1.3) * 0.5, textWidth + fontSize, fontSize * 1.5, fontSize);
		Box(control.m_wButton, x, y, width, height);
	}

	//------------------------------------------------------------------------------------------------
	//! Reference-pixel rectangle to screen
	protected void Box(Widget widget, float x, float y, float width, float height)
	{
		FrameSlot.SetPos(widget, m_fOffsetX + x * m_fScale, m_fOffsetY + y * m_fScale);
		FrameSlot.SetSize(widget, width * m_fScale, height * m_fScale);
	}

	//------------------------------------------------------------------------------------------------
	protected void Text(RichTextWidget widget, float x, float y, float width, float height, float fontSize)
	{
		widget.SetExactFontSize(Math.Max(1, Math.Round(fontSize * m_fScale)));
		Box(widget, x, y, width, height);
	}

	//------------------------------------------------------------------------------------------------
	protected static void Style(RichTextWidget widget, int color, bool bold)
	{
		widget.SetColor(AROR_UIColors.Get(color));
		widget.SetBold(bold);
	}
}
