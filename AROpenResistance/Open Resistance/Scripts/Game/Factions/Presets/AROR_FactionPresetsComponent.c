[ComponentEditorProps(category: "OpenResistance/Factions", description: "Faction presets: loads the available presets and holds the campaign's preset per role. Attach to the game mode entity.")]
class AROR_FactionPresetsComponentClass : SCR_BaseGameModeComponentClass
{
}

void AROR_OnFactionPresetsChanged();
typedef func AROR_OnFactionPresetsChanged;

//! Faction presets (see ARCHITECTURE.md -> Factions). Every machine loads the presets listed in the registry, so clients
//! can show and query them without network traffic. The server owns which preset each role uses: chosen by the admin
//! when a campaign is created (defaults otherwise), replicated, and saved with the campaign
//! (AROR_FactionPresetsComponentSerializer). Other systems ask this component for a role's preset.
class AROR_FactionPresetsComponent : SCR_BaseGameModeComponent
{
	[Attribute("{2D8E5A71C4B93F06}Configs/FactionPresets/AROR_FactionPresetRegistry.conf", UIWidgets.ResourceNamePicker, desc: "Registry listing the available faction presets", params: "conf class=AROR_FactionPresetRegistry")]
	protected ResourceName m_sRegistry;

	[Attribute("FIA", desc: "Preset ID the Resistance uses unless the admin picks another", category: "Defaults")]
	protected string m_sDefaultResistance;

	[Attribute("US_ARMY", desc: "Preset ID the Occupiers use unless the admin picks another", category: "Defaults")]
	protected string m_sDefaultOccupiers;

	[Attribute("USSR_ARMY", desc: "Preset ID the Invaders use unless the admin picks another", category: "Defaults")]
	protected string m_sDefaultInvaders;

	[Attribute("EVERON_CIV", desc: "Preset ID the Civilians use unless the admin picks another", category: "Defaults")]
	protected string m_sDefaultCivilians;

	protected static AROR_FactionPresetsComponent s_Instance;

	//! All loaded presets, in registry order
	protected ref array<ref AROR_FactionPreset> m_aPresets = {};

	// The campaign's preset ID per role (server-owned, replicated)
	[RplProp(onRplName: "OnPresetsReplicated")]
	protected string m_sResistancePreset;

	[RplProp(onRplName: "OnPresetsReplicated")]
	protected string m_sOccupiersPreset;

	[RplProp(onRplName: "OnPresetsReplicated")]
	protected string m_sInvadersPreset;

	[RplProp(onRplName: "OnPresetsReplicated")]
	protected string m_sCiviliansPreset;

	protected ref ScriptInvokerBase<AROR_OnFactionPresetsChanged> m_OnPresetsChanged = new ScriptInvokerBase<AROR_OnFactionPresetsChanged>();

	//------------------------------------------------------------------------------------------------
	static AROR_FactionPresetsComponent GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	//! Every machine; fires when the campaign's presets change (set at campaign creation, restored from a save, or replicated)
	ScriptInvokerBase<AROR_OnFactionPresetsChanged> GetOnPresetsChanged()
	{
		return m_OnPresetsChanged;
	}

	//------------------------------------------------------------------------------------------------
	// Queries
	//------------------------------------------------------------------------------------------------
	//! The preset the role uses in this campaign, or null if it has none
	AROR_FactionPreset GetPreset(AROR_EFactionRole role)
	{
		return FindPreset(GetPresetId(role));
	}

