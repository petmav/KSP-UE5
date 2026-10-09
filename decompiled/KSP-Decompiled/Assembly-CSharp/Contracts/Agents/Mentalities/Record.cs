using System;
using KSP.Localization;

namespace Contracts.Agents.Mentalities
{
	[Serializable]
	public class Record : AgentMentality
	{
		protected override string GetDisplayName()
		{
			return Localizer.Format("#autoLOC_7001051");
		}

		protected override string GetDescription()
		{
			return Localizer.Format("#autoLOC_265328");
		}

		public override bool CanProcessContract(Contract contract)
		{
			int count = contract.KeywordsRequired.Count;
			do
			{
				if (count-- <= 0)
				{
					return false;
				}
			}
			while (!(contract.KeywordsRequired[count] == "Record"));
			return true;
		}

		public override KeywordScore ScoreKeyword(string keyword)
		{
			if (keyword == "Record")
			{
				return KeywordScore.Positive;
			}
			return KeywordScore.NegativeHigh;
		}

		public override void ProcessContract(Contract contract)
		{
		}
	}
}
