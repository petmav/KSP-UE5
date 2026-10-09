namespace SteamController
{
	public static class KSPSteamControllerUtil
	{
		public static string GetModeName(this KSPSteamController.KSPControllerModes m)
		{
			switch (m)
			{
			default:
				return "Menu Controls";
			case KSPSteamController.KSPControllerModes.Flight:
				return "Flight Controls";
			case KSPSteamController.KSPControllerModes.Docking:
				return "Docking Controls";
			case KSPSteamController.KSPControllerModes.Editor:
				return "Editor Controls";
			case KSPSteamController.KSPControllerModes.Map:
				return "Map Controls";
			case KSPSteamController.KSPControllerModes.EVA:
				return "EVA Controls";
			}
		}
	}
}
