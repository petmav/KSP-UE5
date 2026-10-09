using UnityEngine;

namespace KSP.UI.Screens
{
	public class CargoApp : UIApp
	{
		protected override void DisplayApp()
		{
			if ((bool)EVAConstructionModeController.Instance)
			{
				EVAConstructionModeController.Instance.OpenCargoPanel();
			}
		}

		protected override void HideApp()
		{
			if ((bool)EVAConstructionModeController.Instance)
			{
				EVAConstructionModeController.Instance.ClosePanel();
			}
		}

		protected override ApplicationLauncher.AppScenes GetAppScenes()
		{
			return ApplicationLauncher.AppScenes.FLIGHT;
		}

		protected override Vector3 GetAppScreenPos(Vector3 defaultAnchorPos)
		{
			return new Vector3(ApplicationLauncher.Instance.transform.position.x, defaultAnchorPos.y, defaultAnchorPos.z);
		}

		protected override bool OnAppAboutToStart()
		{
			return true;
		}

		protected override void OnAppDestroy()
		{
		}

		protected override void OnAppInitialized()
		{
			if ((bool)EVAConstructionModeController.Instance)
			{
				EVAConstructionModeController.Instance.RegisterAppButtonCargo(base.appLauncherButton);
			}
		}
	}
}
