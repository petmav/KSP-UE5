using System;
using System.Globalization;
using Contracts;
using FinePrint.Utilities;
using KSP.Localization;

namespace FinePrint.Contracts.Parameters
{
	public class LocationAndSituationParameter : ContractParameter
	{
		private CelestialBody targetBody;

		private Vessel.Situations targetSituation;

		private string noun;

		private bool finalObjective;

		private bool validVessel;

		private bool dirtyVessel = true;

		private int successCounter;

		private bool eventsAdded;

		public LocationAndSituationParameter()
		{
			targetSituation = Vessel.Situations.ESCAPING;
			targetBody = null;
			noun = "potato";
			finalObjective = false;
			successCounter = 0;
		}

		public LocationAndSituationParameter(CelestialBody targetBody, Vessel.Situations targetSituation, string noun, bool finalObjective = false)
		{
			this.targetBody = targetBody;
			this.targetSituation = targetSituation;
			this.noun = noun;
			this.finalObjective = finalObjective;
			successCounter = 0;
		}

		protected override void OnRegister()
		{
			base.DisableOnStateChange = false;
			if (base.Root.ContractState == Contract.State.Active)
			{
				GameEvents.onVesselSOIChanged.Add(ChangeBody);
				GameEvents.onVesselSituationChange.Add(ChangeSituation);
				eventsAdded = true;
			}
		}

		protected override void OnUnregister()
		{
			if (eventsAdded)
			{
				GameEvents.onVesselSOIChanged.Remove(ChangeBody);
				GameEvents.onVesselSituationChange.Remove(ChangeSituation);
			}
		}

		protected override void OnReset()
		{
			SetIncomplete();
			dirtyVessel = true;
		}

		private void ChangeBody(GameEvents.HostedFromToAction<Vessel, CelestialBody> action)
		{
			if (action.host == FlightGlobals.ActiveVessel)
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

		protected override string GetHashString()
		{
			return SystemUtilities.SuperSeed(base.Root).ToString(CultureInfo.InvariantCulture) + base.ID;
		}

		protected override string GetTitle()
		{
			if (targetBody == null)
			{
				return Localizer.Format("#autoLOC_283645", Planetarium.fetch.Sun.displayName);
			}
			switch (targetSituation)
			{
			case Vessel.Situations.SUB_ORBITAL:
				return Localizer.Format("#autoLOC_7000017", Convert.ToInt32(!finalObjective), noun, targetBody.displayName);
			case Vessel.Situations.FLYING:
				return Localizer.Format("#autoLOC_7000012", Convert.ToInt32(!finalObjective), noun, targetBody.displayName);
			case Vessel.Situations.LANDED:
				return Localizer.Format("#autoLOC_7000013", Convert.ToInt32(!finalObjective), noun, targetBody.displayName);
			case Vessel.Situations.SPLASHED:
				return Localizer.Format("#autoLOC_7000016", Convert.ToInt32(!finalObjective), noun, targetBody.displayName);
			case Vessel.Situations.PRELAUNCH:
				return Localizer.Format("#autoLOC_7000015", Convert.ToInt32(!finalObjective), noun, targetBody.displayName);
			case Vessel.Situations.DOCKED:
				return Localizer.Format("#autoLOC_7000010", Convert.ToInt32(!finalObjective), noun, targetBody.displayName);
			default:
				return Localizer.Format("#autoLOC_7000018", Convert.ToInt32(!finalObjective), noun, targetBody.displayName);
			case Vessel.Situations.ESCAPING:
				return Localizer.Format("#autoLOC_7000011", Convert.ToInt32(!finalObjective), noun, targetBody.displayName);
			case Vessel.Situations.ORBITING:
				return Localizer.Format("#autoLOC_7000014", Convert.ToInt32(!finalObjective), noun, targetBody.displayName);
			}
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("targetBody", targetBody.flightGlobalsIndex);
			node.AddValue("targetSituation", targetSituation);
			node.AddValue("noun", noun);
			node.AddValue("finalObjective", finalObjective);
		}

		protected override void OnLoad(ConfigNode node)
		{
			SystemUtilities.LoadNode(node, "LocationAndSituationParameter", "targetBody", ref targetBody, Planetarium.fetch.Home);
			SystemUtilities.LoadNode(node, "LocationAndSituationParameter", "targetSituation", ref targetSituation, Vessel.Situations.ORBITING);
			SystemUtilities.LoadNode(node, "LocationAndSituationParameter", "noun", ref noun, "potato");
			SystemUtilities.LoadNode(node, "LocationAndSituationParameter", "finalObjective", ref finalObjective, defaultValue: false);
		}

		protected override void OnUpdate()
		{
			if (!SystemUtilities.FlightIsReady(base.Root.ContractState, Contract.State.Active, checkVessel: true))
			{
				return;
			}
			if (dirtyVessel)
			{
				dirtyVessel = false;
				validVessel = FlightGlobals.ActiveVessel.situation == targetSituation && FlightGlobals.ActiveVessel.mainBody == targetBody && FlightGlobals.ActiveVessel.vesselType > VesselType.Unknown && FlightGlobals.ActiveVessel.vesselType != VesselType.EVA && FlightGlobals.ActiveVessel.vesselType != VesselType.Flag && FlightGlobals.ActiveVessel.vesselType != VesselType.DeployedSciencePart && FlightGlobals.ActiveVessel.vesselType != VesselType.DeployedScienceController;
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
