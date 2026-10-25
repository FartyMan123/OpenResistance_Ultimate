[WorkbenchPluginAttribute(name: "Audit all campaign flags", description: "Read-only terrain and model-bounds report for the 23 military Chicken flags.", wbModules: {"WorldEditor"}, category: "Open Resistance")]
class AROR_AuditCampaignFlagsPlugin : AROR_GroundCampaignFlagsPlugin
{
	override void Run()
	{
		RunAudit(false);
	}
}
