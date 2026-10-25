//! Saves a location's controlling role, only when it differs from the placed location's initial role.
//! Follows the base-game ScriptedComponentSerializer pattern (e.g. SCR_ArsenalManagerComponentSerializer).
//! The location entity needs a Persistence component for this to run.
class AROR_LocationComponentSerializer : ScriptedComponentSerializer
{
	protected static const int VERSION = 1;

	//------------------------------------------------------------------------------------------------
	override static typename GetTargetType()
	{
		return AROR_LocationComponent;
	}

	//------------------------------------------------------------------------------------------------
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull SaveContext context)
	{
		const AROR_LocationComponent location = AROR_LocationComponent.Cast(component);

		const AROR_EFactionRole controllingRole = location.GetControllingRole();
		const AROR_EFactionRole initialRole = location.GetInitialControllingRole();
		if (controllingRole == initialRole)
			return ESerializeResult.DEFAULT;

		context.WriteValue("version", VERSION);
		context.WriteDefault(controllingRole, initialRole);
		return ESerializeResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull LoadContext context)
	{
		AROR_LocationComponent location = AROR_LocationComponent.Cast(component);

		int version;
		context.Read(version);

		AROR_EFactionRole controllingRole;
		if (context.Read(controllingRole))
			location.RestoreControllingRole_S(controllingRole);

		return true;
	}
}
