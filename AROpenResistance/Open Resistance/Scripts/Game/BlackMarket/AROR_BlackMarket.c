//! What the Black Market sells, as the app's tabs
enum AROR_EBlackMarketCategory
{
	VEHICLES,
	FIREARMS
}

//! One thing the Black Market sells. The prefab is what Rowan's virtual garage / arsenal will hand over.
class AROR_BlackMarketItem : Managed
{
	string m_sName;		//!< e.g. "UAZ-469"
	string m_sClass;	//!< e.g. "Light utility vehicle"
	AROR_EBlackMarketCategory m_eCategory;
	int m_iPrice;
	int m_iPortsRequired;	//!< Seaports the Resistance must hold: 1 Dockside, 2 Contraband, 3 Heavy Cargo
	ResourceName m_sPrefab;

	//------------------------------------------------------------------------------------------------
	void AROR_BlackMarketItem(string name, string itemClass, AROR_EBlackMarketCategory category, int price, int portsRequired, ResourceName prefab)
	{
		m_sName = name;
		m_sClass = itemClass;
		m_eCategory = category;
		m_iPrice = price;
		m_iPortsRequired = portsRequired;
		m_sPrefab = prefab;
	}
}

//! A paid order waiting for its collection point (the virtual garage / arsenal hookup)
class AROR_BlackMarketOrder : Managed
{
	int m_iItem;		//!< Index into the catalogue
	int m_iPrice;		//!< Price paid
	string m_sPlacedAt;	//!< Game time "HH:MM"
}

//! The Black Market (see ARCHITECTURE.md -> Black Market): a port-gated smugglers' shop in the phone.
//! Access needs at least one seaport held by the Resistance; each item also needs its own port count (the access
//! tiers Dockside, Contraband, Heavy Cargo). Players pay from their personal money. Orders are taken and paid on the
//! server and wait for a collection point: delivery into the virtual garage / arsenal is not built yet, so
//! GetOnOrderPlaced_S is the seam for it. The catalogue is the same on every machine; orders are kept per player
//! for the session and sent to their owner only.
//! Extender mods change the catalogue with a modded class overriding BuildCatalogue().
class AROR_BlackMarket
{
	static const int MAX_PORT_TIER = 3;
	protected static const int MAX_ORDERS_PER_PLAYER = 30;

	protected static ref array<ref AROR_BlackMarketItem> s_aCatalogue;

	// Server: orders per player key (identity ID, as for player money); session only
	protected static ref map<string, ref array<ref AROR_BlackMarketOrder>> s_mOrders;
	protected static ref ScriptInvoker s_OnOrderPlaced;

	// Client: the local player's orders as last sent by the server
	protected static ref array<ref AROR_BlackMarketOrder> s_aLocalOrders = {};

	//------------------------------------------------------------------------------------------------
	// Catalogue (every machine)
	//------------------------------------------------------------------------------------------------
	static array<ref AROR_BlackMarketItem> GetCatalogue()
	{
		if (!s_aCatalogue)
		{
			s_aCatalogue = {};
			BuildCatalogue(s_aCatalogue);
		}
		return s_aCatalogue;
	}

	//------------------------------------------------------------------------------------------------
	static AROR_BlackMarketItem GetItem(int index)
	{
		array<ref AROR_BlackMarketItem> catalogue = GetCatalogue();
		if (index < 0 || index >= catalogue.Count())
			return null;
		return catalogue[index];
	}

