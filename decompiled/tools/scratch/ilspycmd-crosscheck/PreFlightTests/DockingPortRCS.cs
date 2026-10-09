using System.Collections.Generic;
using KSP.Localization;
using KSP.UI.Screens;

namespace PreFlightTests
{
	public class DockingPortRCS : DesignConcernBase
	{
		private ShipConstruct ship;

		private List<Part> failedParts = new List<Part>();

		private static string cacheAutoLOC_251727;

		private static string cacheAutoLOC_251732;

		public override bool TestCondition()
		{
			ship = EditorLogic.fetch.ship;
			failedParts = StageManager.FindPartsWithModuleSeparatingBeforeOtherPartsWithModule<ModuleDockingNode, ModuleRCS>(ship.parts);
			return failedParts.Count == 0;
		}

		public override string GetConcernTitle()
		{
			return cacheAutoLOC_251727;
		}

		public override string GetConcernDescription()
		{
			return cacheAutoLOC_251732;
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
			cacheAutoLOC_251727 = Localizer.Format("#autoLOC_251727");
			cacheAutoLOC_251732 = Localizer.Format("#autoLOC_251732");
		}
	}
}
