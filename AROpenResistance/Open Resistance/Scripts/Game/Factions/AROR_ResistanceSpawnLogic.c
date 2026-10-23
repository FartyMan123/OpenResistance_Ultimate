//! Auto-spawn that always assigns players to the faction filling the Resistance role (AROR_FactionRolesComponent).
//! Without a forced faction the base auto-spawn picks factions at random until one has a loadout.
//! Set it as the spawn logic of SCR_RespawnSystemComponent on the game mode.
[BaseContainerProps()]
class AROR_ResistanceSpawnLogic : SCR_AutoSpawnLogic
{
	//------------------------------------------------------------------------------------------------
	override protected bool GetForcedFaction(out Faction faction)
	{
		AROR_FactionRolesComponent roles = AROR_FactionRolesComponent.GetInstance();
		if (roles)
		{
			faction = roles.GetFaction(AROR_EFactionRole.RESISTANCE);
			if (faction)
				return true;
		}

		Print("[AROR] Resistance faction unavailable; falling back to the base auto-spawn faction choice", LogLevel.WARNING);
		return super.GetForcedFaction(faction);
	}
}
