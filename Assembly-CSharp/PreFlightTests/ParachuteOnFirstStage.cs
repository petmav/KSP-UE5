using System.Collections.Generic;
using KSP.Localization;
using KSP.UI.Screens;

namespace PreFlightTests
{
	public class ParachuteOnFirstStage : DesignConcernBase
	{
		private ShipConstruct ship;

		private List<Part> failedParts = new List<Part>();

		private static string cacheAutoLOC_252675;

		private static string cacheAutoLOC_252676;

		private static string cacheAutoLOC_252682;

		private static string cacheAutoLOC_252683;

		public override bool TestCondition()
		{
			ship = EditorLogic.fetch.ship;
			failedParts.Clear();
			int count = ship.Count;
			for (int i = 0; i < count; i++)
			{
				Part part = ship[i];
				if (part.isParachute() && part.inverseStage == StageManager.LastStage)
				{
					failedParts.Add(part);
				}
			}
			return failedParts.Count == 0;
		}

		public override string GetConcernTitle()
		{
			if (failedParts.Count == 1)
			{
				return cacheAutoLOC_252675;
			}
			return cacheAutoLOC_252676;
		}

		public override string GetConcernDescription()
		{
			if (failedParts.Count == 1)
			{
				return cacheAutoLOC_252682;
			}
			return cacheAutoLOC_252683;
		}

		public override DesignConcernSeverity GetSeverity()
		{
			return DesignConcernSeverity.WARNING;
		}

		public override List<Part> GetAffectedParts()
		{
			return failedParts;
		}

		internal static void CacheLocalStrings()
		{
			cacheAutoLOC_252675 = Localizer.Format("#autoLOC_252675");
			cacheAutoLOC_252676 = Localizer.Format("#autoLOC_252676");
			cacheAutoLOC_252682 = Localizer.Format("#autoLOC_252682");
			cacheAutoLOC_252683 = Localizer.Format("#autoLOC_252683");
		}
	}
}
