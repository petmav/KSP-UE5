using Expansions.Missions.Runtime;
using KSP.Localization;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace Expansions.Missions
{
	public class AwardWidget : MonoBehaviour
	{
		public Image awardIcon;

		public TextMeshProUGUI awardText;

		public TextMeshProUGUI descriptionText;

		protected AwardDefinition awardDefinition;

		public AwardWidget Create(string awardId, Transform parent)
		{
			AwardWidget awardWidget = Object.Instantiate(this, parent);
			Awards awards = MissionSystem.awardDefinitions;
			if (awards == null)
			{
				awards = Object.FindObjectOfType<Awards>();
				if (awards == null)
				{
					GameObject gameObject = MissionsUtils.MEPrefab("Prefabs/MEAwards.prefab");
					if (gameObject != null)
					{
						awards = Object.Instantiate(gameObject).GetComponent<Awards>();
					}
				}
			}
			if (awards != null)
			{
				awardWidget.awardDefinition = awards.GetAwardDefinition(awardId);
			}
			return awardWidget;
		}

		private void Start()
		{
			awardIcon.sprite = awardDefinition.icon;
			awardText.text = $"<color=#FCCB44FF>{Localizer.Format(awardDefinition.displayName)}</color>";
			descriptionText.text = $"<color=#EDFEAAFF>{Localizer.Format(awardDefinition.description)}</color>";
		}
	}
}
