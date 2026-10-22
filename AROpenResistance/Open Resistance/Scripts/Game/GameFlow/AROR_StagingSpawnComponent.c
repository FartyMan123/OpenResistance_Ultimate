[ComponentEditorProps(category: "OpenResistance/GameFlow", description: "Marks a spawn point as the pre-game staging spawn. Place AROR_StagingSpawn.et once per map.")]
class AROR_StagingSpawnComponentClass : ScriptComponentClass
{
}

//! Staging spawn (Antistasi-style): players spawn here while the admin sets up the campaign.
//! AROR_GameFlowComponent sets its faction to the Resistance role and disables it once a playthrough exists.
class AROR_StagingSpawnComponent : ScriptComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;

		SCR_SpawnPoint spawnPoint = SCR_SpawnPoint.Cast(owner);
		if (!spawnPoint)
		{
			Print("[AROR] AROR_StagingSpawnComponent must be on an SCR_SpawnPoint", LogLevel.ERROR);
			return;
		}

		AROR_GameFlowComponent.RegisterStagingSpawn(spawnPoint);
	}
}
