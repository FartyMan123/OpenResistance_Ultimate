//! One saved roster asset. Runtime entity refs are weak and are never serialized.
class AROR_GarrisonSlot
{
	ResourceName m_sPrefab;
	int m_iKind; // 0 infantry, 1 static MG, 2 parked vehicle, 3 manned vehicle, 4 crew
	int m_iGroup = -1;
	int m_iVehicle = -1;
	int m_iSeat = -1;
	bool m_bLost;
	float m_fHealth = 1;
	bool m_bPlaced;
	vector m_vPosition;
	vector m_vAngles;
	IEntity m_Entity;
	bool m_bSpawned;
	//! Placement failure is session-local, not a casualty or a permanent roster loss.
	int m_iPlacementFailureSince = -1;
	bool m_bPlacementUnavailable;
}
