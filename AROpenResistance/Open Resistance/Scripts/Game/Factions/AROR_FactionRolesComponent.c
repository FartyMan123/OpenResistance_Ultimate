[ComponentEditorProps(category: "OpenResistance/Factions", description: "Maps gameplay roles (Resistance, Occupiers, Invaders) to faction keys. Attach to the game mode entity.")]
class AROR_FactionRolesComponentClass : SCR_BaseGameModeComponentClass
{
}

//! Role -> faction mapping (see ARCHITECTURE.md -> Factions).
//! Roles are gameplay; factions are data (identity, side, catalogs). Point a role at another faction key
//! (e.g. from an extender mod) to swap the army without changing code or the core factions.
class AROR_FactionRolesComponent : SCR_BaseGameModeComponent
{
	[Attribute("AROR_RESISTANCE", desc: "Faction key of the Resistance (the player faction)", category: "Roles")]
	protected string m_sResistanceFactionKey;

	[Attribute("AROR_OCCUPIERS", desc: "Faction key of the Occupiers (AI)", category: "Roles")]
	protected string m_sOccupiersFactionKey;

	[Attribute("AROR_INVADERS", desc: "Faction key of the Invaders (AI)", category: "Roles")]
	protected string m_sInvadersFactionKey;

	protected static AROR_FactionRolesComponent s_Instance;

	//------------------------------------------------------------------------------------------------
	static AROR_FactionRolesComponent GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	string GetFactionKey(AROR_EFactionRole role)
	{
		switch (role)
		{
			case AROR_EFactionRole.RESISTANCE:
				return m_sResistanceFactionKey;
			case AROR_EFactionRole.OCCUPIERS:
				return m_sOccupiersFactionKey;
			case AROR_EFactionRole.INVADERS:
				return m_sInvadersFactionKey;
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns the faction filling the role, or null if the key is not in the faction manager
	Faction GetFaction(AROR_EFactionRole role)
	{
		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
			return null;

		return factionManager.GetFactionByKey(GetFactionKey(role));
	}

	//------------------------------------------------------------------------------------------------
	//! Returns true and the role if the faction fills one
	bool GetRoleOfFaction(Faction faction, out AROR_EFactionRole role)
	{
		if (!faction)
			return false;

		string key = faction.GetFactionKey();
		array<AROR_EFactionRole> roles = {AROR_EFactionRole.RESISTANCE, AROR_EFactionRole.OCCUPIERS, AROR_EFactionRole.INVADERS};
		foreach (AROR_EFactionRole candidate : roles)
		{
			if (GetFactionKey(candidate) == key)
			{
				role = candidate;
				return true;
			}
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Warn early if a role points at a faction the faction manager does not have
	protected void ValidateRoles()
	{
		array<AROR_EFactionRole> roles = {AROR_EFactionRole.RESISTANCE, AROR_EFactionRole.OCCUPIERS, AROR_EFactionRole.INVADERS};
		foreach (AROR_EFactionRole role : roles)
		{
			if (!GetFaction(role))
				Print(string.Format("[AROR] Faction role %1 points at unknown faction '%2'", typename.EnumToString(AROR_EFactionRole, role), GetFactionKey(role)), LogLevel.ERROR);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode())
			return;

		s_Instance = this;

		// The faction manager may initialise after the game mode
		GetGame().GetCallqueue().CallLater(ValidateRoles, 1000);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(ValidateRoles);

		if (s_Instance == this)
			s_Instance = null;

		super.OnDelete(owner);
	}
}
