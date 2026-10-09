using TMPro;
using UnityEngine;

namespace KSP.UI.Screens.DebugToolbar.Screens.Mission
{
	public class ScreenMissionNewItem : MonoBehaviour
	{
		public TextMeshProUGUI titleText;

		public TextMeshProUGUI assemblytext;

		private void Start()
		{
		}

		public void SetupError(string errorText)
		{
			titleText.text = "<color=red>" + errorText + "</color>";
			titleText.enableWordWrapping = false;
			titleText.overflowMode = TextOverflowModes.Overflow;
			assemblytext.text = string.Empty;
		}
	}
}
