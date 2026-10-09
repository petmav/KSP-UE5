using TMPro;
using UnityEngine;

namespace KSP.UI.Screens.DebugToolbar.Screens.Debug
{
	public class ScreenDeltaVPartsListInfo : MonoBehaviour
	{
		[SerializeField]
		private TextMeshProUGUI partsListText;

		public void UpdateData(string partsText)
		{
			partsListText.text = partsText;
		}
	}
}
