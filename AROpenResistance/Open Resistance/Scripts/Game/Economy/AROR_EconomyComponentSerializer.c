//! Saves faction funds and player money (AROR_EconomyComponent on the game mode, which has a Persistence component).
//! Nothing is written while funds are at their starting values and no player has money yet.
class AROR_EconomyComponentSerializer : ScriptedComponentSerializer
{
	protected static const int VERSION = 1;

	//------------------------------------------------------------------------------------------------
	override static typename GetTargetType()
	{
		return AROR_EconomyComponent;
	}

	//------------------------------------------------------------------------------------------------
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull SaveContext context)
	{
		const AROR_EconomyComponent economy = AROR_EconomyComponent.Cast(component);

		array<string> playerKeys = {};
		array<int> playerBalances = {};
		economy.GetPlayerMoneyEntries(playerKeys, playerBalances);

		if (economy.HasStartingFactionFunds() && playerKeys.IsEmpty())
			return ESerializeResult.DEFAULT;

		context.WriteValue("version", VERSION);
		context.WriteValue("resistance", economy.GetFactionFunds(AROR_EFactionRole.RESISTANCE));
		context.WriteValue("occupiers", economy.GetFactionFunds(AROR_EFactionRole.OCCUPIERS));
		context.WriteValue("invaders", economy.GetFactionFunds(AROR_EFactionRole.INVADERS));
		context.WriteValue("playerKeys", playerKeys);
		context.WriteValue("playerBalances", playerBalances);
		return ESerializeResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull LoadContext context)
	{
		AROR_EconomyComponent economy = AROR_EconomyComponent.Cast(component);

		int version;
		context.ReadValue("version", version);

		int resistance, occupiers, invaders;
		if (context.ReadValue("resistance", resistance) && context.ReadValue("occupiers", occupiers) && context.ReadValue("invaders", invaders))
			economy.RestoreFactionFunds_S(resistance, occupiers, invaders);

		array<string> playerKeys = {};
		array<int> playerBalances = {};
		if (context.ReadValue("playerKeys", playerKeys) && context.ReadValue("playerBalances", playerBalances))
			economy.RestorePlayerMoney_S(playerKeys, playerBalances);

		return true;
	}
}
