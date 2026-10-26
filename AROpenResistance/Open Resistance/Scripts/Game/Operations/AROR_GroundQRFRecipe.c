//! Authored ground mix: level is a tier, and the element list is deployed exactly once.
[BaseContainerProps()]
class AROR_GroundQRFRecipe
{
	[Attribute("", desc: "Stable recipe ID, e.g. combined_arms_l1")]
	string m_sId;
	[Attribute("1", desc: "Authored response tier; does not multiply the element list", params: "1 5 1")]
	int m_iLevel;
	[Attribute("1", desc: "First war level using this equipment-pool recipe", params: "1 5 1")]
	int m_iMinimumWarLevel = 1;
	[Attribute("5", desc: "Last war level using this equipment-pool recipe", params: "1 5 1")]
	int m_iMaximumWarLevel = 5;
	[Attribute("1", UIWidgets.ComboBox, desc: "Responding campaign role", enums: AROR_FactionRoles.MilitaryRoleParams())]
	AROR_EFactionRole m_eRole;
	[Attribute("AROR_OCCUPIERS", desc: "Required faction from the shared role mapping")]
	string m_sFactionKey;
	[Attribute("", desc: "Ordered ground force-profile IDs. Each entry is one complete element; repeat an ID for additional vehicles.")]
	ref array<string> m_aProfileIds;

	bool IsValid()
	{
		if (m_iMinimumWarLevel < 1 || m_iMaximumWarLevel > 5 || m_iMinimumWarLevel > m_iMaximumWarLevel)
			return false;
		if (m_sId.IsEmpty() || m_sFactionKey.IsEmpty() || m_iLevel < 1 || m_iLevel > 5)
			return false;
		if (m_eRole != AROR_EFactionRole.OCCUPIERS && m_eRole != AROR_EFactionRole.INVADERS)
			return false;
		if (!m_aProfileIds || m_aProfileIds.Count() < 1 || m_aProfileIds.Count() > 8)
			return false;
		foreach (string id : m_aProfileIds)
		{
			if (id.IsEmpty())
				return false;
		}
		return true;
	}
	bool AllowsWarLevel(int warLevel)
	{
		return warLevel >= m_iMinimumWarLevel && warLevel <= m_iMaximumWarLevel;
	}

	//! Validate every element before any spawning or reservation conversion.
	bool Resolve(array<ref AROR_ForceProfile> profiles, AROR_EFactionRole role, string factionKey, array<AROR_ForceProfile> resolved, int warLevel = 5)
	{
		resolved.Clear();
		if (!IsValid() || m_eRole != role || m_sFactionKey != factionKey)
			return false;
		if (warLevel < 1 || warLevel > 5 || !AllowsWarLevel(warLevel))
			return false;
		foreach (string id : m_aProfileIds)
		{
			AROR_ForceProfile selected = null;
			foreach (AROR_ForceProfile profile : profiles)
			{
				if (profile && profile.m_sId == id)
					selected = profile;
			}
			if (!selected || !selected.IsValid() || !selected.m_bEnabled)
				return false;
			if (selected.m_iMinimumWarLevel > warLevel)
				return false;
			if (selected.m_eRole != role || selected.m_sFactionKey != factionKey)
				return false;
			if (!selected.HasNativeExecutor() || selected.GetWeight(AROR_EOperationType.QRF) <= 0)
				return false;
			resolved.Insert(selected);
		}
		return true;
	}
}
