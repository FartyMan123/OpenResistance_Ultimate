class AROR_ResponseScale
{
	//! Level probabilities are independent of how many profile variants exist for that level.
	static int GetEligibleWeight(AROR_ResponseLevel level, notnull array<ref AROR_ForceProfile> profiles, AROR_EFactionRole role, string factionKey, AROR_EOperationType type, int freeGroups, int freeMembers, int freeVehicles)
	{
		if (!level || !level.IsValid())
			return 0;
		foreach (AROR_ForceProfile profile : profiles)
		{
			if (AROR_ForcePool.GetEligibleWeight(profile, role, factionKey, type, freeGroups, freeMembers, freeVehicles, level.m_iLevel) > 0)
				return level.GetWeight(type);
		}
		return 0;
	}

	static int PickLevel(notnull array<ref AROR_ResponseLevel> levels, notnull array<ref AROR_ForceProfile> profiles, AROR_EFactionRole role, string factionKey, AROR_EOperationType type, int freeGroups, int freeMembers, int freeVehicles, int ticket)
	{
		if (ticket < 0)
			return 0;
		foreach (AROR_ResponseLevel level : levels)
		{
			int weight = GetEligibleWeight(level, profiles, role, factionKey, type, freeGroups, freeMembers, freeVehicles);
			if (ticket < weight)
				return level.m_iLevel;
			ticket -= weight;
		}
		return 0;
	}
}
