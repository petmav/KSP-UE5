using KSP.Localization;

namespace PreFlightTests
{
	public class CraftWithinPartCountLimit : IPreFlightTest
	{
		private int partCount;

		private int maxParts;

		private string facilityName;

		private static string cacheAutoLOC_250727;

		private static string cacheAutoLOC_250742;

		public CraftWithinPartCountLimit(ShipConstruct ship, SpaceCenterFacility facility, int partLimit)
		{
			partCount = ship.parts.Count;
			facilityName = Localizer.Format(KSPUtil.PrintLocalizedModuleName(facility.ToString()));
			maxParts = partLimit;
		}

		public CraftWithinPartCountLimit(ShipTemplate template, SpaceCenterFacility facility, int partLimit)
		{
			partCount = template.partCount;
			facilityName = Localizer.Format(KSPUtil.PrintLocalizedModuleName(facility.ToString()));
			maxParts = partLimit;
		}

		public CraftWithinPartCountLimit(int count, SpaceCenterFacility facility, int partLimit)
		{
			partCount = count;
			facilityName = Localizer.Format(KSPUtil.PrintLocalizedModuleName(facility.ToString()));
			maxParts = partLimit;
		}

		public bool Test()
		{
			return partCount <= maxParts;
		}

		public string GetWarningTitle()
		{
			return cacheAutoLOC_250727;
		}

		public string GetWarningDescription()
		{
			return Localizer.Format("#autoLOC_250732", facilityName, maxParts.ToString());
		}

		public string GetProceedOption()
		{
			return null;
		}

		public string GetAbortOption()
		{
			return cacheAutoLOC_250742;
		}

		internal static void CacheLocalStrings()
		{
			cacheAutoLOC_250727 = Localizer.Format("#autoLOC_250727");
			cacheAutoLOC_250742 = Localizer.Format("#autoLOC_250742");
		}
	}
}
