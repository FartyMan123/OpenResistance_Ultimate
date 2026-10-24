[ComponentEditorProps(category: "OpenResistance/Economy", description: "Periodic income from held locations. Server only. Attach to the game mode entity.")]
class AROR_IncomeComponentClass : SCR_BaseGameModeComponentClass
{
}

//! What one faction role holds, counted for an income tick
class AROR_IncomeTally : Managed
{
	int m_iTowns;
	int m_iCities;
	int m_iPopulation;	//!< Total population of held towns and cities
	int m_iResources;
	int m_iFactories;
	int m_iPorts;
}

//! Resource income (see ARCHITECTURE.md -> Economy). Every interval the server counts what each faction role holds and
//! pays it into the role's funds:
//!   money = resources x resource income x (1 + factory bonus per factory + port bonus per port)
//!         + population of held towns and cities x money per inhabitant (not multiplied)
//! The Resistance also gains manpower per town and per city held, and its players get a dispatch card.
//! No income while the campaign is still in setup.
class AROR_IncomeComponent : SCR_BaseGameModeComponent
{
	[Attribute("300", desc: "Seconds between income payments", params: "10 3600", category: "Income")]
	protected int m_iIntervalSeconds;

	[Attribute("500", desc: "Money per held resource location per payment", params: "0 inf", category: "Income")]
	protected int m_iResourceIncome;

	[Attribute("0.3", desc: "Added to the resource income multiplier for each held factory", params: "0 10", category: "Income")]
	protected float m_fFactoryBonus;

	[Attribute("0.5", desc: "Added to the resource income multiplier for each held port", params: "0 10", category: "Income")]
	protected float m_fPortBonus;

	[Attribute("5", desc: "Money per inhabitant of held towns and cities per payment (not multiplied)", params: "0 inf", category: "Income")]
	protected int m_iMoneyPerInhabitant;

	[Attribute("2", desc: "Resistance manpower per held town per payment", params: "0 inf", category: "Manpower")]
	protected int m_iManpowerPerTown;

	[Attribute("5", desc: "Resistance manpower per held city per payment", params: "0 inf", category: "Manpower")]
	protected int m_iManpowerPerCity;

	protected static AROR_IncomeComponent s_Instance;

	//------------------------------------------------------------------------------------------------
	static AROR_IncomeComponent GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	//! Server only. Pay every faction now (the timer calls this; the admin menu can too, for testing).
	void PayIncome_S()
	{
		if (!Replication.IsServer())
			return;

		AROR_GameFlowComponent gameFlow = AROR_GameFlowComponent.GetInstance();
		if (gameFlow && !gameFlow.IsPlaythroughReady())
			return;

		AROR_EconomyComponent economy = AROR_EconomyComponent.GetInstance();
		if (!economy)
		{
			Print("[AROR] Income: no AROR_EconomyComponent on the game mode", LogLevel.WARNING);
			return;
		}

		array<AROR_EFactionRole> roles = {AROR_EFactionRole.RESISTANCE, AROR_EFactionRole.OCCUPIERS, AROR_EFactionRole.INVADERS};
		foreach (AROR_EFactionRole role : roles)
		{
			AROR_IncomeTally tally = Tally(role);
			int money = GetMoney(tally);
			int manpower;
			if (role == AROR_EFactionRole.RESISTANCE)
				manpower = GetManpower(tally);

			economy.AddFactionFunds_S(role, money);
			if (manpower > 0)
				economy.AddManpower_S(manpower);

			Print(string.Format("[AROR] Income: %1 +%2 money, +%3 manpower (towns %4, cities %5, population %6, resources %7, factories %8, ports %9)",
				typename.EnumToString(AROR_EFactionRole, role), money, manpower,
				tally.m_iTowns, tally.m_iCities, tally.m_iPopulation, tally.m_iResources, tally.m_iFactories, tally.m_iPorts), LogLevel.NORMAL);

			if (role == AROR_EFactionRole.RESISTANCE)
				NotifyResistance(money, manpower);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Count the locations the role controls
	AROR_IncomeTally Tally(AROR_EFactionRole role)
	{
		AROR_IncomeTally tally = new AROR_IncomeTally();

		AROR_LocationManagerComponent locationManager = AROR_LocationManagerComponent.GetInstance();
		if (!locationManager)
			return tally;

		array<AROR_LocationComponent> locations = {};
		locationManager.GetLocations(locations);
		foreach (AROR_LocationComponent location : locations)
		{
			if (!location || location.GetControllingRole() != role)
				continue;

			switch (location.GetLocationType())
			{
				case AROR_ELocationType.TOWN:
					tally.m_iTowns++;
					tally.m_iPopulation += GetPopulation(location);
					break;
				case AROR_ELocationType.CITY:
					tally.m_iCities++;
					tally.m_iPopulation += GetPopulation(location);
					break;
				case AROR_ELocationType.RESOURCES:
					tally.m_iResources++;
					break;
				case AROR_ELocationType.FACTORY:
					tally.m_iFactories++;
					break;
				case AROR_ELocationType.PORT:
					tally.m_iPorts++;
					break;
			}
		}

		return tally;
	}

	//------------------------------------------------------------------------------------------------
	//! Money for one payment: boosted resource income plus population income
	int GetMoney(notnull AROR_IncomeTally tally)
	{
		float multiplier = 1 + m_fFactoryBonus * tally.m_iFactories + m_fPortBonus * tally.m_iPorts;
		float resourceMoney = tally.m_iResources * m_iResourceIncome * multiplier;
		int money = Math.Round(resourceMoney);
		return money + tally.m_iPopulation * m_iMoneyPerInhabitant;
	}

	//------------------------------------------------------------------------------------------------
	//! Resistance manpower for one payment
	int GetManpower(notnull AROR_IncomeTally tally)
	{
		return tally.m_iTowns * m_iManpowerPerTown + tally.m_iCities * m_iManpowerPerCity;
	}

	//------------------------------------------------------------------------------------------------
	protected static int GetPopulation(notnull AROR_LocationComponent location)
	{
		AROR_SettlementComponent settlement = AROR_SettlementComponent.Cast(location.GetOwner().FindComponent(AROR_SettlementComponent));
		if (!settlement)
			return 0;

		return settlement.GetPopulation();
	}

	//------------------------------------------------------------------------------------------------
	//! Dispatch card for the Resistance players (every player is Resistance)
	protected void NotifyResistance(int money, int manpower)
	{
		if (money <= 0 && manpower <= 0)
			return;

		Color color;
		AROR_FactionRolesComponent roles = AROR_FactionRolesComponent.GetInstance();
		if (roles && roles.GetFaction(AROR_EFactionRole.RESISTANCE))
			color = roles.GetFaction(AROR_EFactionRole.RESISTANCE).GetFactionColor();

		string text = string.Format("+%1 funds", AROR_EconomyComponent.FormatMoney(money));
		if (manpower > 0)
			text += string.Format(", +%1 manpower", manpower);

		AROR_NotificationManagerComponent.Send_S(new AROR_Notification(AROR_ENotificationType.NOTIFICATION, text, string.Empty, color, "Income"));
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode())
			return;

		s_Instance = this;

		if (Replication.IsServer())
			GetGame().GetCallqueue().CallLater(PayIncome_S, m_iIntervalSeconds * 1000, true);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(PayIncome_S);

		if (s_Instance == this)
			s_Instance = null;

		super.OnDelete(owner);
	}
}
