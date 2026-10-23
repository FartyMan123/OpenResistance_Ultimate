[ComponentEditorProps(category: "OpenResistance/Locations", description: "Draws location icons on any open map (client only). Attach to the game mode entity.")]
class AROR_LocationMapIconsComponentClass : SCR_BaseGameModeComponentClass
{
}

//! Location markers (map marker layer 1): our own icon widgets drawn on whichever map is open
//! (player map, deploy screen, setup screen), positioned from world coordinates every frame while open.
//! Follows the location registry, so spawned locations (the HQ) get icons too. Never runs on a dedicated server.
class AROR_LocationMapIconsComponent : SCR_BaseGameModeComponent
{
	[Attribute("{6E2BA15FAEED49D7}UI/Layouts/Map/AROR_MapIcon.layout", UIWidgets.ResourceNamePicker, desc: "Layout for one icon; must contain an ImageWidget named 'Icon'", params: "layout")]
	protected ResourceName m_sIconLayout;

	[Attribute("32", UIWidgets.Slider, desc: "Icon size in reference pixels", params: "8 128 1")]
	protected float m_fIconSize;

	protected static const string ICON_WIDGET_NAME = "Icon";

	protected Widget m_wIconParent;
	protected ref array<AROR_LocationComponent> m_aIconLocations = {};
	protected ref array<Widget> m_aIconWidgets = {};

	//------------------------------------------------------------------------------------------------
	protected void OnMapOpen(MapConfiguration config)
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity || !mapEntity.GetMapWidget())
			return;

		ClearIcons();
		m_wIconParent = mapEntity.GetMapWidget().GetParent();
		if (!m_wIconParent)
			return;

		AROR_LocationManagerComponent locationManager = AROR_LocationManagerComponent.GetInstance();
		if (!locationManager)
			return;

		array<AROR_LocationComponent> locations = {};
		locationManager.GetLocations(locations);
		foreach (AROR_LocationComponent location : locations)
		{
			CreateIcon(location);
		}

		locationManager.GetOnLocationRegistered().Insert(CreateIcon);
		locationManager.GetOnLocationUnregistered().Insert(RemoveIcon);
		locationManager.GetOnLocationControlChanged().Insert(OnLocationControlChanged);

		// Per frame only while a map is open, so icons follow pan and zoom interpolation
		UpdateIconPositions();
		GetGame().GetCallqueue().CallLater(UpdateIconPositions, 0, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnMapClose(MapConfiguration config)
	{
		GetGame().GetCallqueue().Remove(UpdateIconPositions);

		AROR_LocationManagerComponent locationManager = AROR_LocationManagerComponent.GetInstance();
		if (locationManager)
		{
			locationManager.GetOnLocationRegistered().Remove(CreateIcon);
			locationManager.GetOnLocationUnregistered().Remove(RemoveIcon);
			locationManager.GetOnLocationControlChanged().Remove(OnLocationControlChanged);
		}

		ClearIcons();
	}

	//------------------------------------------------------------------------------------------------
	protected void CreateIcon(AROR_LocationComponent location)
	{
		if (!location || !m_wIconParent || m_aIconLocations.Contains(location))
			return;

		ResourceName icon = location.GetMapIcon();
		if (icon.IsEmpty())
			return;

		Widget root = GetGame().GetWorkspace().CreateWidgets(m_sIconLayout, m_wIconParent);
		if (!root)
			return;

		ImageWidget image = ImageWidget.Cast(root.FindAnyWidget(ICON_WIDGET_NAME));
		if (!image)
		{
			Print(string.Format("[AROR] Map icon layout has no ImageWidget named '%1'", ICON_WIDGET_NAME), LogLevel.ERROR);
			root.RemoveFromHierarchy();
			return;
		}

		image.LoadImageTexture(0, icon);
		ApplyControlColor(image, location);
		FrameSlot.SetSize(root, m_fIconSize, m_fIconSize);
		FrameSlot.SetAlignment(root, 0.5, 0.5);

		m_aIconLocations.Insert(location);
		m_aIconWidgets.Insert(root);
	}

	//------------------------------------------------------------------------------------------------
	protected void RemoveIcon(AROR_LocationComponent location)
	{
		int index = m_aIconLocations.Find(location);
		if (index < 0)
			return;

		if (m_aIconWidgets[index])
			m_aIconWidgets[index].RemoveFromHierarchy();

		m_aIconLocations.Remove(index);
		m_aIconWidgets.Remove(index);
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearIcons()
	{
		foreach (Widget widget : m_aIconWidgets)
		{
			if (widget)
				widget.RemoveFromHierarchy();
		}

		m_aIconLocations.Clear();
		m_aIconWidgets.Clear();
		m_wIconParent = null;
	}

	//------------------------------------------------------------------------------------------------
	//! Tint the icon with the controlling role's faction colour (icons must be white to show the tint)
	protected void ApplyControlColor(notnull ImageWidget image, notnull AROR_LocationComponent location)
	{
		AROR_FactionRolesComponent roles = AROR_FactionRolesComponent.GetInstance();
		if (!roles)
			return;

		Faction faction = roles.GetFaction(location.GetControllingRole());
		if (faction)
			image.SetColor(faction.GetFactionColor());
	}

	//------------------------------------------------------------------------------------------------
	protected void OnLocationControlChanged(AROR_LocationComponent location)
	{
		int index = m_aIconLocations.Find(location);
		if (index < 0 || !m_aIconWidgets[index])
			return;

		ImageWidget image = ImageWidget.Cast(m_aIconWidgets[index].FindAnyWidget(ICON_WIDGET_NAME));
		if (image)
			ApplyControlColor(image, location);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateIconPositions()
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		int screenX, screenY;

		foreach (int i, AROR_LocationComponent location : m_aIconLocations)
		{
			Widget widget = m_aIconWidgets[i];
			if (!location || !widget)
				continue;

			vector center = location.GetCenter();
			mapEntity.WorldToScreen(center[0], center[2], screenX, screenY, true);
			FrameSlot.SetPos(widget, workspace.DPIUnscale(screenX), workspace.DPIUnscale(screenY));
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode() || RplSession.Mode() == RplMode.Dedicated)
			return;

		SCR_MapEntity.GetOnMapOpen().Insert(OnMapOpen);
		SCR_MapEntity.GetOnMapClose().Insert(OnMapClose);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(UpdateIconPositions);
		SCR_MapEntity.GetOnMapOpen().Remove(OnMapOpen);
		SCR_MapEntity.GetOnMapClose().Remove(OnMapClose);
		ClearIcons();

		super.OnDelete(owner);
	}
}
