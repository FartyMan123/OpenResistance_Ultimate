[WorkbenchPluginAttribute(name: "Audit Gillnet Pond outpost clearing", description: "Read-only dense 25m-grid search within 800m of Andres Beacon Outpost. Checks its complete standard footprint and eight orientations without relaxing terrain limits.", wbModules: {"WorldEditor"}, category: "Open Resistance")]
class AROR_AuditAndresDefenceSitePlugin : AROR_AuditDefenceSitesPlugin
{
	override void Run()
	{
		RunSiteAudit("everon_outpost_andres_beacon");
	}
}
