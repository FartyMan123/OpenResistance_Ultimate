class AROR_GroundOperationForce : AROR_OperationForce
{
	ref AROR_ForceProfile m_Profile;
	SCR_AIGroup m_CrewGroup;
	IEntity m_Vehicle;
	AIWaypoint m_CrewWaypoint;
	bool m_bMounted;
	bool m_bDismountRequested;
	bool m_bDismounted;
	bool m_bSawInfantry;
	bool m_bSawCrew;
	bool m_bTownSearchAttempted;
	bool m_bTownSearchFinished;
	ref AROR_VehicleTownSearch m_TownSearch;
	vector m_vApproachPosition;
	protected ref array<IEntity> m_aPassengers = {};

	int CountInfantry()
	{
		int living = super.CountLivingMembers();
		if (living > 0)
			m_bSawInfantry = true;
		return living;
	}

	int CountCrew()
	{
		if (!m_CrewGroup)
			return 0;
		array<AIAgent> agents = {};
		m_CrewGroup.GetAgents(agents);
		int living;
		foreach (AIAgent agent : agents)
		{
			OnAgentAdded(agent);
			if (!agent)
				continue;
			ChimeraCharacter character = ChimeraCharacter.Cast(agent.GetControlledEntity());
			if (character && character.GetCharacterController() && !character.GetCharacterController().IsDead())
				living++;
		}
		if (living > 0)
			m_bSawCrew = true;
		return living;
	}

	override int CountLivingMembers()
	{
		int living = super.CountLivingMembers();
		if (m_CrewGroup != m_Group)
			living += CountCrew();
		return living;
	}

	bool Seat(IEntity entity, BaseCompartmentSlot slot)
	{
		if (!entity || !slot || slot.IsOccupied())
			return false;
		CompartmentAccessComponent access = CompartmentAccessComponent.Cast(entity.FindComponent(CompartmentAccessComponent));
		return access && access.GetInVehicle(m_Vehicle, slot, true, -1, ECloseDoorAfterActions.CLOSE_DOOR, false);
	}

	//! Spawn into known seats only after every requested member exists; no crew/infantry index mixing.
	bool Mount_S()
	{
		if (!Replication.IsServer() || !m_Vehicle || !m_CrewGroup)
			return false;
		BaseCompartmentManagerComponent compartments = BaseCompartmentManagerComponent.Cast(m_Vehicle.FindComponent(BaseCompartmentManagerComponent));
		SCR_AIVehicleUsageComponent usage = SCR_AIVehicleUsageComponent.Cast(m_Vehicle.FindComponent(SCR_AIVehicleUsageComponent));
		if (!compartments || !usage || !usage.CanBePiloted() || !usage.GetPilotCompartmentSlot())
			return false;
		array<BaseCompartmentSlot> slots = {};
		compartments.GetCompartments(slots);
		array<BaseCompartmentSlot> crewSeats = {};
		array<BaseCompartmentSlot> cargoSeats = {};
		crewSeats.Insert(usage.GetPilotCompartmentSlot());
		foreach (BaseCompartmentSlot slot : slots)
		{
			if (!slot || !slot.IsCompartmentAccessible() || slot.IsOccupied() || crewSeats.Contains(slot))
				continue;
			if (TurretCompartmentSlot.Cast(slot))
				crewSeats.Insert(slot);
			else if (CargoCompartmentSlot.Cast(slot))
				cargoSeats.Insert(slot);
		}
		array<AIAgent> crew = {};
		m_CrewGroup.GetAgents(crew);
		if (crew.Count() < crewSeats.Count())
			return false;
		// Extra crew may occupy spare cargo seats, but never displace passenger infantry.
		array<AIAgent> infantry = {};
		if (m_Group != m_CrewGroup)
			m_Group.GetAgents(infantry);
		int extraCrew = crew.Count() - crewSeats.Count();
		if (cargoSeats.Count() < infantry.Count() + extraCrew)
			return false;
		for (int i = 0; i < crew.Count(); i++)
		{
			BaseCompartmentSlot crewSeat;
			if (i < crewSeats.Count())
				crewSeat = crewSeats[i];
			else
				crewSeat = cargoSeats[i - crewSeats.Count()];
			if (!crew[i] || !Seat(crew[i].GetControlledEntity(), crewSeat))
				return false;
		}
		for (int p = 0; p < infantry.Count(); p++)
		{
			IEntity passenger;
			if (infantry[p])
				passenger = infantry[p].GetControlledEntity();
			if (!Seat(passenger, cargoSeats[extraCrew + p]))
				return false;
			m_aPassengers.Insert(passenger);
		}
		SCR_AIGroupUtilityComponent utility = m_CrewGroup.GetGroupUtilityComponent();
		if (!utility)
			return false;
		utility.AddUsableVehicle(usage);
		m_bMounted = true;
		return true;
	}

	//! Passengers exit with animations. Caller waits for confirmation before issuing ground orders.
	bool RequestDismount_S()
	{
		if (!Replication.IsServer())
			return false;
		foreach (IEntity passenger : m_aPassengers)
		{
			if (!passenger)
				continue;
			ChimeraCharacter character = ChimeraCharacter.Cast(passenger);
			if (character && character.GetCharacterController() && character.GetCharacterController().IsDead())
				continue;
			CompartmentAccessComponent access = CompartmentAccessComponent.Cast(passenger.FindComponent(CompartmentAccessComponent));
			if (access && access.IsInCompartment() && !access.IsGettingOut())
			{
				BaseCompartmentSlot slot = access.GetCompartment();
				array<int> doors = {};
				if (slot)
					slot.GetAvailableDoorIndices(doors);
				int doorIndex = -1;
				foreach (int candidate : doors)
				{
					if (access.CanGetOutVehicleViaDoor(candidate))
					{
						doorIndex = candidate;
						break;
					}
				}
				if (doorIndex < 0 || !access.GetOutVehicle(EGetOutType.ANIMATED, doorIndex, ECloseDoorAfterActions.CLOSE_DOOR, false))
					return false;
			}
		}
		m_bDismountRequested = true;
		return true;
	}

	bool ArePassengersOut()
	{
		foreach (IEntity passenger : m_aPassengers)
		{
			if (!passenger)
				continue;
			ChimeraCharacter character = ChimeraCharacter.Cast(passenger);
			if (character && character.GetCharacterController() && character.GetCharacterController().IsDead())
				continue;
			CompartmentAccessComponent access = CompartmentAccessComponent.Cast(passenger.FindComponent(CompartmentAccessComponent));
			if (access && (access.IsInCompartment() || access.IsGettingOut()))
				return false;
		}
		return true;
	}

	override void Cleanup_S()
	{
		if (!Replication.IsServer())
			return;
		if (m_TownSearch)
			m_TownSearch.Cleanup_S();
		m_TownSearch = null;
		CountLivingMembers();
		bool preserveVehicle;
		if (m_Vehicle)
		{
			BaseCompartmentManagerComponent compartments = BaseCompartmentManagerComponent.Cast(m_Vehicle.FindComponent(BaseCompartmentManagerComponent));
			array<BaseCompartmentSlot> slots = {};
			if (compartments)
				compartments.GetCompartments(slots);
			foreach (BaseCompartmentSlot slot : slots)
			{
				if (slot && slot.GetOccupant() && GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(slot.GetOccupant()) > 0)
					preserveVehicle = true;
			}
		}
		if (m_CrewGroup && m_CrewGroup != m_Group)
		{
			m_CrewGroup.GetOnAgentAdded().Remove(OnAgentAdded);
			if (m_CrewWaypoint)
				m_CrewGroup.RemoveWaypoint(m_CrewWaypoint);
			if (m_CrewGroup.GetPlayerCount() == 0)
				SCR_EntityHelper.DeleteEntityAndChildren(m_CrewGroup);
		}
		if (m_CrewWaypoint)
			SCR_EntityHelper.DeleteEntityAndChildren(m_CrewWaypoint);
		super.Cleanup_S();
		if (m_Vehicle && !preserveVehicle)
			SCR_EntityHelper.DeleteEntityAndChildren(m_Vehicle);
		m_Vehicle = null;
		m_CrewGroup = null;
		m_CrewWaypoint = null;
		m_aPassengers.Clear();
	}
}
