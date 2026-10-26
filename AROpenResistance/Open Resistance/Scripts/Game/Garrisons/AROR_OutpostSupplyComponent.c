[ComponentEditorProps(category: "OpenResistance/Garrisons", description: "Identifies a native, finite outpost equipment container and reports its physical inventory once after startup. Never refills or replaces looted items.")]
class AROR_OutpostSupplyComponentClass : ScriptComponentClass {}

//! Inventory and contents belong to the inherited base-game equipment-box prefab.
//! This marker audits the real container, not an arsenal catalogue or a supply balance.
class AROR_OutpostSupplyComponent : ScriptComponent
{
	[Attribute("Equipment", desc: "Supply category shown in server validation logs")]
	protected string m_sCategory;

	protected void CheckContents_S()
	{
		if (!Replication.IsServer())
			return;
		IEntity owner = GetOwner();
		if (!owner)
			return;
		string locationId = "unregistered";
		IEntity parent = owner.GetParent();
		for (int depth = 0; parent && depth < 16; depth++)
		{
			AROR_LocationComponent location = AROR_LocationComponent.Cast(parent.FindComponent(AROR_LocationComponent));
			if (location)
			{
				locationId = location.GetId();
				break;
			}
			parent = parent.GetParent();
		}
		InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(owner.FindComponent(InventoryStorageManagerComponent));
		if (!inventory)
		{
			Print(string.Format("[AROR OUTPOST STORE] site=%1 category=%2 missing native inventory manager; container access/content requires correction", locationId, m_sCategory), LogLevel.ERROR);
			return;
		}
		array<IEntity> items = {};
		inventory.GetItems(items);
		if (items.IsEmpty())
			Print(string.Format("[AROR OUTPOST STORE] site=%1 category=%2 observedItems=0; inspect native contents or prior looting/save restoration; no refill performed", locationId, m_sCategory), LogLevel.WARNING);
		else
			Print(string.Format("[AROR OUTPOST STORE] site=%1 category=%2 observedItems=%3; native finite inventory, no automatic restock", locationId, m_sCategory, items.Count()), LogLevel.NORMAL);
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;
		GetGame().GetCallqueue().CallLater(CheckContents_S, 5000, false);
	}

	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(CheckContents_S);
		super.OnDelete(owner);
	}
}
