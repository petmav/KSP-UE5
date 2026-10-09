using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	public class VesselIconSprite : MonoBehaviour
	{
		[SerializeField]
		private Image image;

		[SerializeField]
		private Sprite[] vesselTypeIcons;

		private VesselType type;

		public VesselType vesselType => type;

		public void SetType(VesselType t)
		{
			if (t != type)
			{
				type = t;
				if ((int)t > vesselTypeIcons.Length - 1)
				{
					image.sprite = vesselTypeIcons[2];
				}
				else
				{
					image.sprite = vesselTypeIcons[(int)t];
				}
			}
		}
	}
}
