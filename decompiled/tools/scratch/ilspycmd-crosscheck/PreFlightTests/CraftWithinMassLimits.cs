using KSP.Localization;

namespace PreFlightTests
{
	public class CraftWithinMassLimits : IPreFlightTest
	{
		private double craftMass;

		private double maxMass;

		private string facilityName;

		private string shipName;

		private static string cacheAutoLOC_250677;

		private static string cacheAutoLOC_250692;

		public CraftWithinMassLimits(ShipConstruct ship, SpaceCenterFacility facility, double maxMass)
		{
			facilityName = Localizer.Format(KSPUtil.PrintLocalizedModuleName(facility.ToString()));
			craftMass = ship.GetTotalMass();
			this.maxMass = maxMass;
			shipName = ship.shipName;
		}

		public CraftWithinMassLimits(ShipTemplate template, SpaceCenterFacility facility, double maxMass)
		{
			facilityName = Localizer.Format(KSPUtil.PrintLocalizedModuleName(facility.ToString()));
			craftMass = template.totalMass;
			this.maxMass = maxMass;
			shipName = template.shipName;
		}

		public CraftWithinMassLimits(float mass, string name, SpaceCenterFacility facility, double maxMass)
		{
			facilityName = Localizer.Format(KSPUtil.PrintLocalizedModuleName(facility.ToString()));
			craftMass = mass;
			this.maxMass = maxMass;
			shipName = name;
		}

		public CraftWithinMassLimits(ShipConstruct ship, LaunchSite facility, double maxMass)
		{
			facilityName = KSPUtil.PrintModuleName(facility.ToString());
			craftMass = ship.GetTotalMass();
			this.maxMass = maxMass;
			shipName = ship.shipName;
		}

		public CraftWithinMassLimits(ShipTemplate template, LaunchSite facility, double maxMass)
		{
			facilityName = KSPUtil.PrintModuleName(facility.ToString());
			craftMass = template.totalMass;
			this.maxMass = maxMass;
			shipName = template.shipName;
		}

		public bool Test()
		{
			return craftMass <= maxMass;
		}

		public string GetWarningTitle()
		{
			return cacheAutoLOC_250677;
		}

		public string GetWarningDescription()
		{
			return Localizer.Format("#autoLOC_250682", facilityName, maxMass.ToString("0.##"), shipName, craftMass.ToString("0.##"));
		}

		public string GetProceedOption()
		{
			return null;
		}

		public string GetAbortOption()
		{
			return cacheAutoLOC_250692;
		}

		internal static void CacheLocalStrings()
		{
			cacheAutoLOC_250677 = Localizer.Format("#autoLOC_250677");
			cacheAutoLOC_250692 = Localizer.Format("#autoLOC_250692");
		}
	}
}
