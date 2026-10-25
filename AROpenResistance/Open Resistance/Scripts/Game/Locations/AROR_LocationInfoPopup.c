//! On-map info card for a location, shown next to its icon (driven by AROR_LocationMapIconsComponent).
//! Name, type and controlling faction; settlements add population and a support bar per faction.
//! The layout (AROR_MapInfoPopup.layout) only declares the widgets; positions, sizes and styling are set here.
class AROR_LocationInfoPopup : Managed
{
	// Card geometry in reference pixels
	protected static const float PADDING = 12;
	protected static const float ACCENT_WIDTH = 3;
	protected static const float TITLE_HEIGHT = 22;
	protected static const float LINE_HEIGHT = 18;
	protected static const float ROW_HEIGHT = 20;
	protected static const float LABEL_WIDTH = 82;
	protected static const float VALUE_WIDTH = 38;
	protected static const float BAR_HEIGHT = 6;

	protected static const int TITLE_FONT_SIZE = 18;
	protected static const int BODY_FONT_SIZE = 14;
	protected static const int SMALL_FONT_SIZE = 13;

	protected static const int SUPPORT_ROWS = 3;

	protected Widget m_wRoot;
	protected ImageWidget m_wBackground;
	protected ImageWidget m_wAccent;
	protected TextWidget m_wTitle;
	protected TextWidget m_wSubtitle;
	protected TextWidget m_wControl;
	protected ImageWidget m_wDivider;
	protected TextWidget m_wPopulation;
	protected ref array<TextWidget> m_aSupportLabels = {};
	protected ref array<ImageWidget> m_aSupportTracks = {};
	protected ref array<ImageWidget> m_aSupportFills = {};
	protected ref array<TextWidget> m_aSupportValues = {};

	protected AROR_LocationComponent m_Location;
	protected float m_fWidth;
	protected ref Color m_BackgroundColor;

