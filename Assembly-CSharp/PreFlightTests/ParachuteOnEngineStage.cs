using System.Collections.Generic;
using KSP.Localization;

namespace PreFlightTests
{
	public class ParachuteOnEngineStage : DesignConcernBase
	{
		private ShipConstruct ship;

		private List<Part> failedParts = new List<Part>();

		private static string cacheAutoLOC_252616;

		private static string cacheAutoLOC_252617;

		private static string cacheAutoLOC_252623;

		private static string cacheAutoLOC_252624;

		public override bool TestCondition()
		{
			ship = EditorLogic.fetch.ship;
			failedParts.Clear();
			List<Part> list = RUIutils.Where(ship.parts, (Part a) => a.FindModuleImplementing<ModuleParachute>() != null);
			List<Part> engines = RUIutils.Where(ship.parts, (Part a) => a.FindModuleImplementing<ModuleEngines>() != null || a.FindModuleImplementing<ModuleEnginesFX>() != null);
			failedParts = RUIutils.WhereMutating(ref list, (Part a) => RUIutils.Any(engines, (Part b) => a.inverseStage == b.inverseStage));
			return failedParts.Count == 0;
		}

		public override string GetConcernTitle()
		{
			if (failedParts.Count == 1)
			{
				return cacheAutoLOC_252616;
			}
			return cacheAutoLOC_252617;
		}

		public override string GetConcernDescription()
		{
			if (failedParts.Count == 1)
			{
				return cacheAutoLOC_252623;
			}
			return cacheAutoLOC_252624;
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
			cacheAutoLOC_252616 = Localizer.Format("#autoLOC_252616");
			cacheAutoLOC_252617 = Localizer.Format("#autoLOC_252617");
			cacheAutoLOC_252623 = Localizer.Format("#autoLOC_252623");
			cacheAutoLOC_252624 = Localizer.Format("#autoLOC_252624");
		}
	}
}
