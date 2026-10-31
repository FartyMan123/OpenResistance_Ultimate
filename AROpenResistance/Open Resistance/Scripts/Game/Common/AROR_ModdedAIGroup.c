//! Every AI group: soldiers of the enemy roles are armed from their role's faction preset as they join
//! (see AROR_SoldierArmory). Modded rather than subclassed because garrisons, QRFs, patrols and missions all use
//! base-game group prefabs.
modded class SCR_AIGroup
{
	//------------------------------------------------------------------------------------------------
	override void OnAgentAdded(AIAgent child)
	{
		super.OnAgentAdded(child);
		AROR_SoldierArmory.OnAgentAdded_S(child);
	}
}