	//------------------------------------------------------------------------------------------------
	string GetPresetId(AROR_EFactionRole role)
	{
		switch (role)
		{
			case AROR_EFactionRole.RESISTANCE:
				return m_sResistancePreset;
			case AROR_EFactionRole.OCCUPIERS:
				return m_sOccupiersPreset;
			case AROR_EFactionRole.INVADERS:
				return m_sInvadersPreset;
			case AROR_EFactionRole.CIVILIANS:
				return m_sCiviliansPreset;
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	AROR_FactionPreset FindPreset(string presetId)
	{
		if (presetId.IsEmpty())
			return null;

		foreach (AROR_FactionPreset preset : m_aPresets)
		{
			if (preset.GetId() == presetId)
				return preset;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Adds every loaded preset, in registry order (e.g. for shops that sell every faction's equipment)
	int GetAllPresets(notnull array<AROR_FactionPreset> outPresets)
	{
		foreach (AROR_FactionPreset preset : m_aPresets)
		{
			outPresets.Insert(preset);
		}

		return m_aPresets.Count();
	}

	//------------------------------------------------------------------------------------------------
	//! Adds the presets that can be chosen for the role, in registry order; returns how many were added
	int GetPresetsForRole(AROR_EFactionRole role, notnull array<AROR_FactionPreset> outPresets)
	{
		int added;
		foreach (AROR_FactionPreset preset : m_aPresets)
		{
			if (preset.IsUsableAs(role))
			{
				outPresets.Insert(preset);
				added++;
			}
		}

		return added;
	}

	//------------------------------------------------------------------------------------------------
	string GetDefaultPresetId(AROR_EFactionRole role)
	{
		switch (role)
		{
			case AROR_EFactionRole.RESISTANCE:
				return m_sDefaultResistance;
			case AROR_EFactionRole.OCCUPIERS:
				return m_sDefaultOccupiers;
			case AROR_EFactionRole.INVADERS:
				return m_sDefaultInvaders;
			case AROR_EFactionRole.CIVILIANS:
				return m_sDefaultCivilians;
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	// Server
	//------------------------------------------------------------------------------------------------
	//! Server only. Sets the campaign's presets from IDs indexed by role (AROR_EFactionRole). A missing, unknown or
	//! unusable ID falls back to the role's default. Called when a campaign is created.
	void SetPresets_S(array<string> presetIds)
	{
		if (!Replication.IsServer())
			return;

		array<AROR_EFactionRole> roles = {};
		AROR_FactionRoles.GetAll(roles);
		foreach (AROR_EFactionRole role : roles)
		{
			string requested;
			if (presetIds && role < presetIds.Count())
				requested = presetIds[role];

			SetRolePreset(role, ResolvePresetId(role, requested));
		}

		Replication.BumpMe();
		LogPresets();
		m_OnPresetsChanged.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	//! Server only: restore saved presets (used by the serializer). Same fallback rules as SetPresets_S, so a save
	//! whose preset no longer exists (e.g. its mod was removed) still loads, with that role on its default.
	void RestorePresets_S(array<string> presetIds)
	{
		SetPresets_S(presetIds);
	}

	//------------------------------------------------------------------------------------------------
	//! The requested ID if it exists and is usable for the role, otherwise the role's default
	protected string ResolvePresetId(AROR_EFactionRole role, string requested)
	{
		string roleName = typename.EnumToString(AROR_EFactionRole, role);
		if (!requested.IsEmpty())
		{
			AROR_FactionPreset preset = FindPreset(requested);
			if (preset && preset.IsUsableAs(role))
				return requested;

			Print(string.Format("[AROR] Faction preset '%1' is not available for %2; using the default", requested, roleName), LogLevel.WARNING);
		}

		string fallback = GetDefaultPresetId(role);
		AROR_FactionPreset defaultPreset = FindPreset(fallback);
		if (!defaultPreset || !defaultPreset.IsUsableAs(role))
			Print(string.Format("[AROR] Default faction preset '%1' for %2 is missing or not usable for that role", fallback, roleName), LogLevel.ERROR);

		return fallback;
	}

	//------------------------------------------------------------------------------------------------
	protected void SetRolePreset(AROR_EFactionRole role, string presetId)
	{
		switch (role)
		{
			case AROR_EFactionRole.RESISTANCE:
				m_sResistancePreset = presetId;
				break;
			case AROR_EFactionRole.OCCUPIERS:
				m_sOccupiersPreset = presetId;
				break;
			case AROR_EFactionRole.INVADERS:
				m_sInvadersPreset = presetId;
				break;
			case AROR_EFactionRole.CIVILIANS:
				m_sCiviliansPreset = presetId;
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Server: saved preset IDs indexed by role (for the serializer)
	void GetPresetIds(notnull array<string> outPresetIds)
	{
		array<AROR_EFactionRole> roles = {};
		AROR_FactionRoles.GetAll(roles);
		foreach (AROR_EFactionRole role : roles)
		{
			outPresetIds.Insert(GetPresetId(role));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void LogPresets()
	{
		Print(string.Format("[AROR] Faction presets: Resistance %1 | Occupiers %2 | Invaders %3 | Civilians %4",
			m_sResistancePreset, m_sOccupiersPreset, m_sInvadersPreset, m_sCiviliansPreset), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPresetsReplicated()
	{
		m_OnPresetsChanged.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	// Loading
	//------------------------------------------------------------------------------------------------
	//! Every machine: load the registry and each preset it lists
	protected void LoadPresets()
	{
		m_aPresets.Clear();

		AROR_FactionPresetRegistry registry = AROR_FactionPresetRegistry.Cast(LoadConfig(m_sRegistry));
		if (!registry)
		{
			Print(string.Format("[AROR] Faction preset registry '%1' could not be loaded", m_sRegistry), LogLevel.ERROR);
			return;
		}

		array<ResourceName> paths = {};
		registry.GetPresetPaths(paths);
		foreach (ResourceName path : paths)
		{
			AROR_FactionPreset preset = AROR_FactionPreset.Cast(LoadConfig(path));
			if (!preset)
			{
				Print(string.Format("[AROR] Faction preset '%1' could not be loaded", path), LogLevel.ERROR);
				continue;
			}

			if (!preset.Validate(path))
				continue;

			if (FindPreset(preset.GetId()))
			{
				Print(string.Format("[AROR] Faction preset ID '%1' is used more than once; '%2' is ignored", preset.GetId(), path), LogLevel.ERROR);
				continue;
			}

			m_aPresets.Insert(preset);
		}

		Print(string.Format("[AROR] Loaded %1 faction presets", m_aPresets.Count()), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected static Managed LoadConfig(ResourceName path)
	{
		if (path.IsEmpty())
			return null;

		Resource resource = BaseContainerTools.LoadContainer(path);
		if (!resource || !resource.IsValid())
			return null;

		return BaseContainerTools.CreateInstanceFromContainer(resource.GetResource().ToBaseContainer());
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode())
			return;

		s_Instance = this;
		LoadPresets();

		// Defaults until a campaign is created or a save restores the real choice; set directly (no BumpMe during
		// init), the initial replication snapshot carries them to clients
		if (Replication.IsServer())
		{
			array<AROR_EFactionRole> roles = {};
			AROR_FactionRoles.GetAll(roles);
			foreach (AROR_EFactionRole role : roles)
			{
				SetRolePreset(role, ResolvePresetId(role, string.Empty));
			}

			LogPresets();
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (s_Instance == this)
			s_Instance = null;

		super.OnDelete(owner);
	}
}
