using System;
using System.Collections.Generic;
using KSP.UI;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace Expansions.Missions.Editor
{
	public class MissionValidationDialog : MonoBehaviour
	{
		private static MissionValidationDialog Instance;

		[SerializeField]
		private MissionValidationEntry validationEntryPrefab;

		[SerializeField]
		private Button btnValidate;

		[SerializeField]
		private ToggleGroup entryGroup;

		[SerializeField]
		private Button btnOK;

		[SerializeField]
		private TMP_Dropdown modeDropdown;

		private static Mission currentMission;

		private Callback afterOKCallback;

		private Callback afterCancelCallback;

		private string[] modeNames;

		public static MissionValidationDialog Display(Mission currentMission, Callback afterOKCallback = null, Callback afterCancelCallback = null)
		{
			if (Instance == null)
			{
				UnityEngine.Object @object = MissionsUtils.MEPrefab("_UI5/Dialogs/MissionValidationDialog/prefabs/MissionValidationDialog.prefab");
				if (@object == null)
				{
					Debug.LogError("[MissionBriefingDialog]: Unable to load the Asset");
					return null;
				}
				Instance = ((GameObject)UnityEngine.Object.Instantiate(@object)).GetComponent<MissionValidationDialog>();
			}
			Instance.transform.SetParent(DialogCanvasUtil.DialogCanvasRect, worldPositionStays: false);
			MissionValidationDialog.currentMission = currentMission;
			Instance.PrintValidationResults(currentMission.ValidationResults);
			Instance.afterOKCallback = afterOKCallback;
			Instance.afterCancelCallback = afterCancelCallback;
			return Instance;
		}

		private void Start()
		{
			BuildDropDown();
			btnOK.onClick.AddListener(OnOK);
			btnValidate.onClick.AddListener(OnButtonValidate);
		}

		private void BuildDropDown()
		{
			modeNames = Enum.GetNames(typeof(ValidatorMode));
			if (modeNames.Length == 0)
			{
				return;
			}
			modeDropdown.options.Clear();
			int value = 0;
			for (int i = 0; i < modeNames.Length; i++)
			{
				modeDropdown.options.Add(new TMP_Dropdown.OptionData(((ValidatorMode)i).displayDescription()));
				if (modeNames[i] == GameSettings.MISSION_VALIDATOR_MODE.ToString())
				{
					value = i;
				}
			}
			modeDropdown.value = value;
			modeDropdown.RefreshShownValue();
		}

		private void OnDestroy()
		{
			btnOK.onClick.RemoveListener(OnOK);
			btnValidate.onClick.RemoveListener(OnButtonValidate);
		}

		private void OnCancel()
		{
			if (afterCancelCallback != null)
			{
				afterCancelCallback();
			}
			DestroyDialog();
		}

		private void OnOK()
		{
			ValidatorMode value = (ValidatorMode)modeDropdown.value;
			if (value != GameSettings.MISSION_VALIDATOR_MODE)
			{
				GameSettings.MISSION_VALIDATOR_MODE = value;
				MissionEditorValidator.Instance.mode = value;
				GameSettings.SaveGameSettingsOnly();
			}
			if (afterOKCallback != null)
			{
				afterOKCallback();
			}
			DestroyDialog();
		}

		private void DestroyDialog()
		{
			UIMasterController.Instance.UnregisterNonModalDialog(GetComponent<CanvasGroup>());
			UnityEngine.Object.Destroy(base.gameObject);
			currentMission = null;
		}

		private void OnButtonValidate()
		{
			if (MissionEditorLogic.Instance == null)
			{
				Debug.LogError("Cant access the MissionEditorLogic Instance to do the validation");
			}
			MissionEditorLogic.Instance.RunValidator();
			PrintValidationResults(currentMission.ValidationResults);
		}

		private void PrintValidationResults(List<MissionValidationTestResult> results, bool includePasses = true)
		{
			bool flag = true;
			ClearValidationResults();
			if (results != null)
			{
				for (int i = 0; i < results.Count; i++)
				{
					MissionValidationTestResult missionValidationTestResult = results[i];
					if (includePasses || missionValidationTestResult.status != 0)
					{
						flag = false;
						validationEntryPrefab.Create(missionValidationTestResult, entryGroup);
					}
				}
			}
			if (flag)
			{
				validationEntryPrefab.Create(null, entryGroup);
			}
		}

		private void ClearValidationResults()
		{
			int i = 0;
			for (int childCount = entryGroup.transform.childCount; i < childCount; i++)
			{
				UnityEngine.Object.Destroy(entryGroup.transform.GetChild(i).gameObject);
			}
		}

		private void Update()
		{
			if (Input.GetKeyUp(KeyCode.Escape))
			{
				OnCancel();
			}
		}
	}
}