	//------------------------------------------------------------------------------------------------
	//! Prefabs are the base-game ones already used by the faction presets
	protected static void BuildCatalogue(notnull array<ref AROR_BlackMarketItem> items)
	{
		AROR_EBlackMarketCategory v = AROR_EBlackMarketCategory.VEHICLES;
		items.Insert(new AROR_BlackMarketItem("S1203 van", "Civilian van", v, 1200, 1, "{49C909AFD66E90A1}Prefabs/Vehicles/Wheeled/S1203/S1203_transport_randomized.et"));
		items.Insert(new AROR_BlackMarketItem("UAZ-452", "Utility van", v, 1500, 1, "{5F5AAFE8465D9D62}Prefabs/Vehicles/Wheeled/UAZ452/UAZ452_cargo_FIA.et"));
		items.Insert(new AROR_BlackMarketItem("UAZ-469", "Light utility vehicle", v, 1800, 1, "{F7E9AA0C813EABDA}Prefabs/Vehicles/Wheeled/UAZ469/UAZ469_FIA.et"));
		items.Insert(new AROR_BlackMarketItem("Ural-4320", "Troop truck", v, 3200, 2, "{16E32C3ABEAFC2C6}Prefabs/Vehicles/Wheeled/Ural4320/Ural4320_FIA_transport.et"));
		items.Insert(new AROR_BlackMarketItem("Ural-4320 ammo", "Supply truck", v, 4000, 2, "{3F5D19AC3C7E20DC}Prefabs/Vehicles/Wheeled/Ural4320/Ural4320_ammo_FIA.et"));
		items.Insert(new AROR_BlackMarketItem("UAZ-469 PKM", "Armed light vehicle", v, 4500, 2, "{22B327C6752EC4D4}Prefabs/Vehicles/Wheeled/UAZ469/UAZ469_PKM_FIA.et"));
		items.Insert(new AROR_BlackMarketItem("UAZ-469 UK59", "Armed light vehicle", v, 4800, 2, "{E72D78E7F45532EC}Prefabs/Vehicles/Wheeled/UAZ469/UAZ469_UK59_FIA.et"));
		items.Insert(new AROR_BlackMarketItem("M1025 M2HB", "Armed light vehicle", v, 7500, 3, "{3EA6F47D95867114}Prefabs/Vehicles/Wheeled/M998/M1025_armed_M2HB.et"));
		items.Insert(new AROR_BlackMarketItem("BRDM-2", "Armoured car", v, 12000, 3, "{442939C9617DF228}Prefabs/Vehicles/Wheeled/BRDM2/BRDM2_FIA.et"));
		items.Insert(new AROR_BlackMarketItem("BTR-70", "Armoured personnel carrier", v, 18000, 3, "{B47110AA1A806556}Prefabs/Vehicles/Wheeled/BTR70/BTR70_FIA.et"));
		items.Insert(new AROR_BlackMarketItem("UH-1H", "Utility helicopter", v, 22000, 3, "{7A29940A71EF5F5C}Prefabs/Vehicles/Helicopters/UH1H/UH1H_civ_livery_v1.et"));

		AROR_EBlackMarketCategory f = AROR_EBlackMarketCategory.FIREARMS;
		items.Insert(new AROR_BlackMarketItem("PM", "Pistol", f, 150, 1, "{C0F7DD85A86B2900}Prefabs/Weapons/Handguns/PM/Handgun_PM.et"));
		items.Insert(new AROR_BlackMarketItem("M9", "Pistol", f, 200, 1, "{1353C6EAD1DCFE43}Prefabs/Weapons/Handguns/M9/Handgun_M9.et"));
		items.Insert(new AROR_BlackMarketItem("vz. 58P", "Assault rifle", f, 450, 1, "{9C948630078D154D}Prefabs/Weapons/Rifles/VZ58/Rifle_VZ58P.et"));
		items.Insert(new AROR_BlackMarketItem("vz. 58V", "Assault rifle, folding stock", f, 500, 1, "{443CEFF17E040B11}Prefabs/Weapons/Rifles/VZ58/Rifle_VZ58V.et"));
		items.Insert(new AROR_BlackMarketItem("AKS-74U", "Carbine", f, 550, 1, "{BFEA719491610A45}Prefabs/Weapons/Rifles/AKS74U/Rifle_AKS74U.et"));
		items.Insert(new AROR_BlackMarketItem("AK-74", "Assault rifle", f, 600, 2, "{FA5C25BF66A53DCF}Prefabs/Weapons/Rifles/AK74/Rifle_AK74.et"));
		items.Insert(new AROR_BlackMarketItem("M16A2", "Assault rifle", f, 650, 2, "{3E413771E1834D2F}Prefabs/Weapons/Rifles/M16/Rifle_M16A2.et"));
		items.Insert(new AROR_BlackMarketItem("M72A3 LAW", "Disposable anti-tank launcher", f, 650, 2, "{9C5C20FB0E01E64F}Prefabs/Weapons/Launchers/M72/Launcher_M72A3.et"));
		items.Insert(new AROR_BlackMarketItem("RPG-75", "Disposable anti-tank launcher", f, 700, 2, "{7C45EC94C698246B}Prefabs/Weapons/Launchers/RPG75/Launcher_RPG75.et"));
		items.Insert(new AROR_BlackMarketItem("AK-74 GP-25", "Rifle with grenade launcher", f, 900, 2, "{63E8322E2ADD4AA7}Prefabs/Weapons/Rifles/AK74/Rifle_AK74_GP25.et"));
		items.Insert(new AROR_BlackMarketItem("M16A2 M203", "Rifle with grenade launcher", f, 950, 2, "{5A987A8A13763769}Prefabs/Weapons/Rifles/M16/Rifle_M16A2_M203.et"));
		items.Insert(new AROR_BlackMarketItem("SVD", "Marksman rifle", f, 1400, 2, "{3EB02CDAD5F23C82}Prefabs/Weapons/Rifles/SVD/Rifle_SVD.et"));
		items.Insert(new AROR_BlackMarketItem("M21", "Marksman rifle", f, 1500, 2, "{81EB948E6414BD6F}Prefabs/Weapons/Rifles/M14/Rifle_M21_ARTII.et"));
		items.Insert(new AROR_BlackMarketItem("RPG-7", "Anti-tank launcher", f, 1800, 3, "{7A82FE978603F137}Prefabs/Weapons/Launchers/RPG7/Launcher_RPG7.et"));
	}

