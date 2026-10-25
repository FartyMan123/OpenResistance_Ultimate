//! Saves a town or city's support, only when it differs from the starting 100% for the initial controlling role.
//! Same pattern as AROR_LocationComponentSerializer; the location entity needs a Persistence component.
class AROR_SettlementComponentSerializer : ScriptedComponentSerializer
{
	protected static const int VERSION = 1;

	//------------------------------------------------------------------------------------------------
	override static typename GetTargetType()
	{
		return AROR_SettlementComponent;
	}

	//------------------------------------------------------------------------------------------------
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull SaveContext context)
	{
		const AROR_SettlementComponent support = AROR_SettlementComponent.Cast(component);
		if (support.IsInitialSupport())
			return ESerializeResult.DEFAULT;

		context.WriteValue("version", VERSION);
		context.WriteValue("resistance", support.GetSupport(AROR_EFactionRole.RESISTANCE));
		context.WriteValue("occupiers", support.GetSupport(AROR_EFactionRole.OCCUPIERS));
		context.WriteValue("invaders", support.GetSupport(AROR_EFactionRole.INVADERS));
		return ESerializeResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull LoadContext context)
	{
		AROR_SettlementComponent support = AROR_SettlementComponent.Cast(component);

		int version;
		context.Read(version);

		float resistance, occupiers, invaders;
		if (context.Read(resistance) && context.Read(occupiers) && context.Read(invaders))
			support.RestoreSupport_S(resistance, occupiers, invaders);

		return true;
	}
}
