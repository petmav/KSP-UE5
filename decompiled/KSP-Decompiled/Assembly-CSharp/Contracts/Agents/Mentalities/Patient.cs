using System;
using KSP.Localization;

namespace Contracts.Agents.Mentalities
{
	[Serializable]
	public class Patient : AgentMentality
	{
		protected override string GetDisplayName()
		{
			return Localizer.Format("#autoLOC_7001048");
		}

		protected override string GetDescription()
		{
			return Localizer.Format("#autoLOC_265230");
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
			FactorDeadline(contract, positive: true);
			FactorExpiry(contract, positive: true);
			FactorReputationCompletion(contract, positive: false);
			FactorReputationFailure(contract, positive: false);
		}
	}
}
