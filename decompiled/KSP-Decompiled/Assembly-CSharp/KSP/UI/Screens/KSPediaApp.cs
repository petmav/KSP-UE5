using KSPAssets.KSPedia;
using UnityEngine;

namespace KSP.UI.Screens
{
	public class KSPediaApp : UIApp
	{
		protected override void DisplayApp()
		{
			KSPediaSpawner.Show(base.appLauncherButton);
		}

		protected override void HideApp()
		{
			KSPediaSpawner.Hide();
		}

		protected override ApplicationLauncher.AppScenes GetAppScenes()
		{
			return ApplicationLauncher.AppScenes.ALWAYS;
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
		}
	}
}
