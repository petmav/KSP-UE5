using System.Collections.Generic;
using KSP.Localization;

namespace PreFlightTests
{
	public class RequiredIScienceDataTransmitter : DesignConcernBase
	{
		private ShipConstruct ship;

		private List<Part> failedParts = new List<Part>();

		private static string cacheAutoLOC_252827;

		private static string cacheAutoLOC_252834;

		private static string cacheAutoLOC_252835;

		public override bool TestCondition()
		{
			ship = EditorLogic.fetch.ship;
			failedParts.Clear();
			if (!RUIutils.Any(ship.parts, (Part a) => a.FindModuleImplementing<IScienceDataTransmitter>() != null))
			{
				failedParts = RUIutils.Where(ship.parts, (Part a) => a.FindModuleImplementing<ModuleOrbitalSurveyor>() != null);
			}
			return failedParts.Count == 0;
		}

		public override string GetConcernTitle()
		{
			_ = failedParts.Count;
			return cacheAutoLOC_252827;
		}

		public override string GetConcernDescription()
		{
			if (failedParts.Count == 1)
			{
				return cacheAutoLOC_252834;
			}
			return cacheAutoLOC_252835;
		}

		public override DesignConcernSeverity GetSeverity()
		{
			return DesignConcernSeverity.CRITICAL;
		}

		public override List<Part> GetAffectedParts()
		{
			return failedParts;
		}

		internal static void CacheLocalStrings()
		{
			cacheAutoLOC_252827 = Localizer.Format("#autoLOC_252827");
			cacheAutoLOC_252834 = Localizer.Format("#autoLOC_252834");
			cacheAutoLOC_252835 = Localizer.Format("#autoLOC_252835");
		}
	}
}
