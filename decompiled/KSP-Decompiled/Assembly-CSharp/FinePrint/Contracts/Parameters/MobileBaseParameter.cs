using System.Globalization;
using Contracts;
using FinePrint.Utilities;
using KSP.Localization;

namespace FinePrint.Contracts.Parameters
{
	public class MobileBaseParameter : ContractParameter
	{
		private int successCounter;

		private bool validVessel;

		private bool dirtyVessel = true;

		private int activePartCount;

		private bool eventsAdded;

		public MobileBaseParameter()
		{
			successCounter = 0;
		}

		protected override string GetHashString()
		{
			return SystemUtilities.SuperSeed(base.Root).ToString(CultureInfo.InvariantCulture) + base.ID;
		}

		protected override string GetTitle()
		{
			return Localizer.Format("#autoLOC_7001018");
		}

		protected override void OnRegister()
		{
			base.DisableOnStateChange = false;
			if (base.Root.ContractState == Contract.State.Active)
			{
				GameEvents.onPartCouple.Add(OnDock);
				GameEvents.onVesselWasModified.Add(VesselModified);
				GameEvents.onPartDie.Add(PartModified);
				GameEvents.onVesselSituationChange.Add(ChangeSituation);
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
				GameEvents.onVesselSituationChange.Remove(ChangeSituation);
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

		private void ChangeSituation(GameEvents.HostedFromToAction<Vessel, Vessel.Situations> action)
		{
			if (action.host == FlightGlobals.ActiveVessel)
			{
				dirtyVessel = true;
			}
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
				validVessel = VesselUtilities.VesselHasWheelsOnGround(FlightGlobals.ActiveVessel, WheelType.MOTORIZED);
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
