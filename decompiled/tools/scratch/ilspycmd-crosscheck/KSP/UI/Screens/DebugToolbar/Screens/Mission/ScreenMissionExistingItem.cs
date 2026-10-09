using Expansions.Missions;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.DebugToolbar.Screens.Mission
{
	public class ScreenMissionExistingItem : MonoBehaviour
	{
		public TextMeshProUGUI titleText;

		public TextMeshProUGUI stateText;

		public TextMeshProUGUI prestigeText;

		public Button leftButton;

		public TextMeshProUGUI leftButtonText;

		public Button rightButton;

		public TextMeshProUGUI rightButtonText;

		public TextMeshProUGUI errorText;

		public Expansions.Missions.Mission mission { get; private set; }

		private void Start()
		{
			leftButton.onClick.AddListener(OnLeftButtonClicked);
			rightButton.onClick.AddListener(OnRightButtonClicked);
		}

		public void Setup(Expansions.Missions.Mission mission)
		{
			if (!(mission == null))
			{
				this.mission = mission;
				titleText.text = mission.title;
			}
		}

		public void SetupError(string errorText)
		{
			mission = null;
			titleText.text = "<color=red>" + errorText + "</color>";
			titleText.enableWordWrapping = false;
			titleText.overflowMode = TextOverflowModes.Overflow;
			stateText.text = string.Empty;
			prestigeText.text = string.Empty;
			leftButton.gameObject.SetActive(value: false);
			rightButton.gameObject.SetActive(value: false);
		}

		private void OnLeftButtonClicked()
		{
			_ = mission == null;
		}

		private void OnRightButtonClicked()
		{
			_ = mission == null;
		}
	}
}
