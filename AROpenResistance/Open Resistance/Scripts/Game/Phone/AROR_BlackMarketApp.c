//! Black Market phone app (landscape; see AROR_BlackMarket and docs/UI_DESIGN.md -> Black Market).
//! Without a seaport held by the Resistance it shows only "NO ACCESS TERMINAL TO BLACK MARKET" and the ports to take.
//! With access: the access ladder (Dockside, Contraband, Heavy Cargo by ports held), tabs for Vehicles, Firearms and
//! the player's Orders, item rows (locked items stay visible with the reason) and a detail panel with the two-step
//! order (ORDER, then CONFIRM inline). Orders are paid and recorded on the server; collection points come with the
//! virtual garage / arsenal. Its widgets come from its own layout, created under the phone like MARS.
class AROR_BlackMarketApp : AROR_PhonePooledApp
{
	protected static const ResourceName LAYOUT = "{6AB5C0D7B1A4C3E2}UI/Layouts/Phone/AROR_BlackMarket.layout";

	protected static const int TAB_VEHICLES = 0;
	protected static const int TAB_FIREARMS = 1;
	protected static const int TAB_ORDERS = 2;
	protected static const int ROWS_PER_PAGE = 8;

	// Layout (app units, 1700 x 900)
	protected static const float MARGIN = 40;
	protected static const float LIST_WIDTH = 1020;
	protected static const float LIST_TOP = 252;
	protected static const float ROW_HEIGHT = 64;
	protected static const float ROW_GAP = 6;
	protected static const float DETAIL_LEFT = 1090;
	protected static const float DETAIL_WIDTH = 570;

	protected Widget m_wResource;
	protected int m_iTab = TAB_VEHICLES;
	protected int m_iSelected = -1;	//!< Catalogue index of the selected item
	protected int m_iPage;
	protected bool m_bConfirming;
	protected bool m_bShown;