	//------------------------------------------------------------------------------------------------
	// Access (every machine: location control is replicated)
	//------------------------------------------------------------------------------------------------
	//! Seaports the Resistance holds now
	static int CountHeldPorts()
	{
		AROR_LocationManagerComponent locations = AROR_LocationManagerComponent.GetInstance();
		if (!locations)
			return 0;

		array<AROR_LocationComponent> ports = {};
		locations.GetLocationsOfType(AROR_ELocationType.PORT, ports);
		int held;
		foreach (AROR_LocationComponent port : ports)
		{
			if (port && port.GetControllingRole() == AROR_EFactionRole.RESISTANCE)
				held++;
		}
		return held;
	}

	//------------------------------------------------------------------------------------------------
	//! Every seaport on the map, for the no-access screen
	static void GetPorts(notnull array<AROR_LocationComponent> outPorts)
	{
		AROR_LocationManagerComponent locations = AROR_LocationManagerComponent.GetInstance();
		if (locations)
			locations.GetLocationsOfType(AROR_ELocationType.PORT, outPorts);
	}

	//------------------------------------------------------------------------------------------------
	//! Access tier name for a port count (1 Dockside, 2 Contraband, 3+ Heavy Cargo); empty without access
	static string GetTierName(int ports)
	{
		if (ports >= 3)
			return "HEAVY CARGO";
		if (ports == 2)
			return "CONTRABAND";
		if (ports == 1)
			return "DOCKSIDE";
		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	// Ordering (server)
	//------------------------------------------------------------------------------------------------
	//! Invoked on the server for every paid order: (int playerId, AROR_BlackMarketItem item). The virtual garage /
	//! arsenal will hook in here to deliver it.
	static ScriptInvoker GetOnOrderPlaced_S()
	{
		if (!s_OnOrderPlaced)
			s_OnOrderPlaced = new ScriptInvoker();
		return s_OnOrderPlaced;
	}

	//------------------------------------------------------------------------------------------------
	//! Server: checks access, port tier and money, takes payment and records the order. Returns a message for the
	//! player (success or why it was refused).
	static string Order_S(int playerId, int itemIndex, out bool success)
	{
		success = false;
		if (!Replication.IsServer())
			return string.Empty;

		AROR_BlackMarketItem item = GetItem(itemIndex);
		if (!item)
			return "That item is no longer on offer.";
		if (!IsResistancePlayer(playerId))
			return "Only the Resistance can trade on the Black Market.";

		int ports = CountHeldPorts();
		if (ports < 1)
			return "No access terminal to the Black Market. Capture a seaport.";
		if (ports < item.m_iPortsRequired)
			return string.Format("%1 needs %2 seaports held.", item.m_sName, item.m_iPortsRequired);

		string key = PlayerKey(playerId);
		array<ref AROR_BlackMarketOrder> orders = GetOrders_S(key);
		if (orders.Count() >= MAX_ORDERS_PER_PLAYER)
			return "Too many orders waiting. Collect some first.";

		AROR_EconomyComponent economy = AROR_EconomyComponent.GetInstance();
		if (!economy || !economy.SpendPlayerMoney_S(playerId, item.m_iPrice))
			return string.Format("Not enough money for %1 ($%2).", item.m_sName, item.m_iPrice);

		AROR_BlackMarketOrder order = new AROR_BlackMarketOrder();
		order.m_iItem = itemIndex;
		order.m_iPrice = item.m_iPrice;
		order.m_sPlacedAt = GameTime();
		orders.Insert(order);
		success = true;

		Print(string.Format("[AROR] Black Market: player %1 ordered %2 for $%3", playerId, item.m_sName, item.m_iPrice), LogLevel.NORMAL);
		GetOnOrderPlaced_S().Invoke(playerId, item);
		return string.Format("Order placed: %1. Waiting for a collection point.", item.m_sName);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: the player's orders as parallel arrays for the owner RPC
	static void GetOrderRows_S(int playerId, notnull array<int> outItems, notnull array<int> outPrices, notnull array<string> outTimes)
	{
		array<ref AROR_BlackMarketOrder> orders = GetOrders_S(PlayerKey(playerId));
		foreach (AROR_BlackMarketOrder order : orders)
		{
			outItems.Insert(order.m_iItem);
			outPrices.Insert(order.m_iPrice);
			outTimes.Insert(order.m_sPlacedAt);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static array<ref AROR_BlackMarketOrder> GetOrders_S(string key)
	{
		if (!s_mOrders)
			s_mOrders = new map<string, ref array<ref AROR_BlackMarketOrder>>();
		array<ref AROR_BlackMarketOrder> orders = s_mOrders.Get(key);
		if (!orders)
		{
			orders = new array<ref AROR_BlackMarketOrder>();
			s_mOrders.Set(key, orders);
		}
		return orders;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsResistancePlayer(int playerId)
	{
		AROR_FactionRolesComponent roles = AROR_FactionRolesComponent.GetInstance();
		AROR_EFactionRole role;
		return roles && roles.GetRoleOfFaction(SCR_FactionManager.SGetPlayerFaction(playerId), role) && role == AROR_EFactionRole.RESISTANCE;
	}

	//------------------------------------------------------------------------------------------------
	//! Same key as player money: the Bohemia identity ID, else the player ID (local sessions)
	protected static string PlayerKey(int playerId)
	{
		BackendApi backend = GetGame().GetBackendApi();
		if (backend)
		{
			string identity = backend.GetPlayerIdentityId(playerId);
			if (!identity.IsEmpty())
				return identity;
		}
		return "player_" + playerId;
	}

	//------------------------------------------------------------------------------------------------
	protected static string GameTime()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world || !world.GetTimeAndWeatherManager())
			return "--:--";
		int hours, minutes, seconds;
		world.GetTimeAndWeatherManager().GetHoursMinutesSeconds(hours, minutes, seconds);
		return hours.ToString(2) + ":" + minutes.ToString(2);
	}

	//------------------------------------------------------------------------------------------------
	// Local player's orders (client)
	//------------------------------------------------------------------------------------------------
	static void SetLocalOrders(notnull array<int> items, notnull array<int> prices, notnull array<string> times)
	{
		s_aLocalOrders.Clear();
		foreach (int i, int item : items)
		{
			AROR_BlackMarketOrder order = new AROR_BlackMarketOrder();
			order.m_iItem = item;
			if (i < prices.Count())
				order.m_iPrice = prices[i];
			if (i < times.Count())
				order.m_sPlacedAt = times[i];
			s_aLocalOrders.Insert(order);
		}
	}

	//------------------------------------------------------------------------------------------------
	static array<ref AROR_BlackMarketOrder> GetLocalOrders()
	{
		return s_aLocalOrders;
	}
}
