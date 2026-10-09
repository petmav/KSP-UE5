using System;
using KSP.Localization;

namespace Contracts.Agents.Mentalities
{
	[Serializable]
	public class Moral : AgentMentality
	{
		protected override string GetDisplayName()
		{
			return Localizer.Format("#autoLOC_7001046");
		}

		protected override string GetDescription()
		{
			return Localizer.Format("#autoLOC_265172");
		}

		public override bool CanProcessContract(Contract contract)
		{
			return true;
		}

		public override void ProcessContract(Contract contract)
		{
			FactorFundsAdvance(contract, positive: false);
			FactorFundsCompletion(contract, positive: false);
			FactorReputationCompletion(contract, positive: true);
		}
	}
}
