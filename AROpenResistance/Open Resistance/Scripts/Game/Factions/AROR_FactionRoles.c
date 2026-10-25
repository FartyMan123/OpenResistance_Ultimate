//! The one place that defines which roles exist and how they group (see ARCHITECTURE.md -> Factions).
//! Use these instead of writing role lists by hand, so adding a role cannot leave a hand-written list behind.
class AROR_FactionRoles
{
	//------------------------------------------------------------------------------------------------
	//! Every role, in display order
	static void GetAll(notnull array<AROR_EFactionRole> outRoles)
	{
		outRoles.Insert(AROR_EFactionRole.RESISTANCE);
		outRoles.Insert(AROR_EFactionRole.OCCUPIERS);
		outRoles.Insert(AROR_EFactionRole.INVADERS);
		outRoles.Insert(AROR_EFactionRole.CIVILIANS);
	}

	//------------------------------------------------------------------------------------------------
	//! Roles that fight and hold territory: they control locations, have support, funds and income
	static void GetMilitary(notnull array<AROR_EFactionRole> outRoles)
	{
		outRoles.Insert(AROR_EFactionRole.RESISTANCE);
		outRoles.Insert(AROR_EFactionRole.OCCUPIERS);
		outRoles.Insert(AROR_EFactionRole.INVADERS);
	}

	//------------------------------------------------------------------------------------------------
	//! AI-controlled military roles (attacks, QRFs, AI recapture)
	static void GetAIMilitary(notnull array<AROR_EFactionRole> outRoles)
	{
		outRoles.Insert(AROR_EFactionRole.OCCUPIERS);
		outRoles.Insert(AROR_EFactionRole.INVADERS);
	}

	//------------------------------------------------------------------------------------------------
	//! Number of roles; arrays indexed by role must be at least this long
	static int Count()
	{
		array<AROR_EFactionRole> roles = {};
		GetAll(roles);
		return roles.Count();
	}

	//------------------------------------------------------------------------------------------------
	static bool IsMilitary(AROR_EFactionRole role)
	{
		return role != AROR_EFactionRole.CIVILIANS;
	}

	//------------------------------------------------------------------------------------------------
	//! "OCCUPIERS" -> "Occupiers"
	static string GetDisplayName(AROR_EFactionRole role)
	{
		return AROR_TextUtils.FormatEnumName(typename.EnumToString(AROR_EFactionRole, role));
	}

	//------------------------------------------------------------------------------------------------
	//! Editor dropdown of the military roles only, for attributes where Civilians make no sense
	//! (e.g. who controls a location). Use as `enums: AROR_FactionRoles.MilitaryRoleParams()`.
	static ParamEnumArray MilitaryRoleParams()
	{
		ParamEnumArray result = new ParamEnumArray();
		array<AROR_EFactionRole> roles = {};
		GetMilitary(roles);
		foreach (AROR_EFactionRole role : roles)
		{
			result.Insert(new ParamEnum(typename.EnumToString(AROR_EFactionRole, role), role.ToString()));
		}

		return result;
	}
}
