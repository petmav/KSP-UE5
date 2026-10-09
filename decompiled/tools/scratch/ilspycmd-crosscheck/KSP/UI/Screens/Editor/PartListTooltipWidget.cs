using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.Editor
{
	public class PartListTooltipWidget : MonoBehaviour
	{
		public TextMeshProUGUI textName;

		public TextMeshProUGUI textInfo;

		public Image widgetColourImage;

		public void Setup(string name, string info)
		{
			textName.text = name;
			textInfo.text = info;
		}
	}
}
