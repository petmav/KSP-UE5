using System.Collections.Generic;
using System.Globalization;
using Contracts;
using FinePrint.Utilities;
using KSP.Localization;

namespace FinePrint.Contracts.Parameters
{
	public class VesselDestinationParameter : ContractParameter
	{
		public CelestialBody targetBody;

		public FlightLog.EntryType targetType;

		private bool eventsAdded;

		public VesselDestinationParameter()
		{
			targetBody = Planetarium.fetch.Home;
			targetType = FlightLog.EntryType.Suborbit;
		}

		public VesselDestinationParameter(CelestialBody targetBody, FlightLog.EntryType targetType)
		{
			this.targetBody = targetBody;
			this.targetType = targetType;
		}

		protected override string GetHashString()
		{
			return SystemUtilities.SuperSeed(base.Root).ToString(CultureInfo.InvariantCulture) + base.ID;
		}

		protected override string GetTitle()
		{
			switch (targetType)
			{
			default:
				return Localizer.Format("#autoLOC_286272");
			case FlightLog.EntryType.Land:
				return Localizer.Format("#autoLOC_286262", targetBody.displayName);
			case FlightLog.EntryType.Flight:
				return Localizer.Format("#autoLOC_286266", targetBody.displayName);
			case FlightLog.EntryType.Flyby:
				return Localizer.Format("#autoLOC_286268", targetBody.displayName);
			case FlightLog.EntryType.Orbit:
				return Localizer.Format("#autoLOC_286270", targetBody.displayName);
			case FlightLog.EntryType.Suborbit:
				return Localizer.Format("#autoLOC_286264", targetBody.displayName);
			}
		}

		protected override void OnRegister()
		{
			base.DisableOnStateChange = false;
			if (base.Root.ContractState == Contract.State.Active)
			{
				GameEvents.OnFlightLogRecorded.Add(CheckFlightLog);
				GameEvents.onVesselWasModified.Add(CheckFlightLog);
				eventsAdded = true;
			}
		}

		protected override void OnUnregister()
		{
			if (eventsAdded)
			{
				GameEvents.OnFlightLogRecorded.Remove(CheckFlightLog);
				GameEvents.onVesselWasModified.Remove(CheckFlightLog);
			}
		}

		protected override void OnReset()
		{
			if (SystemUtilities.FlightIsReady(checkVessel: true))
			{
				CheckFlightLog(FlightGlobals.ActiveVessel);
			}
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("targetBody", targetBody.flightGlobalsIndex);
			node.AddValue("targetType", targetType);
		}

		protected override void OnLoad(ConfigNode node)
		{
			SystemUtilities.LoadNode(node, "VesselDestinationParameter", "targetBody", ref targetBody, Planetarium.fetch.Home);
			SystemUtilities.LoadNode(node, "VesselDestinationParameter", "targetType", ref targetType, FlightLog.EntryType.Suborbit);
		}

		private void CheckFlightLog(Vessel v)
		{
			if (v.vesselType <= VesselType.SpaceObject)
			{
				return;
			}
			List<FlightLog.Entry> entries = VesselTripLog.FromVessel(v).Log.Entries;
			int count = entries.Count;
			FlightLog.Entry entry;
			do
			{
				if (count-- > 0)
				{
					entry = entries[count];
					continue;
				}
				SetIncomplete();
				return;
			}
			while (entry.type != targetType.ToString() || entry.target != targetBody.GetName());
			SetComplete();
		}
	}
}
