//! Gameplay roles. Code refers to roles, never to faction keys; AROR_FactionRolesComponent maps roles to factions.
//! Append new values at the end so existing values stay stable.
enum AROR_EFactionRole
{
	RESISTANCE,
	OCCUPIERS,
	INVADERS,
	CIVILIANS	//!< Non-playable civilians; no support, income or capture
}
