using System.Globalization;
using Contracts;
using FinePrint.Utilities;
using KSP.Localization;

namespace FinePrint.Contracts.Parameters
{
	public class CrewTraitParameter : ContractParameter
	{
		private string vesselDescription;

		private string targetTrait;

		private int targetCount;

		private int successCounter;

		private bool validVessel;

		private bool dirtyVessel = true;

		private int activePartCount;

		private bool eventsAdded;

		private SpecificVesselParameter specificVesselSibling;

		private string targetTraitDisplayName
		{
			get
			{
				if (KerbalRoster.TryGetExperienceTraitConfig(targetTrait, out var traitConfig))
				{
					return traitConfig.Title;
				}
				return targetTrait;
			}
		}

		private SpecificVesselParameter SpecificVesselSibling => specificVesselSibling ?? (specificVesselSibling = base.Root.GetParameter<SpecificVesselParameter>());

		public CrewTraitParameter()
		{
			targetTrait = "Scientist";
			targetCount = 1;
		}

		public CrewTraitParameter(string targetTrait, int targetCount, string vesselDescription)
		{
			this.targetTrait = targetTrait;
			this.targetCount = targetCount;
			this.vesselDescription = vesselDescription;
		}

		protected override string GetHashString()
		{
			return SystemUtilities.SuperSeed(base.Root).ToString(CultureInfo.InvariantCulture) + base.ID;
		}

		protected override string GetTitle()
		{
			if (targetCount <= 1)
			{
				return Localizer.Format("#autoLOC_6001095", targetTraitDisplayName.ToLower(), vesselDescription);
			}
			return Localizer.Format("#autoLOC_283108", targetCount, targetTraitDisplayName.ToLower(), vesselDescription);
		}

		protected override string GetNotes()
		{
			if (!base.Root.IsFinished() && SpecificVesselSibling != null && !(SpecificVesselSibling.targetVessel == null))
			{
				int num = VesselUtilities.VesselCrewWithTraitCount(targetTrait, SpecificVesselSibling.targetVessel);
				if (num < 0)
				{
					return null;
				}
				if (num == 0)
				{
					return Localizer.Format("#autoLOC_283122", SpecificVesselSibling.targetVesselName, targetTraitDisplayName.ToLower());
				}
				return Localizer.Format("#autoLOC_283124", SpecificVesselSibling.targetVesselName, num, targetTraitDisplayName.ToLower());
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
				GameEvents.onVesselCrewWasModified.Add(CrewModified);
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
				GameEvents.onVesselCrewWasModified.Remove(CrewModified);
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

		private void CrewModified(Vessel v)
		{
			if (v != null && v == FlightGlobals.ActiveVessel)
			{
				dirtyVessel = true;
			}
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("targetTrait", targetTrait);
			node.AddValue("targetCount", targetCount);
			node.AddValue("vesselDescription", vesselDescription);
		}

		protected override void OnLoad(ConfigNode node)
		{
			SystemUtilities.LoadNode(node, "CrewTraitParameter", "targetTrait", ref targetTrait, "Scientist");
			SystemUtilities.LoadNode(node, "CrewTraitParameter", "targetCount", ref targetCount, 1);
			SystemUtilities.LoadNode(node, "CrewTraitParameter", "vesselDescription", ref vesselDescription, "vessel");
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
				validVessel = VesselUtilities.VesselCrewWithTraitCount(targetTrait, FlightGlobals.ActiveVessel) >= targetCount;
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
