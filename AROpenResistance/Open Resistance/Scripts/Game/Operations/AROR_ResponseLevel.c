[BaseContainerProps()]
class AROR_ResponseLevel
{
	[Attribute("1", desc: "Response strength, 1 to 5. Each level deploys that many force packages.")]
	int m_iLevel;
	[Attribute("20", desc: "Scheduled attack weight for this level; zero excludes it")]
	int m_iAttackWeight;
	[Attribute("20", desc: "Defensive QRF weight for this level; zero excludes it")]
	int m_iQRFWeight;

	bool IsValid()
	{
		return m_iLevel >= 1 && m_iLevel <= 5 && m_iAttackWeight >= 0 && m_iAttackWeight <= 1000 && m_iQRFWeight >= 0 && m_iQRFWeight <= 1000;
	}

	int GetWeight(AROR_EOperationType type)
	{
		if (type == AROR_EOperationType.QRF)
			return m_iQRFWeight;
		return m_iAttackWeight;
	}
}
