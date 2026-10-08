using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	public class ResourceDisplayOptions : MonoBehaviour
	{
		public UIStateButton btnStage;

		public Button background;

		public void Setup(bool showStage)
		{
			btnStage.SetState(showStage ? 1 : 0);
			btnStage.onClick.AddListener(OnStageInput);
			background.onClick.AddListener(OnStageInput);
		}

		public void OnStageInput()
		{
			ResourceDisplay.Instance.ToggleStageView();
		}
	}
}