	//------------------------------------------------------------------------------------------------
	bool Init(Widget phoneRoot)
	{
		m_wResource = GetGame().GetWorkspace().CreateWidgets(LAYOUT, phoneRoot);
		if (!m_wResource || !InitPools(m_wResource, "BM", 64, 80, 40))
			return false;
		m_wBackground.SetColor(AROR_UIColors.Get(AROR_UIColors.SURFACE));
		SetVisible(false, 1);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! The app's layout also holds the Black Market home tile (bound by the phone)
	Widget GetResource()
	{
		return m_wResource;
	}

	//------------------------------------------------------------------------------------------------
	void Shutdown()
	{
		if (m_wResource)
			m_wResource.RemoveFromHierarchy();
		m_wResource = null;
	}

	//------------------------------------------------------------------------------------------------
	//! Asks the server for the player's orders each time the app opens
	override void SetVisible(bool visible, float opacity)
	{
		super.SetVisible(visible, opacity);
		if (visible && !m_bShown)
		{
			m_bConfirming = false;
			SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
			if (controller)
				controller.AROR_RequestBlackMarketOrders();
		}
		m_bShown = visible;
	}

	//------------------------------------------------------------------------------------------------
	void Layout(float left, float top, float scale)
	{
		BeginFrame(left, top, scale, 1);
		int ports = AROR_BlackMarket.CountHeldPorts();

		DrawTopBar();
		if (ports < 1)
			DrawNoAccess();
		else
			DrawMarket(ports);

		EndFrame();
	}

	//------------------------------------------------------------------------------------------------
	// Screens
	//------------------------------------------------------------------------------------------------
	protected void DrawTopBar()
	{
		Text(MARGIN, 14, 600, 22, 15, "HICOMM  //  SMUGGLERS' NET", C(AROR_UIColors.TEXT_MUTED), true);
		Text(MARGIN, 34, 800, 50, 40, "BLACK MARKET", C(AROR_UIColors.TEXT), true);
		int money;
		AROR_EconomyComponent economy = AROR_EconomyComponent.GetInstance();
		if (economy)
			money = economy.GetLocalPlayerMoney();
		TextRight(WIDTH - MARGIN, 14, 15, "YOUR MONEY", C(AROR_UIColors.TEXT_MUTED), true);
		TextRight(WIDTH - MARGIN, 36, 32, Money(money), C(AROR_UIColors.REBEL), true);
		Panel(MARGIN, 88, WIDTH - MARGIN * 2, 2, C(AROR_UIColors.LINE));
	}

	//------------------------------------------------------------------------------------------------
	//! No seaport held: the terminal is dead, and the player is told which ports would open it
	protected void DrawNoAccess()
	{
		TextCentre(WIDTH * 0.5, 230, 54, "NO ACCESS TERMINAL TO BLACK MARKET", C(AROR_UIColors.DANGER), true);
		TextCentre(WIDTH * 0.5, 312, 22, "The smugglers only talk to a held seaport. Capture one to open the line.", C(AROR_UIColors.TEXT_MUTED));

		array<AROR_LocationComponent> ports = {};
		AROR_BlackMarket.GetPorts(ports);
		float y = 400;
		TextCentre(WIDTH * 0.5, y, 16, "SEAPORTS", C(AROR_UIColors.TEXT_MUTED), true);
		y += 34;
		foreach (AROR_LocationComponent port : ports)
		{
			if (!port)
				continue;
			string line = Upper(port.GetName()) + "  ·  HELD BY " + AROR_RebIntelHud.ArmyName(port.GetControllingRole());
			TextCentre(WIDTH * 0.5, y, 22, line, C(AROR_UIColors.TEXT));
			y += 36;
		}
		if (ports.IsEmpty())
			TextCentre(WIDTH * 0.5, y, 22, "No seaports on this map.", C(AROR_UIColors.TEXT_FAINT));

		DrawLadder(0, 760);
	}

	//------------------------------------------------------------------------------------------------
	protected void DrawMarket(int ports)
	{
		DrawLadder(ports, 104);
		DrawTabs();

		if (m_iTab == TAB_ORDERS)
		{
			DrawOrders();
			return;
		}

		array<int> items = {};
		ItemsInTab(m_iTab, items);
		if (m_iSelected < 0 || !items.Contains(m_iSelected))
		{
			m_iSelected = -1;
			if (!items.IsEmpty())
				m_iSelected = items[0];
			m_bConfirming = false;
		}

		int pages = Math.Max(1, (items.Count() + ROWS_PER_PAGE - 1) / ROWS_PER_PAGE);
		m_iPage = Math.ClampInt(m_iPage, 0, pages - 1);
		for (int row = 0; row < ROWS_PER_PAGE; row++)
		{
			int position = m_iPage * ROWS_PER_PAGE + row;
			if (position >= items.Count())
				break;
			DrawItemRow(items[position], LIST_TOP + row * (ROW_HEIGHT + ROW_GAP), ports);
		}
		DrawPager(pages);
		DrawDetail(AROR_BlackMarket.GetItem(m_iSelected), ports);
	}

	//------------------------------------------------------------------------------------------------
	//! Dockside / Contraband / Heavy Cargo: reached steps filled, the current one outlined
	protected void DrawLadder(int ports, float y)
	{
		float stepWidth = 300;
		for (int step = 1; step <= AROR_BlackMarket.MAX_PORT_TIER; step++)
		{
			float x = MARGIN + (step - 1) * (stepWidth + 12);
			bool reached = ports >= step;
			bool current = reached && (ports == step || (step == AROR_BlackMarket.MAX_PORT_TIER && ports > step));
			int fill = AROR_UIColors.SURFACE_RAISED;
			int text = AROR_UIColors.TEXT_FAINT;
			if (reached)
			{
				fill = AROR_UIColors.REBEL_DIM;
				text = AROR_UIColors.REBEL;
			}
			Panel(x, y, stepWidth, 56, C(fill));
			if (current)
				Frame(x, y, stepWidth, 56, 2, C(AROR_UIColors.REBEL));
			Text(x + 18, y + 6, stepWidth - 36, 26, 20, AROR_BlackMarket.GetTierName(step), C(text), true);
			string need = "1 SEAPORT";
			if (step > 1)
				need = string.Format("%1 SEAPORTS", step);
			Text(x + 18, y + 32, stepWidth - 36, 20, 14, need, C(AROR_UIColors.TEXT_MUTED));
		}
		string held = string.Format("SEAPORTS HELD  %1", ports);
		TextRight(WIDTH - MARGIN, y + 16, 22, held, C(AROR_UIColors.TEXT), true);
	}

	//------------------------------------------------------------------------------------------------
	protected void DrawTabs()
	{
		array<string> labels = {"VEHICLES", "FIREARMS", string.Format("ORDERS (%1)", AROR_BlackMarket.GetLocalOrders().Count())};
		foreach (int tab, string label : labels)
		{
			float x = MARGIN + tab * 232;
			if (tab == m_iTab)
				Button(x, 180, 220, 56, label, "tab:" + tab, C(AROR_UIColors.REBEL), C(AROR_UIColors.REBEL_INK), 20);
			else
			{
				Frame(x, 180, 220, 56, 2, C(AROR_UIColors.LINE_STRONG));
				Button(x + 2, 182, 216, 52, label, "tab:" + tab, C(AROR_UIColors.SURFACE), C(AROR_UIColors.REBEL), 20);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Name and class on the left, price on the right; locked items stay visible, dimmed, with the reason
	protected void DrawItemRow(int index, float y, int ports)
	{
		AROR_BlackMarketItem item = AROR_BlackMarket.GetItem(index);
		if (!item)
			return;
		bool locked = ports < item.m_iPortsRequired;
		bool selected = index == m_iSelected;

		int fill = AROR_UIColors.SURFACE_RAISED;
		if (selected)
			fill = AROR_UIColors.REBEL_DIM;
		Panel(MARGIN, y, LIST_WIDTH, ROW_HEIGHT, C(fill));
		if (selected)
			Frame(MARGIN, y, LIST_WIDTH, ROW_HEIGHT, 2, C(AROR_UIColors.REBEL));

		int nameColor = AROR_UIColors.TEXT;
		int priceColor = AROR_UIColors.REBEL;
		if (locked)
		{
			nameColor = AROR_UIColors.TEXT_FAINT;
			priceColor = AROR_UIColors.TEXT_FAINT;
		}
		Text(MARGIN + 20, y + 6, 600, 30, 24, item.m_sName, C(nameColor), true);
		Text(MARGIN + 20, y + 36, 600, 22, 16, item.m_sClass, C(AROR_UIColors.TEXT_MUTED));
		TextRight(MARGIN + LIST_WIDTH - 20, y + 6, 24, Money(item.m_iPrice), C(priceColor), true);
		if (locked)
			TextRight(MARGIN + LIST_WIDTH - 20, y + 38, 14, NeedsText(item), C(AROR_UIColors.WARN), true);
		HitArea(MARGIN, y, LIST_WIDTH, ROW_HEIGHT, "select:" + index);
	}

	//------------------------------------------------------------------------------------------------
	protected void DrawPager(int pages)
	{
		if (pages <= 1)
			return;
		float y = LIST_TOP + ROWS_PER_PAGE * (ROW_HEIGHT + ROW_GAP) + 4;
		int previous = AROR_UIColors.REBEL;
		if (m_iPage == 0)
			previous = AROR_UIColors.TEXT_FAINT;
		int next = AROR_UIColors.REBEL;
		if (m_iPage >= pages - 1)
			next = AROR_UIColors.TEXT_FAINT;
		Button(MARGIN, y, 80, 48, "<", "page:-1", C(AROR_UIColors.SURFACE_RAISED), C(previous), 24);
		TextCentre(MARGIN + LIST_WIDTH * 0.5, y + 12, 18, string.Format("PAGE %1 OF %2", m_iPage + 1, pages), C(AROR_UIColors.TEXT_MUTED), true);
		Button(MARGIN + LIST_WIDTH - 80, y, 80, 48, ">", "page:1", C(AROR_UIColors.SURFACE_RAISED), C(next), 24);
	}

	//------------------------------------------------------------------------------------------------
	//! The selected item: name, class, tier, price, collection; ORDER, then CONFIRM / CANCEL in place
	protected void DrawDetail(AROR_BlackMarketItem item, int ports)
	{
		float x = DETAIL_LEFT;
		float y = LIST_TOP;
		float height = 900 - LIST_TOP - 28;
		Panel(x, y, DETAIL_WIDTH, height, C(AROR_UIColors.SURFACE_RAISED));
		Frame(x, y, DETAIL_WIDTH, height, 1, C(AROR_UIColors.LINE));
		if (!item)
		{
			Text(x + 28, y + 28, DETAIL_WIDTH - 56, 30, 20, "Select an item.", C(AROR_UIColors.TEXT_MUTED));
			return;
		}

		string category = "VEHICLE";
		if (item.m_eCategory == AROR_EBlackMarketCategory.FIREARMS)
			category = "FIREARM";
		Text(x + 28, y + 24, DETAIL_WIDTH - 56, 22, 15, category, C(AROR_UIColors.TEXT_MUTED), true);
		Text(x + 28, y + 48, DETAIL_WIDTH - 56, 52, 42, item.m_sName, C(AROR_UIColors.TEXT), true);
		Text(x + 28, y + 104, DETAIL_WIDTH - 56, 28, 20, item.m_sClass, C(AROR_UIColors.TEXT_MUTED));

		bool locked = ports < item.m_iPortsRequired;
		int tierColor = AROR_UIColors.REBEL;
		if (locked)
			tierColor = AROR_UIColors.WARN;
		Fact(x + 28, y + 160, "TIER", AROR_BlackMarket.GetTierName(item.m_iPortsRequired), tierColor);
		Fact(x + 28, y + 220, "PRICE", Money(item.m_iPrice), AROR_UIColors.TEXT);
		Fact(x + 28, y + 280, "COLLECTION", "To be arranged", AROR_UIColors.TEXT_MUTED);
		Text(x + 28, y + 340, DETAIL_WIDTH - 56, 60, 16, Wrap("Paid orders wait for a collection point; vehicles and weapons will be handed over through the garage and arsenal.", DETAIL_WIDTH - 56, 16), C(AROR_UIColors.TEXT_FAINT));

		int money;
		AROR_EconomyComponent economy = AROR_EconomyComponent.GetInstance();
		if (economy)
			money = economy.GetLocalPlayerMoney();

		float buttonY = y + height - 100;
		string reason;
		if (locked)
			reason = NeedsText(item);
		else if (money < item.m_iPrice)
			reason = "NOT ENOUGH MONEY";

		if (!reason.IsEmpty())
		{
			m_bConfirming = false;
			Frame(x + 28, buttonY, DETAIL_WIDTH - 56, 72, 2, C(AROR_UIColors.LINE));
			TextCentre(x + DETAIL_WIDTH * 0.5, buttonY + 22, 22, reason, C(AROR_UIColors.WARN), true);
			return;
		}

		string price = Money(item.m_iPrice);
		if (!m_bConfirming)
		{
			Button(x + 28, buttonY, DETAIL_WIDTH - 56, 72, "ORDER " + price, "order", C(AROR_UIColors.REBEL), C(AROR_UIColors.REBEL_INK), 24);
			return;
		}

		// Two-step: the confirm row replaces the button in place, nothing pops over it
		float half = (DETAIL_WIDTH - 56 - 12) * 0.5;
		Frame(x + 28, buttonY, half, 72, 2, C(AROR_UIColors.LINE_STRONG));
		Button(x + 30, buttonY + 2, half - 4, 68, "CANCEL", "cancel", C(AROR_UIColors.SURFACE), C(AROR_UIColors.REBEL), 22);
		Button(x + 28 + half + 12, buttonY, half, 72, "CONFIRM " + price, "confirm", C(AROR_UIColors.REBEL), C(AROR_UIColors.REBEL_INK), 22);
	}

	//------------------------------------------------------------------------------------------------
	//! Paid orders, newest last, all waiting for a collection point
	protected void DrawOrders()
	{
		array<ref AROR_BlackMarketOrder> orders = AROR_BlackMarket.GetLocalOrders();
		if (orders.IsEmpty())
			Text(MARGIN, LIST_TOP, LIST_WIDTH, 30, 22, "No orders yet.", C(AROR_UIColors.TEXT_MUTED));

		int shown;
		for (int i = orders.Count() - 1; i >= 0 && shown < ROWS_PER_PAGE; i--)
		{
			AROR_BlackMarketOrder order = orders[i];
			AROR_BlackMarketItem item = AROR_BlackMarket.GetItem(order.m_iItem);
			if (!item)
				continue;
			float y = LIST_TOP + shown * (ROW_HEIGHT + ROW_GAP);
			Panel(MARGIN, y, LIST_WIDTH, ROW_HEIGHT, C(AROR_UIColors.SURFACE_RAISED));
			Text(MARGIN + 20, y + 6, 600, 30, 24, item.m_sName, C(AROR_UIColors.TEXT), true);
			Text(MARGIN + 20, y + 36, 600, 22, 16, string.Format("Paid %1 at %2", Money(order.m_iPrice), order.m_sPlacedAt), C(AROR_UIColors.TEXT_MUTED));
			TextRight(MARGIN + LIST_WIDTH - 20, y + 20, 16, "AWAITING COLLECTION POINT", C(AROR_UIColors.WARN), true);
			shown++;
		}

		float x = DETAIL_LEFT;
		float height = 900 - LIST_TOP - 28;
		Panel(x, LIST_TOP, DETAIL_WIDTH, height, C(AROR_UIColors.SURFACE_RAISED));
		Frame(x, LIST_TOP, DETAIL_WIDTH, height, 1, C(AROR_UIColors.LINE));
		Text(x + 28, LIST_TOP + 24, DETAIL_WIDTH - 56, 22, 15, "INBOUND", C(AROR_UIColors.TEXT_MUTED), true);
		Text(x + 28, LIST_TOP + 56, DETAIL_WIDTH - 56, 200, 18, Wrap("Your paid orders. Collection points open when the virtual garage and arsenal are linked; until then orders wait here. Orders last for this session.", DETAIL_WIDTH - 56, 18), C(AROR_UIColors.TEXT));
	}

	//------------------------------------------------------------------------------------------------
	protected void Fact(float x, float y, string label, string value, int valueColor)
	{
		Text(x, y, 200, 20, 14, label, C(AROR_UIColors.TEXT_MUTED), true);
		Text(x, y + 20, DETAIL_WIDTH - 56, 32, 26, value, C(valueColor), true);
	}

	//------------------------------------------------------------------------------------------------
	// Actions
	//------------------------------------------------------------------------------------------------
	override protected void DoAction(string action)
	{
		if (action.IndexOf("tab:") == 0)
		{
			m_iTab = action.Substring(4, action.Length() - 4).ToInt();
			m_iPage = 0;
			m_iSelected = -1;
			m_bConfirming = false;
		}
		else if (action.IndexOf("select:") == 0)
		{
			m_iSelected = action.Substring(7, action.Length() - 7).ToInt();
			m_bConfirming = false;
		}
		else if (action == "page:-1")
			m_iPage = Math.Max(0, m_iPage - 1);
		else if (action == "page:1")
			m_iPage++;
		else if (action == "order")
			m_bConfirming = true;
		else if (action == "cancel")
			m_bConfirming = false;
		else if (action == "confirm")
		{
			m_bConfirming = false;
			SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
			if (controller && m_iSelected >= 0)
				controller.AROR_RequestBlackMarketOrder(m_iSelected);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Helpers
	//------------------------------------------------------------------------------------------------
	protected static void ItemsInTab(int tab, notnull array<int> outItems)
	{
		AROR_EBlackMarketCategory category = AROR_EBlackMarketCategory.VEHICLES;
		if (tab == TAB_FIREARMS)
			category = AROR_EBlackMarketCategory.FIREARMS;
		array<ref AROR_BlackMarketItem> catalogue = AROR_BlackMarket.GetCatalogue();
		foreach (int i, AROR_BlackMarketItem item : catalogue)
		{
			if (item.m_eCategory == category)
				outItems.Insert(i);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static string NeedsText(AROR_BlackMarketItem item)
	{
		if (item.m_iPortsRequired == 1)
			return "NEEDS 1 SEAPORT";
		return string.Format("NEEDS %1 SEAPORTS", item.m_iPortsRequired);
	}

	//------------------------------------------------------------------------------------------------
	//! "$12,400"
	protected static string Money(int amount)
	{
		string digits = Math.AbsInt(amount).ToString();
		string grouped;
		int count = digits.Length();
		for (int i = 0; i < count; i++)
		{
			if (i > 0 && (count - i) % 3 == 0)
				grouped += ",";
			grouped += digits.Get(i);
		}
		if (amount < 0)
			return "-$" + grouped;
		return "$" + grouped;
	}

	//------------------------------------------------------------------------------------------------
	protected static Color C(int token)
	{
		return AROR_UIColors.Get(token);
	}
}
