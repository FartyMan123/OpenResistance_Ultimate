[ComponentEditorProps(category: "OpenResistance/Locations", description: "Marks the owner entity as a map location (town, etc.). Place it via the AROR_Location_* prefabs.")]
class AROR_LocationComponentClass : ScriptComponentClass
{
}

//! A named area on the map. Registers itself with AROR_LocationManagerComponent when play starts.
//! The centre is the owner entity's position; the area is a circle of m_fRadius on the XZ plane.
class AROR_LocationComponent : ScriptComponent
{
	[Attribute("", UIWidgets.EditBox, desc: "Display name of the location, e.g. the town name")]
	protected string m_sName;

	[Attribute("0", UIWidgets.ComboBox, desc: "Type of location", enums: ParamEnumArray.FromEnum(AROR_ELocationType))]
	protected AROR_ELocationType m_eType;

	[Attribute("250", UIWidgets.Slider, desc: "Radius of the location in metres. Set per town in the World Editor.", params: "10 2000 5")]
	protected float m_fRadius;

	//------------------------------------------------------------------------------------------------
	string GetName()
	{
		return m_sName;
	}

	//------------------------------------------------------------------------------------------------
	AROR_ELocationType GetLocationType()
	{
		return m_eType;
	}

	//------------------------------------------------------------------------------------------------
	float GetRadius()
	{
		return m_fRadius;
	}

	//------------------------------------------------------------------------------------------------
	vector GetCenter()
	{
		return GetOwner().GetOrigin();
	}

	//------------------------------------------------------------------------------------------------
	//! True if the position lies within the location's radius (height is ignored)
	bool IsPositionInside(vector position)
	{
		return vector.DistanceXZ(GetCenter(), position) <= m_fRadius;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		if (!GetGame().InPlayMode())
			return;

		Print(string.Format("[AROR] Location '%1' initialised (%2, radius %3 m)", m_sName, typename.EnumToString(AROR_ELocationType, m_eType), m_fRadius), LogLevel.NORMAL);
		AROR_LocationManagerComponent.RegisterLocation(this);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		AROR_LocationManagerComponent.UnregisterLocation(this);
		super.OnDelete(owner);
	}

#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	//! Draw the radius in the World Editor so it can be sized against the terrain
	override void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		Shape.CreateCylinder(ARGB(255, 255, 140, 0), ShapeFlags.WIREFRAME | ShapeFlags.ONCE, owner.GetOrigin(), m_fRadius, 60);
	}
#endif
}
