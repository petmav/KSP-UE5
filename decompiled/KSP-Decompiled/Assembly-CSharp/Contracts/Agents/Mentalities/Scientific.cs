using System;
using KSP.Localization;

namespace Contracts.Agents.Mentalities
{
	[Serializable]
	public class Scientific : AgentMentality
	{
		protected override string GetDisplayName()
		{
			return Localizer.Format("#autoLOC_7001052");
		}

		protected override string GetDescription()
		{
			return Localizer.Format("#autoLOC_265367");
		}

		public override KeywordScore ScoreKeyword(string keyword)
		{
			if (keyword == "Scientific")
			{
				return KeywordScore.Positive;
			}
			return KeywordScore.None;
		}

		public override bool CanProcessContract(Contract contract)
		{
			return !contract.Keywords.Contains("Record");
		}

		public override void ProcessContract(Contract contract)
		{
		}
	}
}
