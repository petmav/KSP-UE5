using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI
{
	public class ProgressBar : MonoBehaviour
	{
		private Image foregroundImage;

		public float Value
		{
			get
			{
				if (foregroundImage != null)
				{
					return foregroundImage.fillAmount;
				}
				return 0f;
			}
			set
			{
				if (foregroundImage != null)
				{
					foregroundImage.fillAmount = value;
				}
			}
		}

		private void Start()
		{
			Value = 0f;
			foregroundImage = base.gameObject.GetComponent<Image>();
		}
	}
}
