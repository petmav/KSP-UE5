using Expansions;
using KSP.Localization;
using TMPro;
using UnityEngine;

namespace KSP.UI.Screens.DebugToolbar.Screens.Debug
{
	public class VersionInfo : MonoBehaviour
	{
		[SerializeField]
		private TextMeshProUGUI version_text;

		private void Start()
		{
			if (version_text != null)
			{
				version_text.text = Versioning.GetVersionStringFull() + " " + Localizer.CurrentLanguage;
				version_text.text += ExpansionsLoader.GetInstalledExpansionsString();
			}
		}
	}
}
