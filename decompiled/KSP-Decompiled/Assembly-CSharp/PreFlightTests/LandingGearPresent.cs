using KSP.Localization;

namespace PreFlightTests
{
	public class LandingGearPresent : DesignConcernBase
	{
		private ShipConstruct ship;

		private bool roverWheelsPresent;

		private static string cacheAutoLOC_252437;

		private static string cacheAutoLOC_252438;

		private static string cacheAutoLOC_252444;

		private static string cacheAutoLOC_252445;

		public LandingGearPresent(EditorFacility facility)
		{
		}

		public override bool TestCondition()
		{
			ship = EditorLogic.fetch.ship;
			roverWheelsPresent = false;
			int count = ship.Count;
			for (int i = 0; i < count; i++)
			{
				ModuleWheelBase moduleWheelBase = ship[i].FindModuleImplementing<ModuleWheelBase>();
				if (moduleWheelBase != null)
				{
					switch (moduleWheelBase.wheelType)
					{
					case WheelType.MOTORIZED:
						roverWheelsPresent = true;
						break;
					case WheelType.FREE:
						return true;
					}
				}
			}
			return false;
		}

		public override string GetConcernTitle()
		{
			if (roverWheelsPresent)
			{
				return cacheAutoLOC_252437;
			}
			return cacheAutoLOC_252438;
		}

		public override string GetConcernDescription()
		{
			if (roverWheelsPresent)
			{
				return cacheAutoLOC_252444;
			}
			return cacheAutoLOC_252445;
		}

		public override EditorFacilities GetEditorFacilities()
		{
			return EditorFacilities.SPH;
		}

		public override DesignConcernSeverity GetSeverity()
		{
			return DesignConcernSeverity.WARNING;
		}

		internal static void CacheLocalStrings()
		{
			cacheAutoLOC_252437 = Localizer.Format("#autoLOC_252437");
			cacheAutoLOC_252438 = Localizer.Format("#autoLOC_252438");
			cacheAutoLOC_252444 = Localizer.Format("#autoLOC_252444");
			cacheAutoLOC_252445 = Localizer.Format("#autoLOC_252445");
		}
	}
}
