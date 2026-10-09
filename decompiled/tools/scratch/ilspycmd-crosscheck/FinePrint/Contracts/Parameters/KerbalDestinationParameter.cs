using System.Collections.Generic;
using System.Globalization;
using Contracts;
using FinePrint.Utilities;
using KSP.Localization;

namespace FinePrint.Contracts.Parameters
{
	public class KerbalDestinationParameter : ContractParameter
	{
		public CelestialBody targetBody;

		public FlightLog.EntryType targetType;

		public string kerbalName;

		private bool eventsAdded;

		public KerbalDestinationParameter()
		{
			targetBody = Planetarium.fetch.Home;
			targetType = FlightLog.EntryType.Suborbit;
			kerbalName = "Jebediah Kerman";
		}

		public KerbalDestinationParameter(CelestialBody targetBody, FlightLog.EntryType targetType, string kerbalName)
		{
			this.targetBody = targetBody;
			this.targetType = targetType;
			this.kerbalName = kerbalName;
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
				return Localizer.Format("#autoLOC_283280");
			case FlightLog.EntryType.Land:
				return Localizer.Format("#autoLOC_283270", targetBody.displayName);
			case FlightLog.EntryType.Flight:
				return Localizer.Format("#autoLOC_283274", targetBody.displayName);
			case FlightLog.EntryType.Flyby:
				return Localizer.Format("#autoLOC_283276", targetBody.displayName);
			case FlightLog.EntryType.Orbit:
				return Localizer.Format("#autoLOC_283278", targetBody.displayName);
			case FlightLog.EntryType.Suborbit:
				return Localizer.Format("#autoLOC_283272", targetBody.displayName);
			}
		}

		protected override void OnRegister()
		{
			base.DisableOnStateChange = true;
			if (base.Root.ContractState == Contract.State.Active && !CheckFlightLogs())
			{
				GameEvents.OnFlightLogRecorded.Add(CheckFlightLog);
				eventsAdded = true;
			}
		}

		protected override void OnUnregister()
		{
			if (eventsAdded)
			{
				GameEvents.OnFlightLogRecorded.Remove(CheckFlightLog);
			}
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("targetBody", targetBody.flightGlobalsIndex);
			node.AddValue("targetType", targetType);
			node.AddValue("kerbalName", kerbalName);
		}

		protected override void OnLoad(ConfigNode node)
		{
			SystemUtilities.LoadNode(node, "KerbalDestinationParameter", "targetBody", ref targetBody, Planetarium.fetch.Home);
			SystemUtilities.LoadNode(node, "KerbalDestinationParameter", "targetType", ref targetType, FlightLog.EntryType.Suborbit);
			SystemUtilities.LoadNode(node, "KerbalDestinationParameter", "kerbalName", ref kerbalName, "Jebediah Kerman");
		}

		private void CheckFlightLog(Vessel v)
		{
			List<ProtoCrewMember> vesselCrew = v.GetVesselCrew();
			int count = vesselCrew.Count;
			while (count-- > 0)
			{
				ProtoCrewMember protoCrewMember = vesselCrew[count];
				if (protoCrewMember.name != kerbalName)
				{
					continue;
				}
				List<FlightLog.Entry> list = new List<FlightLog.Entry>(protoCrewMember.flightLog.Entries);
				list.AddRange(protoCrewMember.careerLog.Entries);
				for (int num = list.Count - 1; num >= 0; num--)
				{
					FlightLog.Entry entry = list[num];
					if (entry.type == targetType.ToString() && entry.target == targetBody.GetName())
					{
						SetComplete();
					}
				}
			}
		}

		private bool CheckFlightLogs()
		{
			ProtoCrewMember protoCrewMember = null;
			if (HighLogic.CurrentGame != null)
			{
				if (HighLogic.CurrentGame.CrewRoster.Exists(kerbalName))
				{
					protoCrewMember = HighLogic.CurrentGame.CrewRoster[kerbalName];
				}
				if (protoCrewMember != null)
				{
					List<FlightLog.Entry> list = new List<FlightLog.Entry>(protoCrewMember.flightLog.Entries);
					list.AddRange(protoCrewMember.careerLog.Entries);
					int num = list.Count - 1;
					while (num >= 0)
					{
						FlightLog.Entry entry = list[num];
						if (!(entry.type == targetType.ToString()) || !(entry.target == targetBody.GetName()))
						{
							num--;
							continue;
						}
						SetComplete();
						return true;
					}
				}
			}
			return false;
		}
	}
}
