//! Optional server diagnostic: one same-faction damage report per watched entity.
//! Records damage, not an inference that the attacker deliberately targeted a friendly.
class AROR_OperationDamageObserver
{
	protected IEntity m_Entity;
	protected SCR_DamageManagerComponent m_DamageManager;
	protected string m_sOperationId;
	protected int m_iPackageIndex;
	protected bool m_bReported;

	IEntity GetEntity()
	{
		return m_Entity;
	}

	bool Attach_S(IEntity entity, string operationId, int packageIndex)
	{
		if (!Replication.IsServer() || !entity)
			return false;
		m_DamageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		if (!m_DamageManager)
			return false;
		m_Entity = entity;
		m_sOperationId = operationId;
		m_iPackageIndex = packageIndex;
		m_DamageManager.GetOnDamage().Insert(OnDamage);
		return true;
	}

	protected void OnDamage(BaseDamageContext context)
	{
		if (!Replication.IsServer() || m_bReported || !m_Entity || !context || context.damageValue <= 0 || !context.instigator)
			return;
		if (context.damageType == EDamageType.HEALING || context.damageType == EDamageType.REGENERATION || context.instigator.GetInstigatorType() == InstigatorType.INSTIGATOR_GM)
			return;
		IEntity attacker = context.instigator.GetInstigatorEntity();
		if (!attacker || attacker == m_Entity)
			return;
		FactionAffiliationComponent victimFaction = FactionAffiliationComponent.Cast(m_Entity.FindComponent(FactionAffiliationComponent));
		FactionAffiliationComponent attackerFaction = FactionAffiliationComponent.Cast(attacker.FindComponent(FactionAffiliationComponent));
		if (!victimFaction || !attackerFaction || victimFaction.GetAffiliatedFactionKey().IsEmpty() || victimFaction.GetAffiliatedFactionKey() != attackerFaction.GetAffiliatedFactionKey())
			return;
		m_bReported = true;
		Print(string.Format("[AROR] Same-faction damage: operation=%1 package=%2 faction=%3 type=%4 victim=%5 attacker=%6 playerId=%7", m_sOperationId, m_iPackageIndex + 1, victimFaction.GetAffiliatedFactionKey(), context.damageType, m_Entity, attacker, context.instigator.GetInstigatorPlayerID()), LogLevel.WARNING);
	}

	void Detach_S()
	{
		if (m_DamageManager)
			m_DamageManager.GetOnDamage().Remove(OnDamage);
		m_DamageManager = null;
		m_Entity = null;
	}
}
