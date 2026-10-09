using System;
using KSP.Localization;

namespace Contracts.Parameters
{
	[Serializable]
	public class KerbalDeaths : ContractParameter
	{
		protected int countMax;

		protected int countCurrent;

		public int CountMax => countMax;

		public int CountCurrent => countCurrent;

		public KerbalDeaths()
		{
		}

		public KerbalDeaths(int countMax)
		{
			state = ParameterState.Complete;
			this.countMax = countMax;
			countCurrent = 0;
		}

		protected override string GetHashString()
		{
			return null;
		}

		protected override string GetTitle()
		{
			if (countMax > 1)
			{
				return Localizer.Format("#autoLOC_269684", countMax.ToString());
			}
			return Localizer.Format("#autoLOC_269686");
		}

		protected override void OnLoad(ConfigNode node)
		{
			if (node.HasValue("count"))
			{
				countCurrent = int.Parse(node.GetValue("count"));
			}
			if (node.HasValue("countMax"))
			{
				countMax = int.Parse(node.GetValue("countMax"));
			}
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("count", countCurrent);
			node.AddValue("countMax", countMax);
		}

		protected override void OnRegister()
		{
			GameEvents.onCrewKilled.Add(OnCrewKilled);
		}

		protected override void OnUnregister()
		{
			GameEvents.onCrewKilled.Remove(OnCrewKilled);
		}

		private void OnCrewKilled(EventReport report)
		{
			if (report.eventType == FlightEvents.CREW_KILLED)
			{
				countCurrent++;
				if (countCurrent == countMax)
				{
					SetFailed();
				}
			}
		}
	}
}
