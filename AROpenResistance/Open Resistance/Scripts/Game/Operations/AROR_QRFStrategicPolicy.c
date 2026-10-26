//! Pure, bounded decisions. The scheduler supplies validated evidence, never player positions.
class AROR_QRFStrategicPolicy
{
	static int PressureForLevel(int level)
	{
		switch (level)
		{
			case 2: return 100;
			case 3: return 250;
			case 4: return 450;
			case 5: return 700;
		}
		return 0;
	}
	static int WarLevel(int pressure)
	{
		for (int level = 5; level > 1; level--)
		{
			if (pressure >= PressureForLevel(level))
				return level;
		}
		return 1;
	}
	static int Importance(AROR_ELocationType type)
	{
		switch (type)
		{
			case AROR_ELocationType.CITY: return 140;
			case AROR_ELocationType.FACTORY: return 180;
			case AROR_ELocationType.AIRPORT: return 180;
			case AROR_ELocationType.MILITARY_BASE: return 160;
			case AROR_ELocationType.PORT: return 160;
			case AROR_ELocationType.OUTPOST: return 130;
			case AROR_ELocationType.RESOURCES: return 120;
		}
		return 100;
	}
	static int Priority(int importance, int reports, bool captured, int age)
	{
		int result = importance + Math.Min(6, Math.Max(0, reports)) * 15;
		if (captured)
			result += 80;
		// Waiting can help a quiet settlement, but never grows without bound.
		result += Math.Min(60, Math.Max(0, age) / 10);
		return result;
	}
	//! Aggression controls response size only. War level gates equipment separately.
	static int ResponseLevel(int aggression)
	{
		int bounded = Math.Clamp(aggression, 0, 1000);
		return Math.Min(5, 1 + Math.Floor(bounded / 200.0));
	}
	//! Compatibility seam for trusted callers. Threat context affects priority, not strength.
	static int DesiredTier(int aggressionLevel, int reports, bool captured, int roll, int importance = 100)
	{
		return Math.Clamp(aggressionLevel, 1, 5);
	}
	static int LossAggression(int value)
	{
		if (value <= 0)
			return 0;
		return Math.Clamp(Math.Floor(value / 100.0), 1, 30);
	}
	static int VehicleValue(AROR_EForceType type)
	{
		switch (type)
		{
			case AROR_EForceType.MOTORIZED: return 200;
			case AROR_EForceType.MECHANIZED: return 800;
			case AROR_EForceType.ARMOUR: return 1000;
			case AROR_EForceType.AIRBORNE: return 1600;
		}
		return 0;
	}
}
