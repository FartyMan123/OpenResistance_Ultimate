//! Session-only eligibility. A QRF must actually activate after this ownership baseline.
class AROR_TerritoryAttackState
{
	string m_sLocationId;
	int m_iCapturedAt;
	int m_iNextAttempt;
	int m_iQRFCompletedAt = -1;
	string m_sActiveQRF;

	bool IsDue(int now, int captureGrace, int afterQRF)
	{
		return m_iQRFCompletedAt >= 0 && now >= m_iCapturedAt + captureGrace && now >= m_iQRFCompletedAt + afterQRF && now >= m_iNextAttempt;
	}
}
