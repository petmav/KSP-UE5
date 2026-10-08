using KSP.UI.Screens;
using UnityEngine;
using UnityEngine.UI;

public class DeltaVAppStageInfo : MonoBehaviour
{
	[SerializeField]
	private Button showAllButton;

	[SerializeField]
	private Button hideAllButton;

	[SerializeField]
	private DeltaVAppStageInfoToggle infoTogglePrefab;

	[SerializeField]
	private GridLayoutGroup infoToggles;

	internal int infoLineHeight;

	private void Awake()
	{
	}

	private void Start()
	{
		showAllButton.onClick.AddListener(ShowAllClicked);
		hideAllButton.onClick.AddListener(HideAllClicked);
		FillInfoToggles();
	}

	private void OnDestroy()
	{
		showAllButton.onClick.RemoveListener(ShowAllClicked);
		hideAllButton.onClick.RemoveListener(HideAllClicked);
	}

	private void FillInfoToggles()
	{
		DeltaVAppValues deltaVAppValues = DeltaVGlobals.DeltaVAppValues;
		for (int i = 0; i < deltaVAppValues.infoLines.Count; i++)
		{
			DeltaVAppValues.InfoLine info = deltaVAppValues.infoLines[i];
			Object.Instantiate(infoTogglePrefab).Setup(info, infoToggles.gameObject.transform);
		}
	}

	private void ShowAllClicked()
	{
		if (!(StageManager.Instance == null))
		{
			StageManager.Instance.ToggleInfoPanels(showPanels: true);
			StageManager.Instance.usageDV.allStagesShow++;
		}
	}

	private void HideAllClicked()
	{
		if (!(StageManager.Instance == null))
		{
			StageManager.Instance.ToggleInfoPanels(showPanels: false);
			StageManager.Instance.usageDV.allStagesHide++;
		}
	}

	internal void SetColumnLayout(bool multiColumn)
	{
		int num = ((!multiColumn) ? 1 : 2);
		float num2 = (infoToggles.transform as RectTransform).sizeDelta.x;
		if (num2 == 0f)
		{
			num2 = (base.transform as RectTransform).rect.width - 10f;
		}
		infoToggles.constraintCount = num;
		infoToggles.cellSize = new Vector2(num2 / (float)num, infoLineHeight);
	}
}
