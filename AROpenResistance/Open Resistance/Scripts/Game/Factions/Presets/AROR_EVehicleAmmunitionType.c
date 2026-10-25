//! Faction preset vehicle ammunition types: munitions loaded onto vehicles and aircraft as items (e.g. helicopter
//! rocket pods). Vehicle-mounted guns rearm through the supply system, not from items. A preset can list many per type.
//! Fixed list used by the core: do not extend. Presets ignore entries whose type is not in this list.
enum AROR_EVehicleAmmunitionType
{
	ROCKET,
	MISSILE,
	BOMB
}
