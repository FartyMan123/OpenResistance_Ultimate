//! Shared role authorization for campaign debug tools. A UI credential is not authority.
class AROR_AdminAccess
{
	static bool IsAdministrator(int playerId)
	{
		if (playerId <= 0)
			return false;
#ifdef WORKBENCH
		return true;
#else
		PlayerManager players = GetGame().GetPlayerManager();
		return players && (players.HasPlayerRole(playerId, EPlayerRole.ADMINISTRATOR) || players.HasPlayerRole(playerId, EPlayerRole.SESSION_ADMINISTRATOR));
#endif
	}
}
