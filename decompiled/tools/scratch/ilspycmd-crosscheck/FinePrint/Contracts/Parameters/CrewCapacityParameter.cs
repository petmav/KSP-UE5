using System.Globalization;
using Contracts;
using FinePrint.Utilities;
using KSP.Localization;

namespace FinePrint.Contracts.Parameters
{
	public class CrewCapacityParameter : ContractParameter
	{
		private int targetCapacity;

		private int successCounter;

		private bool validVessel;

		private bool dirtyVessel = true;

		private int activePartCount;

		private bool eventsAdded;

		private SpecificVesselParameter specificVesselSibling;

		private SpecificVesselParameter SpecificVesselSibling => specificVesselSibling ?? (specificVesselSibling = base.Root.GetParameter<SpecificVesselParameter>());

		public CrewCapacityParameter()
		{
			targetCapacity = 8;
			successCounter = 0;
		}

		public CrewCapacityParameter(int targetCapacity)
		{
			this.targetCapacity = targetCapacity;
			successCounter = 0;
		}

		protected override string GetHashString()
		{
			return SystemUtilities.SuperSeed(base.Root).ToString(CultureInfo.InvariantCulture) + base.ID;
		}

		protected override string GetTitle()
		{
			return Localizer.Format("#autoLOC_282944", targetCapacity);
		}

		protected override string GetNotes()
		{
			if (!base.Root.IsFinished() && SpecificVesselSibling != null && !(SpecificVesselSibling.targetVessel == null))
			{
				int num = VesselUtilities.ActualCrewCapacity(SpecificVesselSibling.targetVessel);
				if (num < 0)
				{
					return null;
				}
				if (num == 0)
				{
					return Localizer.Format("#autoLOC_282958", SpecificVesselSibling.targetVesselName);
				}
				return Localizer.Format("#autoLOC_282960", SpecificVesselSibling.targetVesselName, num);
			}
			return null;
		}

		protected override void OnRegister()
		{
			base.DisableOnStateChange = false;
			eventsAdded = false;
			if (base.Root.ContractState == Contract.State.Active)
			{
				GameEvents.onPartCouple.Add(OnDock);
				GameEvents.onVesselWasModified.Add(VesselModified);
				GameEvents.onPartDie.Add(PartModified);
				eventsAdded = true;
			}
		}

		protected override void OnUnregister()
		{
			if (eventsAdded)
			{
				GameEvents.onPartCouple.Remove(OnDock);
				GameEvents.onVesselWasModified.Remove(VesselModified);
				GameEvents.onPartDie.Remove(PartModified);
			}
		}

		protected override void OnReset()
		{
			SetIncomplete();
			dirtyVessel = true;
		}

		private void VesselModified(Vessel v)
		{
			if (v == FlightGlobals.ActiveVessel)
			{
				dirtyVessel = true;
			}
		}

		private void PartModified(Part p)
		{
			if (p.vessel == FlightGlobals.ActiveVessel)
			{
				dirtyVessel = true;
			}
		}

		private void OnDock(GameEvents.FromToAction<Part, Part> action)
		{
			if (action.from.vessel == FlightGlobals.ActiveVessel || action.to.vessel == FlightGlobals.ActiveVessel)
			{
				dirtyVessel = true;
			}
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("targetCapacity", targetCapacity);
		}

		protected override void OnLoad(ConfigNode node)
		{
			SystemUtilities.LoadNode(node, "CrewCapacityParameter", "targetCapacity", ref targetCapacity, 8);
		}

		protected override void OnUpdate()
		{
			if (!SystemUtilities.FlightIsReady(base.Root.ContractState, Contract.State.Active, checkVessel: true))
			{
				return;
			}
			int num = (FlightGlobals.ActiveVessel.loaded ? FlightGlobals.ActiveVessel.Parts.Count : FlightGlobals.ActiveVessel.protoVessel.protoPartSnapshots.Count);
			if (activePartCount != num)
			{
				dirtyVessel = true;
			}
			if (dirtyVessel)
			{
				dirtyVessel = false;
				activePartCount = (FlightGlobals.ActiveVessel.loaded ? FlightGlobals.ActiveVessel.Parts.Count : FlightGlobals.ActiveVessel.protoVessel.protoPartSnapshots.Count);
				validVessel = FlightGlobals.ActiveVessel.GetCrewCapacity() >= targetCapacity;
			}
			if (base.State == ParameterState.Incomplete)
			{
				if (validVessel)
				{
					successCounter++;
				}
				else
				{
					successCounter = 0;
				}
				if (successCounter >= 5)
				{
					SetComplete();
				}
			}
			if (base.State == ParameterState.Complete && !validVessel)
			{
				SetIncomplete();
			}
		}
	}
}
