using System.Collections.Generic;

namespace KSP.UI.Screens.DebugToolbar.Screens.Debug
{
	public static class DeltaVPropellantInfoListExtensions
	{
		public static ScreenDeltaVPropellantInfo Get(this List<ScreenDeltaVPropellantInfo> list, DeltaVPropellantInfo propellantInfo)
		{
			int num = 0;
			while (true)
			{
				if (num < list.Count)
				{
					if (list[num].propellantInfo == propellantInfo)
					{
						break;
					}
					num++;
					continue;
				}
				return null;
			}
			return list[num];
		}
	}
}