	//------------------------------------------------------------------------------------------------
	void AROR_LocationInfoPopup(ResourceName layout, notnull Widget parent, float width, Color backgroundColor)
	{
		m_fWidth = width;
		m_BackgroundColor = backgroundColor;

		m_wRoot = GetGame().GetWorkspace().CreateWidgets(layout, parent);
		if (!m_wRoot)
			return;

		m_wBackground = ImageWidget.Cast(m_wRoot.FindAnyWidget("Background"));
		m_wAccent = ImageWidget.Cast(m_wRoot.FindAnyWidget("Accent"));
		m_wTitle = TextWidget.Cast(m_wRoot.FindAnyWidget("Title"));
		m_wSubtitle = TextWidget.Cast(m_wRoot.FindAnyWidget("Subtitle"));
		m_wControl = TextWidget.Cast(m_wRoot.FindAnyWidget("Control"));
		m_wDivider = ImageWidget.Cast(m_wRoot.FindAnyWidget("Divider"));
		m_wPopulation = TextWidget.Cast(m_wRoot.FindAnyWidget("Population"));

		for (int i = 0; i < SUPPORT_ROWS; i++)
		{
			m_aSupportLabels.Insert(TextWidget.Cast(m_wRoot.FindAnyWidget("SupportLabel" + i)));
			m_aSupportTracks.Insert(ImageWidget.Cast(m_wRoot.FindAnyWidget("SupportTrack" + i)));
			m_aSupportFills.Insert(ImageWidget.Cast(m_wRoot.FindAnyWidget("SupportFill" + i)));
			m_aSupportValues.Insert(TextWidget.Cast(m_wRoot.FindAnyWidget("SupportValue" + i)));
		}

		if (!m_wBackground || !m_wTitle || !m_wSubtitle || !m_wControl)
			Print("[AROR] Map info popup layout is missing widgets (Background, Title, Subtitle, Control)", LogLevel.ERROR);

		ApplyStyle();
		m_wRoot.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	void ~AROR_LocationInfoPopup()
	{
		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();
	}

	//------------------------------------------------------------------------------------------------
	AROR_LocationComponent GetLocation()
	{
		return m_Location;
	}

	//------------------------------------------------------------------------------------------------
	void Show(notnull AROR_LocationComponent location)
	{
		if (!m_wRoot)
			return;

		m_Location = location;
		Refresh();
		m_wRoot.SetVisible(true);
	}

	//------------------------------------------------------------------------------------------------
	void Hide()
	{
		m_Location = null;
		if (m_wRoot)
			m_wRoot.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	//! Position in reference (unscaled) pixels
	void SetPosition(float x, float y)
	{
		if (m_wRoot)
			FrameSlot.SetPos(m_wRoot, x, y);
	}

	//------------------------------------------------------------------------------------------------
	//! Fill in the card from the location, e.g. after control or support changed
	void Refresh()
	{
		if (!m_Location || !m_wTitle || !m_wSubtitle || !m_wControl)
			return;

		AROR_EFactionRole controllingRole = m_Location.GetControllingRole();
		Color controlColor = GetRoleColor(controllingRole);

		m_wTitle.SetText(m_Location.GetName());
		m_wSubtitle.SetText(AROR_TextUtils.FormatEnumName(typename.EnumToString(AROR_ELocationType, m_Location.GetLocationType())));
		m_wControl.SetText("Controlled by " + ColorText(GetRoleName(controllingRole), controlColor));

		if (m_wAccent && controlColor)
			m_wAccent.SetColor(controlColor);

		float left = PADDING + ACCENT_WIDTH;
		float contentWidth = m_fWidth - left - PADDING;

		float y = PADDING;
		Place(m_wTitle, left, y, contentWidth, TITLE_HEIGHT);
		y += TITLE_HEIGHT;
		Place(m_wSubtitle, left, y, contentWidth, LINE_HEIGHT);
		y += LINE_HEIGHT;
		Place(m_wControl, left, y, contentWidth, LINE_HEIGHT);
		y += LINE_HEIGHT;

		AROR_SettlementComponent settlement = AROR_SettlementComponent.Cast(m_Location.GetOwner().FindComponent(AROR_SettlementComponent));
		SetSettlementVisible(settlement != null);

		if (settlement)
		{
			y += 8;
			Place(m_wDivider, left, y, contentWidth, 1);
			y += 8;

			if (m_wPopulation)
			{
				m_wPopulation.SetText(string.Format("Population  %1", settlement.GetPopulation()));
				Place(m_wPopulation, left, y, contentWidth, LINE_HEIGHT);
			}

			y += LINE_HEIGHT + 4;

			array<AROR_EFactionRole> roles = {};
			AROR_FactionRoles.GetMilitary(roles);
			foreach (int i, AROR_EFactionRole role : roles)
			{
				LayoutSupportRow(i, role, settlement.GetSupport(role), left, y);
				y += ROW_HEIGHT;
			}
		}

		y += PADDING - 4;

		Place(m_wBackground, 0, 0, m_fWidth, y);
		Place(m_wAccent, 0, 0, ACCENT_WIDTH, y);
		FrameSlot.SetSize(m_wRoot, m_fWidth, y);
	}

	//------------------------------------------------------------------------------------------------
	protected void LayoutSupportRow(int index, AROR_EFactionRole role, float percent, float left, float y)
	{
		TextWidget label = m_aSupportLabels[index];
		ImageWidget track = m_aSupportTracks[index];
		ImageWidget fill = m_aSupportFills[index];
		TextWidget value = m_aSupportValues[index];
		if (!label || !track || !fill || !value)
			return;

		float barLeft = left + LABEL_WIDTH;
		float barWidth = m_fWidth - barLeft - VALUE_WIDTH - PADDING;
		float barY = y + (ROW_HEIGHT - BAR_HEIGHT) * 0.5 - 1;

		label.SetText(GetRoleName(role));
		Place(label, left, y, LABEL_WIDTH, ROW_HEIGHT);

		Place(track, barLeft, barY, barWidth, BAR_HEIGHT);
		Place(fill, barLeft, barY, barWidth * Math.Clamp(percent, 0, 100) / 100, BAR_HEIGHT);

		Color color = GetRoleColor(role);
		if (color)
			fill.SetColor(color);

		value.SetText(string.Format("%1", Math.Round(percent)) + "%");
		Place(value, barLeft + barWidth + 6, y, VALUE_WIDTH, ROW_HEIGHT);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetSettlementVisible(bool visible)
	{
		SetVisible(m_wDivider, visible);
		SetVisible(m_wPopulation, visible);

		for (int i = 0; i < SUPPORT_ROWS; i++)
		{
			SetVisible(m_aSupportLabels[i], visible);
			SetVisible(m_aSupportTracks[i], visible);
			SetVisible(m_aSupportFills[i], visible);
			SetVisible(m_aSupportValues[i], visible);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Colours, fonts and shadows; positions are set in Refresh
	protected void ApplyStyle()
	{
		if (m_wBackground && m_BackgroundColor)
			m_wBackground.SetColor(m_BackgroundColor);

		if (m_wDivider)
			m_wDivider.SetColor(Color.FromRGBA(255, 255, 255, 40));

		StyleText(m_wTitle, TITLE_FONT_SIZE, Color.FromRGBA(255, 255, 255, 255), true);
		StyleText(m_wSubtitle, SMALL_FONT_SIZE, Color.FromRGBA(160, 165, 170, 255), false);
		StyleText(m_wControl, BODY_FONT_SIZE, Color.FromRGBA(220, 220, 220, 255), false);
		StyleText(m_wPopulation, BODY_FONT_SIZE, Color.FromRGBA(220, 220, 220, 255), false);

		for (int i = 0; i < SUPPORT_ROWS; i++)
		{
			StyleText(m_aSupportLabels[i], SMALL_FONT_SIZE, Color.FromRGBA(200, 200, 200, 255), false);
			StyleText(m_aSupportValues[i], SMALL_FONT_SIZE, Color.FromRGBA(200, 200, 200, 255), false);

			if (m_aSupportTracks[i])
				m_aSupportTracks[i].SetColor(Color.FromRGBA(255, 255, 255, 30));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void StyleText(TextWidget text, int fontSize, Color color, bool bold)
	{
		if (!text)
			return;

		text.SetExactFontSize(fontSize);
		text.SetColor(color);
		text.SetBold(bold);
		text.SetShadow(1, 0xFF000000, 0.5, 1, 1);
	}

	//------------------------------------------------------------------------------------------------
	protected static void Place(Widget widget, float x, float y, float width, float height)
	{
		if (!widget)
			return;

		FrameSlot.SetPos(widget, x, y);
		FrameSlot.SetSize(widget, width, height);
	}

	//------------------------------------------------------------------------------------------------
	protected static void SetVisible(Widget widget, bool visible)
	{
		if (widget)
			widget.SetVisible(visible);
	}

	//------------------------------------------------------------------------------------------------
	protected Faction GetRoleFaction(AROR_EFactionRole role)
	{
		AROR_FactionRolesComponent roles = AROR_FactionRolesComponent.GetInstance();
		if (!roles)
			return null;

		return roles.GetFaction(role);
	}

	//------------------------------------------------------------------------------------------------
	//! Faction name filling the role (from the faction manager), falling back to the role name
	protected string GetRoleName(AROR_EFactionRole role)
	{
		Faction faction = GetRoleFaction(role);
		if (faction)
			return faction.GetFactionName();

		return AROR_FactionRoles.GetDisplayName(role);
	}

	//------------------------------------------------------------------------------------------------
	protected Color GetRoleColor(AROR_EFactionRole role)
	{
		Faction faction = GetRoleFaction(role);
		if (faction)
			return faction.GetFactionColor();

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static string ColorText(string text, Color color)
	{
		if (!color)
			return text;

		return string.Format("<color rgba=\"%1,%2,%3,255\">%4</color>", Math.Round(color.R() * 255), Math.Round(color.G() * 255), Math.Round(color.B() * 255), text);
	}
}
