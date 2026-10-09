using UnityEngine;

namespace KSP.UI
{
	public class Tooltip : MonoBehaviour
	{
		private RectTransform rectTransform;

		public RectTransform RectTransform => rectTransform;

		protected void Awake()
		{
			rectTransform = base.transform as RectTransform;
		}
	}
}
