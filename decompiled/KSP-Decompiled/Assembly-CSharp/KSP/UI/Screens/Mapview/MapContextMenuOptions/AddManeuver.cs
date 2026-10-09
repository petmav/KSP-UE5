using KSP.Localization;

namespace KSP.UI.Screens.Mapview.MapContextMenuOptions
{
	public class AddManeuver : MapContextMenuOption
	{
		private PatchedConicRenderer pcr;

		private double UT;

		public AddManeuver(PatchedConicRenderer pcr, double UT)
			: base("#autoLOC_465466")
		{
			this.pcr = pcr;
			this.UT = UT;
		}

		protected override void OnSelect()
		{
			if (InputLockManager.IsUnlocked(ControlTypes.MANNODE_ADDEDIT))
			{
				pcr.AddManeuverNode(UT);
			}
			else
			{
				ScreenMessages.PostScreenMessage(Localizer.Format("#autoLOC_465476"), 3f, ScreenMessageStyle.UPPER_CENTER);
			}
		}
	}
}
