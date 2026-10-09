using Expansions;
using KSP.Localization;
using TMPro;
using UnityEngine;

namespace KSP.UI
{
	public class ExperimentalVersionReadout : MonoBehaviour
	{
		public TextMeshProUGUI versionText;

		private void Start()
		{
			GameEvents.onLevelWasLoaded.Add(OnLevelLoaded);
			base.gameObject.SetActive(value: false);
		}

		private void OnDestroy()
		{
			GameEvents.onLevelWasLoaded.Remove(OnLevelLoaded);
		}

		private void OnLevelLoaded(GameScenes scene)
		{
			if ((!GameSettings.SHOW_VERSION_WATERMARK && !Versioning.isPrerelease && !Versioning.isBeta && Versioning.Experimental <= 0) || (scene != GameScenes.EDITOR && scene != GameScenes.FLIGHT && scene != GameScenes.SPACECENTER && scene != GameScenes.TRACKSTATION && scene != GameScenes.MISSIONBUILDER))
			{
				base.gameObject.SetActive(value: false);
				return;
			}
			base.gameObject.SetActive(value: true);
			versionText.text = Versioning.GetVersionStringFull() + " " + Localizer.CurrentLanguage;
			versionText.text += ExpansionsLoader.GetInstalledExpansionsString();
		}
	}
}
