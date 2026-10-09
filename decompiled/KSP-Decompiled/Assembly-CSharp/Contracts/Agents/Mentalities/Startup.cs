using System;
using KSP.Localization;

namespace Contracts.Agents.Mentalities
{
	[Serializable]
	public class Startup : AgentMentality
	{
		protected override string GetDisplayName()
		{
			return Localizer.Format("#autoLOC_7001053");
		}

		protected override string GetDescription()
		{
			return Localizer.Format("#autoLOC_265398");
		}

		public override KeywordScore ScoreKeyword(string keyword)
		{
			if (keyword == "Commercial")
			{
				return KeywordScore.Positive;
			}
			return KeywordScore.None;
		}

		public override bool CanProcessContract(Contract contract)
		{
			return true;
		}

		public override void ProcessContract(Contract contract)
		{
			FactorFundsAdvance(contract, positive: false);
			FactorFundsCompletion(contract, positive: false);
			FactorExpiry(contract, positive: false);
			FactorDeadline(contract, positive: false);
		}
	}
}
