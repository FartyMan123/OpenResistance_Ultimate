[ComponentEditorProps(category: "OpenResistance/Locations", description: "Marks the owner entity as a map location (town, etc.). Place it via the AROR_Location_* prefabs.")]
class AROR_LocationComponentClass : ScriptComponentClass
{
}

//! A named area on the map. Registers itself with AROR_LocationManagerComponent when play starts.
//! The centre is the owner entity's position; the area is a circle of m_fRadius on the XZ plane.
//! Control: always held by a role. Starts as the initial role set on the placed location; the server changes it,
//! it replicates to clients and is saved (AROR_LocationComponentSerializer), so a loaded campaign restores it.
//! Replicated control needs an RplComponent on the location prefab.
class AROR_LocationComponent : ScriptComponent
{
	[Attribute("", UIWidgets.EditBox, desc: "Stable unique ID used by saves, e.g. \"everon_provins\". Never change it once saves exist.")]
	protected string m_sId;

	[Attribute("", UIWidgets.EditBox, desc: "Display name of the location, e.g. the town name")]
	protected string m_sName;

	[Attribute("0", UIWidgets.ComboBox, desc: "Type of location", enums: ParamEnumArray.FromEnum(AROR_ELocationType))]
	protected AROR_ELocationType m_eType;

	[Attribute("250", UIWidgets.Slider, desc: "Radius of the location in metres. Set per town in the World Editor.", params: "10 2000 5")]
	protected float m_fRadius;

	[Attribute("", UIWidgets.ResourcePickerThumbnail, desc: "Icon drawn on the map for this location. White on transparent so it can be tinted.", params: "edds imageset")]
	protected ResourceName m_sMapIcon;

	[Attribute("1", UIWidgets.ComboBox, desc: "Role controlling this location when a new campaign starts. Set per placed location.", enums: ParamEnumArray.FromEnum(AROR_EFactionRole), category: "Control")]
	protected AROR_EFactionRole m_eInitialControllingRole;

	[Attribute("0", desc: "Control can never change (e.g. the Resistance HQ)", category: "Control")]
	protected bool m_bControlFixed;

	[Attribute("120", UIWidgets.Slider, desc: "After the Resistance takes this location, seconds before it can flip to the Occupiers or Invaders", params: "0 3600 10", category: "Control")]
	protected float m_fFlipCooldownSeconds;

	//! True while the post-Resistance-capture flip cooldown runs; server-owned, replicated, not saved
	[RplProp()]
	protected bool m_bFlipLocked;

	[Attribute("0", UIWidgets.Slider, desc: "Number of inhabitants. Towns and cities only; ignored on other location types.", params: "0 100000 10")]
	protected int m_iPopulation;

	//! Current controlling role; server-owned, replicated, saved
	[RplProp(onRplName: "OnControllingRoleReplicated")]
	protected AROR_EFactionRole m_eControllingRole;

	//------------------------------------------------------------------------------------------------
	string GetId()
	{
		return m_sId;
	}

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
	ResourceName GetMapIcon()
	{
		return m_sMapIcon;
	}

	//------------------------------------------------------------------------------------------------
	//! Towns and cities only; always 0 for other location types
	int GetPopulation()
	{
		if (!HasPopulation())
			return 0;

		return m_iPopulation;
	}

	//------------------------------------------------------------------------------------------------
	bool HasPopulation()
	{
		return m_eType == AROR_ELocationType.TOWN || m_eType == AROR_ELocationType.CITY;
	}

	//------------------------------------------------------------------------------------------------
	AROR_EFactionRole GetControllingRole()
	{
		return m_eControllingRole;
	}

	//------------------------------------------------------------------------------------------------
	AROR_EFactionRole GetInitialControllingRole()
	{
		return m_eInitialControllingRole;
	}

	//------------------------------------------------------------------------------------------------
	bool IsControlFixed()
	{
		return m_bControlFixed;
	}

	//------------------------------------------------------------------------------------------------
	bool IsFlipLocked()
	{
		return m_bFlipLocked;
	}

	//------------------------------------------------------------------------------------------------
	//! False while control is fixed, already held by the role, or the flip cooldown blocks the role
	bool CanBeTakenBy(AROR_EFactionRole role)
	{
		if (m_bControlFixed || role == m_eControllingRole)
			return false;

		if (m_bFlipLocked && role != AROR_EFactionRole.RESISTANCE)
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Server only. Returns false if the change is not allowed (see CanBeTakenBy).
	bool SetControllingRole_S(AROR_EFactionRole role)
	{
		if (!Replication.IsServer() || !CanBeTakenBy(role))
			return false;

		m_eControllingRole = role;

		// Resistance captures lock the location against flipping back for a while
		if (role == AROR_EFactionRole.RESISTANCE && m_fFlipCooldownSeconds > 0)
		{
			m_bFlipLocked = true;
			GetGame().GetCallqueue().Remove(UnlockFlip);
			GetGame().GetCallqueue().CallLater(UnlockFlip, m_fFlipCooldownSeconds * 1000);
		}

		Replication.BumpMe();
		OnControllingRoleReplicated();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void UnlockFlip()
	{
		m_bFlipLocked = false;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	//! Server only: restore saved control, bypassing the fixed-control rule (used by the serializer)
	void RestoreControllingRole_S(AROR_EFactionRole role)
	{
		if (!Replication.IsServer() || role == m_eControllingRole)
			return;

		m_eControllingRole = role;
		Replication.BumpMe();
		OnControllingRoleReplicated();
	}

	//------------------------------------------------------------------------------------------------
	//! Runs on clients when the replicated value arrives, and on the server when it changes
	protected void OnControllingRoleReplicated()
	{
		AROR_LocationManagerComponent locationManager = AROR_LocationManagerComponent.GetInstance();
		if (locationManager)
			locationManager.GetOnLocationControlChanged().Invoke(this);
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

		// Every machine starts from the placed location's initial role; replication and saves override it
		m_eControllingRole = m_eInitialControllingRole;
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		if (!GetGame().InPlayMode())
			return;

		if (m_sId.IsEmpty())
			Print(string.Format("[AROR] Location '%1' has no ID and cannot be referenced by saves", m_sName), LogLevel.WARNING);

		if (m_iPopulation > 0 && !HasPopulation())
			Print(string.Format("[AROR] Location '%1' has a population but is not a town or city; it is ignored", m_sName), LogLevel.WARNING);

		Print(string.Format("[AROR] Location '%1' (%2) initialised (%3, radius %4 m)", m_sName, m_sId, typename.EnumToString(AROR_ELocationType, m_eType), m_fRadius), LogLevel.NORMAL);
		AROR_LocationManagerComponent.RegisterLocation(this);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(UnlockFlip);
		AROR_LocationManagerComponent.UnregisterLocation(this);
		super.OnDelete(owner);
	}
}
