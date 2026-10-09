using System;

namespace Contracts.Parameters
{
	[Serializable]
	public class OR : ContractParameter
	{
		protected override string GetHashString()
		{
			return "OR";
		}

		protected override void OnParameterStateChange(ContractParameter p)
		{
			int num = 0;
			while (true)
			{
				if (num < base.ParameterCount)
				{
					if (GetParameter(num).State == ParameterState.Complete)
					{
						break;
					}
					num++;
					continue;
				}
				return;
			}
			SetComplete();
		}
	}
}
