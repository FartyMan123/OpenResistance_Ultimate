//! Read-only portrait app. The operation manager owns and replicates its campaign pressure.
//! No client requests, threat/contact data or independent aggression simulation are introduced.
class AROR_RebIntelApp
{
	protected Widget m_wRoot;
	protected Widget m_wBackground;
	protected RichTextWidget m_wTitle;
	protected RichTextWidget m_wSubtitle;
	protected RichTextWidget m_wFooter;
	protected ref array<Widget> m_aCards = {};
	protected ref array<ImageWidget> m_aCardBackgrounds = {};
	protected ref array<RichTextWidget> m_aNames = {};
	protected ref array<RichTextWidget> m_aRoles = {};
	protected ref array<RichTextWidget> m_aWar = {};
	protected ref array<RichTextWidget> m_aAggression = {};
	protected ref array<RichTextWidget> m_aStatus = {};
	protected ref array<ImageWidget> m_aTracks = {};
	protected ref array<ImageWidget> m_aBars = {};
	protected ref array<int> m_aPercent = {0, 0};
	protected int m_iNextRefresh;

	bool Init(Widget phoneRoot)
	{
		m_wRoot = phoneRoot.FindAnyWidget("RebIntelApp");
		m_wBackground = phoneRoot.FindAnyWidget("RebIntelBackground");
		m_wTitle = RichTextWidget.Cast(phoneRoot.FindAnyWidget("RebIntelTitle"));
		m_wSubtitle = RichTextWidget.Cast(phoneRoot.FindAnyWidget("RebIntelSubtitle"));
		m_wFooter = RichTextWidget.Cast(phoneRoot.FindAnyWidget("RebIntelFooter"));
		if (!m_wRoot || !m_wBackground || !m_wTitle || !m_wSubtitle || !m_wFooter)
			return false;
		for (int i = 0; i < 2; i++)
		{
			string prefix = "RebIntelOccupiers";
			if (i == 1)
				prefix = "RebIntelInvaders";
			Widget card = phoneRoot.FindAnyWidget(prefix);
			ImageWidget background = ImageWidget.Cast(phoneRoot.FindAnyWidget(prefix + "Background"));
			RichTextWidget name = RichTextWidget.Cast(phoneRoot.FindAnyWidget(prefix + "Name"));
			RichTextWidget role = RichTextWidget.Cast(phoneRoot.FindAnyWidget(prefix + "Role"));
			RichTextWidget war = RichTextWidget.Cast(phoneRoot.FindAnyWidget(prefix + "War"));
			RichTextWidget aggression = RichTextWidget.Cast(phoneRoot.FindAnyWidget(prefix + "Aggression"));
			RichTextWidget status = RichTextWidget.Cast(phoneRoot.FindAnyWidget(prefix + "Status"));
			ImageWidget track = ImageWidget.Cast(phoneRoot.FindAnyWidget(prefix + "Track"));
			ImageWidget bar = ImageWidget.Cast(phoneRoot.FindAnyWidget(prefix + "Bar"));
			if (!card || !background || !name || !role || !war || !aggression || !status || !track || !bar)
				return false;
			m_aCards.Insert(card);
			m_aCardBackgrounds.Insert(background);
			m_aNames.Insert(name);
			m_aRoles.Insert(role);
			m_aWar.Insert(war);
			m_aAggression.Insert(aggression);
			m_aStatus.Insert(status);
			m_aTracks.Insert(track);
			m_aBars.Insert(bar);
			background.SetColor(Color.FromRGBA(25, 34, 43, 255));
			track.SetColor(Color.FromRGBA(58, 68, 78, 255));
			Style(name, string.Empty, true);
			Style(role, "Occupiers", false);
			if (i == 1)
				role.SetText("Invaders");
			Style(war, "War Level   -- / 5", true);
			Style(aggression, "Aggression   --", true);
			Style(status, "Awaiting intelligence", false);
			role.SetColor(Color.FromRGBA(168, 184, 196, 255));
			status.SetColor(Color.FromRGBA(168, 184, 196, 255));
		}
		m_wBackground.SetColor(Color.FromRGBA(13, 20, 27, 255));
		Style(m_wTitle, "RebIntel", true);
		Style(m_wSubtitle, "OPPOSING FORCES / OVERVIEW", false);
		m_wSubtitle.SetColor(Color.FromRGBA(103, 208, 171, 255));
		Style(m_wFooter, "Aggression reflects campaign pressure.\nWar level shapes response strength.", false);
		m_wFooter.SetColor(Color.FromRGBA(168, 184, 196, 255));
		SetVisible(false, 1);
		return true;
	}

	void SetVisible(bool visible, float opacity)
	{
		m_wRoot.SetVisible(visible);
		m_wRoot.SetOpacity(opacity);
		if (!visible)
			m_iNextRefresh = 0;
	}

