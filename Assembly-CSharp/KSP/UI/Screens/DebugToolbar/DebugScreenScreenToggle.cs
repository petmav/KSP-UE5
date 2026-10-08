using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.DebugToolbar
{
	public class DebugScreenScreenToggle : MonoBehaviour
	{
		public GameObject shownObject;

		public GameObject hiddenObject;

		public Button toggleButton;

		private bool isShown = true;

		private void Awake()
		{
			toggleButton.onClick.AddListener(OnToggleClick);
			SetShown();
		}

		private void OnToggleClick()
		{
			isShown = !isShown;
			SetShown();
		}

		private void SetShown()
		{
			if (isShown)
			{
				if (shownObject != null && !shownObject.activeSelf)
				{
					shownObject.SetActive(value: true);
				}
				if (hiddenObject != null && hiddenObject.activeSelf)
				{
					hiddenObject.SetActive(value: false);
				}
			}
			else
			{
				if (shownObject != null && shownObject.activeSelf)
				{
					shownObject.SetActive(value: false);
				}
				if (hiddenObject != null && !hiddenObject.activeSelf)
				{
					hiddenObject.SetActive(value: true);
				}
			}
		}
	}
}
