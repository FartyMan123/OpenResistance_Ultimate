//! Saves the campaign's faction preset per role (AROR_FactionPresetsComponent on the game mode).
//! Loading is lenient: a missing or unknown preset falls back to the role's default instead of failing the load.
class AROR_FactionPresetsComponentSerializer : ScriptedComponentSerializer
{
	protected static const int VERSION = 1;

	//------------------------------------------------------------------------------------------------
	override static typename GetTargetType()
	{
		return AROR_FactionPresetsComponent;
	}

	//------------------------------------------------------------------------------------------------
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull SaveContext context)
	{
		const AROR_FactionPresetsComponent presets = AROR_FactionPresetsComponent.Cast(component);

		array<string> presetIds = {};
		presets.GetPresetIds(presetIds);

		context.WriteValue("version", VERSION);
		context.WriteValue("presetIds", presetIds);
		return ESerializeResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull LoadContext context)
	{
		AROR_FactionPresetsComponent presets = AROR_FactionPresetsComponent.Cast(component);

		int version;
		context.ReadValue("version", version);

		array<string> presetIds = {};
		context.ReadValue("presetIds", presetIds);
		presets.RestorePresets_S(presetIds);
		return true;
	}
}
