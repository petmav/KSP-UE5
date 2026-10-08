using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Util
{
	public class ImgText : MonoBehaviour
	{
		[SerializeField]
		private TextMeshProUGUI textComponent;

		[SerializeField]
		private Image imgComponent;

		public string text
		{
			get
			{
				return textComponent.text;
			}
			set
			{
				textComponent.text = value;
			}
		}

		public Sprite sprite
		{
			get
			{
				return imgComponent.sprite;
			}
			set
			{
				imgComponent.sprite = value;
			}
		}
	}
}
