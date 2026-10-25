//! Asset bindings are data, independent of the campaign role's faction identity.
[BaseContainerProps()]
class AROR_GarrisonFaction
{
	[Attribute("1", UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(AROR_EFactionRole))]
	AROR_EFactionRole m_eRole;
	[Attribute("AROR_OCCUPIERS")]
	string m_sFactionKey;
	[Attribute("US", desc: "Native source faction allowed during initial spawning only")]
	string m_sAssetFactionKey;
	[Attribute("{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et", UIWidgets.ResourceNamePicker, params: "et")]
	ResourceName m_sInfantry;
	[Attribute("{E1CB513B8B9B08F4}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Crew.et", UIWidgets.ResourceNamePicker, params: "et")]
	ResourceName m_sCrew;
	[Attribute("{73530808E4B455BA}Prefabs/Weapons/Tripods/Tripod_M3_M2HB.et", UIWidgets.ResourceNamePicker, params: "et")]
	ResourceName m_sStaticMG;
	[Attribute("{0FBF8F010F81A4E5}Prefabs/Vehicles/Wheeled/LAV25/LAV25.et", UIWidgets.ResourceNamePicker, params: "et")]
	ResourceName m_sParkedArmour;
	[Attribute("", desc: "Random manned defence choices. Every selected hull is fully crewed using native pilot/turret compartments.")]
	ref array<ResourceName> m_aMannedVehicles;
}