	//! Geometry follows the phone animation; data is read at most once per second while open.
	void Layout(float left, float top, float scale)
	{
		int now = GetGame().GetWorld().GetWorldTime();
		if (now >= m_iNextRefresh)
		{
			Refresh();
			m_iNextRefresh = now + 1000;
		}
		Place(m_wRoot, left, top + 70 * scale, 900 * scale, 1630 * scale);
		Place(m_wBackground, 0, 0, 900 * scale, 1630 * scale);
		Label(m_wTitle, 60, 55, 780, 90, 68, scale);
		Label(m_wSubtitle, 60, 160, 780, 60, 29, scale);
		for (int i = 0; i < 2; i++)
		{
			Place(m_aCards[i], 45 * scale, (270 + i * 530) * scale, 810 * scale, 480 * scale);
			Place(m_aCardBackgrounds[i], 0, 0, 810 * scale, 480 * scale);
			Label(m_aNames[i], 36, 28, 738, 70, 54, scale, true);
			Label(m_aRoles[i], 36, 108, 738, 50, 32, scale);
			Label(m_aWar[i], 36, 195, 738, 65, 46, scale);
			Label(m_aAggression[i], 36, 280, 738, 60, 40, scale);
			Place(m_aTracks[i], 36 * scale, 358 * scale, 738 * scale, 16 * scale);
			Place(m_aBars[i], 36 * scale, 358 * scale, 738 * scale * m_aPercent[i] / 100.0, 16 * scale);
			m_aBars[i].SetVisible(m_aPercent[i] > 0);
			Label(m_aStatus[i], 36, 407, 738, 48, 30, scale);
		}
		Label(m_wFooter, 60, 1390, 780, 150, 30, scale);
	}

	protected void Refresh()
	{
		// The scheduler singleton deliberately exists only on the server. Clients read
		// its replicated component on the game mode, never a server-only singleton.
		AROR_OperationManagerComponent manager;
		IEntity gameMode = GetGame().GetGameMode();
		if (gameMode)
			manager = AROR_OperationManagerComponent.Cast(gameMode.FindComponent(AROR_OperationManagerComponent));
		AROR_InvaderCampaignComponent invasion;
		if (gameMode)
			invasion = AROR_InvaderCampaignComponent.Cast(gameMode.FindComponent(AROR_InvaderCampaignComponent));
		AROR_FactionRolesComponent roles = AROR_FactionRolesComponent.GetInstance();
		for (int i = 0; i < 2; i++)
		{
			AROR_EFactionRole role = AROR_EFactionRole.OCCUPIERS;
			Color accent = Color.FromRGBA(83, 155, 239, 255);
			if (i == 1)
			{
				role = AROR_EFactionRole.INVADERS;
				accent = Color.FromRGBA(236, 103, 103, 255);
			}
			m_aNames[i].SetText(ForceName(roles, role));
			if (roles && roles.GetFaction(role))
				accent = roles.GetFaction(role).GetFactionColor();
			m_aBars[i].SetColor(accent);
			m_aWar[i].SetText(WarText(manager, role));
			m_aAggression[i].SetText(AggressionText(manager, role));
			m_aStatus[i].SetText(ResponseText(manager, role));
			if (role == AROR_EFactionRole.INVADERS && invasion && invasion.IsAdvanceActive())
				m_aStatus[i].SetText("Territorial advance active");
			m_aPercent[i] = 0;
			if (manager && manager.HasWarState())
				m_aPercent[i] = manager.GetAggressionPercent(role);
		}
	}

	//! Default campaign labels match the current US/USSR forces; extenders use their configured faction name.
	protected static string ForceName(AROR_FactionRolesComponent roles, AROR_EFactionRole role)
	{
		if (roles)
		{
			string key = roles.GetFactionKey(role);
			if (key == "AROR_OCCUPIERS")
				return "US";
			if (key == "AROR_INVADERS")
				return "USSR";
			Faction faction = roles.GetFaction(role);
			if (faction)
				return faction.GetFactionName();
		}
		if (role == AROR_EFactionRole.OCCUPIERS)
			return "Occupiers";
		return "Invaders";
	}

	protected static string WarText(AROR_OperationManagerComponent manager, AROR_EFactionRole role)
	{
		if (!manager || !manager.HasWarState())
			return "War Level   -- / 5";
		return string.Format("War Level   %1 / 5", manager.GetWarLevel(role));
	}

	protected static string AggressionText(AROR_OperationManagerComponent manager, AROR_EFactionRole role)
	{
		if (!manager || !manager.HasWarState())
			return "Aggression   --";
		return string.Format("Aggression   %1", manager.GetAggressionPercent(role)) + "%";
	}

	protected static string ResponseText(AROR_OperationManagerComponent manager, AROR_EFactionRole role)
	{
		if (!manager || !manager.HasWarState())
			return "Awaiting intelligence";
		if (role == AROR_EFactionRole.INVADERS && !manager.AreInvaderOperationsEnabled())
			return "Responses disabled";
		if (!manager.IsQRFStrategyEnabled())
			return "Strategic selection inactive";
		return "Strategic selection active";
	}

	protected static void Style(RichTextWidget widget, string text, bool bold)
	{
		widget.SetText(text);
		widget.SetColor(Color.FromRGBA(240, 245, 248, 255));
		widget.SetBold(bold);
	}

	protected static void Label(RichTextWidget widget, float x, float y, float width, float height, float size, float scale, bool fit = false)
	{
		// Bound long extender faction names to the card rather than spilling beyond the phone.
		if (fit)
		{
			int length = widget.GetText().Length();
			if (length > 0)
				size = Math.Min(size, width / (length * 0.6));
		}
		widget.SetExactFontSize(Math.Max(1, Math.Round(size * scale)));
		Place(widget, x * scale, y * scale, width * scale, height * scale);
	}

	protected static void Place(Widget widget, float x, float y, float width, float height)
	{
		FrameSlot.SetPos(widget, x, y);
		FrameSlot.SetSize(widget, width, height);
	}
}
