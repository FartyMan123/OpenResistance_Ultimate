//! Pure eligibility/weighted-selection helpers; spawning remains server-owned in the executor.
class AROR_ForcePool
{
	static int GetEligibleWeight(AROR_ForceProfile profile, AROR_EFactionRole role, string factionKey, AROR_EOperationType type, int freeGroups, int freeMembers, int freeVehicles, int packages = 1)
	{
		if (!profile || !profile.m_bEnabled || !profile.IsValid() || !profile.HasNativeExecutor() || profile.m_eRole != role || profile.m_sFactionKey != factionKey)
			return 0;
		if (packages < 1 || packages > 5 || profile.GetGroupBudget() * packages > freeGroups || profile.m_iPersonnelBudget * packages > freeMembers)
			return 0;
		if (profile.m_eType != AROR_EForceType.INFANTRY && freeVehicles < packages)
			return 0;
		return profile.GetWeight(type);
	}

	//! Deterministic ticket selection permits boundary/eligibility tests without random sampling.
	static AROR_ForceProfile Pick(notnull array<ref AROR_ForceProfile> profiles, AROR_EFactionRole role, string factionKey, AROR_EOperationType type, int freeGroups, int freeMembers, int freeVehicles, int ticket, int packages = 1)
	{
		if (ticket < 0)
			return null;
		foreach (AROR_ForceProfile profile : profiles)
		{
			int weight = GetEligibleWeight(profile, role, factionKey, type, freeGroups, freeMembers, freeVehicles, packages);
			if (ticket < weight)
				return profile;
			ticket -= weight;
		}
		return null;
	}
}
