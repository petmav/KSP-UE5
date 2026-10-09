using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	public class CurrencyWidgetsApp : UIApp
	{
		public CurrencyWidget[] widgets;

		[SerializeField]
		private SimpleAppFrame appFramePrefab;

		private SimpleAppFrame appFrame;

		private LayoutGroup layout;

		protected override bool OnAppAboutToStart()
		{
			Debug.Log("CURRENCY WIDGET " + (Funding.Instance != null) + " " + (Reputation.Instance != null) + " " + (ResearchAndDevelopment.Instance != null));
			if (Funding.Instance != null && Reputation.Instance != null && ResearchAndDevelopment.Instance != null)
			{
				return true;
			}
			Object.Destroy(base.gameObject);
			return false;
		}

		protected override ApplicationLauncher.AppScenes GetAppScenes()
		{
			return ApplicationLauncher.AppScenes.SPACECENTER | ApplicationLauncher.AppScenes.FLIGHT | ApplicationLauncher.AppScenes.MAPVIEW;
		}

		protected override Vector3 GetAppScreenPos(Vector3 defaultAnchorPos)
		{
			return new Vector3(ApplicationLauncher.Instance.transform.position.x, defaultAnchorPos.y, defaultAnchorPos.z);
		}

		protected override void OnAppInitialized()
		{
			appFrame = Object.Instantiate(appFramePrefab);
			appFrame.transform.SetParent(base.transform, worldPositionStays: false);
			appFrame.transform.localPosition = Vector3.zero;
			appFrame.Setup(base.appLauncherButton, "CurrencyWidget");
			layout = appFrame.GetComponentInChildren<LayoutGroup>();
			SpawnWidgets();
			HideApp();
		}

		protected override void OnAppDestroy()
		{
			if (appFrame != null)
			{
				appFrame.gameObject.DestroyGameObject();
			}
		}

		protected override void DisplayApp()
		{
			if (appFrame != null)
			{
				appFrame.gameObject.SetActive(value: true);
			}
		}

		protected override void HideApp()
		{
			if (appFrame != null)
			{
				appFrame.gameObject.SetActive(value: false);
			}
		}

		private void SpawnWidgets()
		{
			if (layout.transform.childCount == 0)
			{
				int num = widgets.Length;
				for (int i = 0; i < num; i++)
				{
					CurrencyWidget currencyWidget = Object.Instantiate(widgets[i]);
					currencyWidget.transform.SetParent(layout.transform, worldPositionStays: false);
					if (currencyWidget is ScienceWidget)
					{
						currencyWidget.gameObject.GetChild("Background").GetComponent<Image>().material = null;
						currencyWidget.gameObject.GetChild("Foreground").GetComponent<Image>().material = null;
					}
				}
			}
			else
			{
				Debug.LogWarning("[CurrencyWidgetsApp]: Widgets already spawned!", base.gameObject);
			}
		}

		private void DespawnWidgets()
		{
			if (layout.transform.childCount != 0)
			{
				int childCount = layout.transform.childCount;
				for (int i = 0; i < childCount; i++)
				{
					Object.Destroy(layout.transform.GetChild(i).gameObject);
				}
			}
			else
			{
				Debug.LogWarning("[CurrencyWidgetsApp]: No live widgets to Despawn!", base.gameObject);
			}
		}
	}
}
