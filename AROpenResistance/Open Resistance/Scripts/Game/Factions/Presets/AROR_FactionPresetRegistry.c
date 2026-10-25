//! The list of faction presets available to campaigns (see AROR_FactionPreset).
//! Extender mods add their presets to this list with Override in addon, which stores only their additions.
[BaseContainerProps(configRoot: true)]
class AROR_FactionPresetRegistry
{
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Faction preset configs", params: "conf class=AROR_FactionPreset")]
	protected ref array<ResourceName> m_aPresets;

	//------------------------------------------------------------------------------------------------
	//! Adds the preset config paths to outPresets
	void GetPresetPaths(notnull array<ResourceName> outPresets)
	{
		if (m_aPresets)
			outPresets.InsertAll(m_aPresets);
	}
}
