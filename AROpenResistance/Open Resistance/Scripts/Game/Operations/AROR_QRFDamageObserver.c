//! One server-side subscription. World entities and the defense component are weak references.
class AROR_QRFDamageObserver
{
	protected IEntity m_Entity;
	protected SCR_DamageManagerComponent m_DamageManager;
	protected AROR_QRFDefenseComponent m_Defense;

	bool Attach_S(IEntity entity, AROR_QRFDefenseComponent defense)
	{
		if (!Replication.IsServer() || !entity || !defense)
			return false;
		m_DamageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		if (!m_DamageManager)
			return false;
		m_Entity = entity;
		m_Defense = defense;
		m_DamageManager.GetOnDamage().Insert(OnDamage);
		return true;
	}

	IEntity GetEntity()
	{
		return m_Entity;
	}

	protected void OnDamage(BaseDamageContext context)
	{
		if (m_Defense && m_Entity)
			m_Defense.ReportDamage_S(m_Entity, context);
	}

	void Detach_S()
	{
		if (m_DamageManager)
			m_DamageManager.GetOnDamage().Remove(OnDamage);
		m_DamageManager = null;
		m_Entity = null;
		m_Defense = null;
	}
}
