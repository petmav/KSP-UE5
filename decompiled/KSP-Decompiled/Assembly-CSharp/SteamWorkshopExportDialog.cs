using System.Diagnostics;
using Expansions;
using KSP.Localization;
using KSP.UI;
using KSP.UI.Screens;
using TMPro;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.UI;

public class SteamWorkshopExportDialog : MonoBehaviour
{
	public class ReturnItems
	{
		public bool setPublic;

		public string changeLog;

		public string modsText;

		public VesselType vesselType;

		public bool roboticTag;
	}

	[Header("UI Components")]
	[SerializeField]
	private TextMeshProUGUI textHeader;

	[SerializeField]
	private TextMeshProUGUI textDescription;

	[SerializeField]
	private TextMeshProUGUI textCancel;

	[SerializeField]
	private Button buttonCancel;

	[SerializeField]
	private TextMeshProUGUI textConfirmation;

	[SerializeField]
	private Button buttonConfirmation;

	[SerializeField]
	private TextMeshProUGUI textWorkshopVisibility;

	[SerializeField]
	private Toggle toggleWorkshopVisibility;

	[SerializeField]
	private GameObject steamChangeLogObject;

	[SerializeField]
	private TextMeshProUGUI steamChangeLog;

	[SerializeField]
	private GameObject modsObject;

	[SerializeField]
	private TextMeshProUGUI modsText;

	[SerializeField]
	private Button modsPopulateButton;

	[SerializeField]
	private Button steamLegalAgreement;

	[SerializeField]
	private GameObject vesselTypeOptions;

	private TypeButton selectedToggle;

	[SerializeField]
	private ToggleGroup typeIconsGroup;

	[SerializeField]
	private TypeButton toggleShip;

	[SerializeField]
	private TypeButton toggleLander;

	[SerializeField]
	private TypeButton toggleRover;

	[SerializeField]
	private TypeButton toggleStation;

	[SerializeField]
	private TypeButton toggleProbe;

	[SerializeField]
	private TypeButton toggleBase;

	[SerializeField]
	private TypeButton toggleAircraft;

	[SerializeField]
	private TypeButton toggleCommunicationsRelay;

	[SerializeField]
	private Toggle roboticTagButton;

	public bool modal = true;

	private CanvasGroup canvasGroup;

	private Callback<ReturnItems> onOk;

	private Callback<bool> onCancel;

	private bool showVisibilityOption;

	private bool showChangelog;

	private bool showVesselTypeSection;

	private VesselType vesselType;

	public static SteamWorkshopExportDialog Spawn(string title, string message, Callback<ReturnItems> onOk, Callback<bool> onCancel, bool showCancelBtn = true, bool showVisibilityOption = true, bool showChangeLog = true, bool showModsSection = false, bool showVesselTypeSection = false, VesselType vesselType = VesselType.Ship)
	{
		SteamWorkshopExportDialog component = Object.Instantiate(AssetBase.GetPrefab("SteamWorkshopExportDialog")).GetComponent<SteamWorkshopExportDialog>();
		component.transform.SetParent(PopupDialogController.PopupDialogCanvas.transform, worldPositionStays: true);
		component.transform.localScale = Vector3.one;
		component.transform.localPosition = Vector3.zero;
		component.textHeader.text = title;
		component.textDescription.text = message;
		component.onOk = onOk;
		component.onCancel = onCancel;
		component.showVisibilityOption = showVisibilityOption;
		component.showChangelog = showChangeLog;
		component.showVesselTypeSection = showVesselTypeSection;
		component.vesselType = vesselType;
		if (!showCancelBtn)
		{
			component.buttonCancel.gameObject.SetActive(value: false);
		}
		if (!showModsSection)
		{
			component.modsObject.SetActive(value: false);
		}
		if (!showVesselTypeSection)
		{
			component.vesselTypeOptions.SetActive(value: false);
			component.typeIconsGroup.gameObject.SetActive(value: false);
		}
		if (component.roboticTagButton != null && !ExpansionsLoader.IsExpansionInstalled("Serenity"))
		{
			component.roboticTagButton.gameObject.SetActive(value: false);
		}
		return component;
	}

	public static SteamWorkshopExportDialog Spawn(string title, string message, Callback<ReturnItems> onOk, bool showCancelBtn = true, bool showVisibilityOption = true, bool showChangeLog = true, bool showModsSection = false, bool showVesselTypeSection = false, VesselType vesselType = VesselType.Ship)
	{
		return Spawn(title, message, onOk, null, showCancelBtn, showVisibilityOption, showChangeLog, showModsSection, showVesselTypeSection, vesselType);
	}

	public static SteamWorkshopExportDialog Spawn(string title, string message, string textCancel, string textOK, string textDontShowAgain, Callback<ReturnItems> onOk, Callback<bool> onCancel, bool showCancelBtn = true, bool showVisibilityOption = true, bool showChangeLog = true, bool showModsSection = false, bool showVesselTypeSection = false, VesselType vesselType = VesselType.Ship)
	{
		SteamWorkshopExportDialog steamWorkshopExportDialog = Spawn(title, message, onOk, onCancel, showCancelBtn, showVisibilityOption, showChangeLog, showModsSection, showVesselTypeSection, vesselType);
		steamWorkshopExportDialog.textCancel.text = textCancel;
		steamWorkshopExportDialog.textConfirmation.text = textOK;
		steamWorkshopExportDialog.textWorkshopVisibility.text = textDontShowAgain;
		return steamWorkshopExportDialog;
	}

	private void Start()
	{
		buttonConfirmation.onClick.AddListener(OnConfirm);
		buttonCancel.onClick.AddListener(OnCancel);
		steamLegalAgreement.onClick.AddListener(onBtnLegalAgreement);
		modsPopulateButton.onClick.AddListener(OnModsList);
		canvasGroup = GetComponent<CanvasGroup>();
		if (!showVisibilityOption)
		{
			toggleWorkshopVisibility.gameObject.SetActive(value: false);
		}
		if (!showChangelog)
		{
			steamChangeLogObject.SetActive(value: false);
		}
		if (!showVesselTypeSection)
		{
			vesselTypeOptions.SetActive(value: false);
			typeIconsGroup.gameObject.SetActive(value: false);
		}
		else
		{
			ToggleSetup(toggleBase, delegate(bool b)
			{
				OnToggle(toggleBase, b);
			});
			ToggleSetup(toggleLander, delegate(bool b)
			{
				OnToggle(toggleLander, b);
			});
			ToggleSetup(toggleProbe, delegate(bool b)
			{
				OnToggle(toggleProbe, b);
			});
			ToggleSetup(toggleRover, delegate(bool b)
			{
				OnToggle(toggleRover, b);
			});
			ToggleSetup(toggleShip, delegate(bool b)
			{
				OnToggle(toggleShip, b);
			});
			ToggleSetup(toggleStation, delegate(bool b)
			{
				OnToggle(toggleStation, b);
			});
			ToggleSetup(toggleAircraft, delegate(bool b)
			{
				OnToggle(toggleAircraft, b);
			});
			ToggleSetup(toggleCommunicationsRelay, delegate(bool b)
			{
				OnToggle(toggleCommunicationsRelay, b);
			});
			if (selectedToggle == null)
			{
				toggleShip.Select();
			}
		}
		if (modal)
		{
			UIMasterController.Instance.RegisterModalDialog(canvasGroup);
		}
		else
		{
			UIMasterController.Instance.RegisterNonModalDialog(canvasGroup);
		}
	}

	protected void ToggleSetup(TypeButton t, UnityAction<bool> onValueChangedCallback)
	{
		typeIconsGroup.RegisterToggle(t.toggle);
		t.toggle.group = typeIconsGroup;
		t.toggle.onValueChanged.AddListener(onValueChangedCallback);
		if (t.type == vesselType)
		{
			t.Select();
			selectedToggle = t;
		}
	}

	protected void OnToggle(TypeButton t, bool b)
	{
		if (b && t != selectedToggle)
		{
			if (selectedToggle != null)
			{
				selectedToggle.Deselect();
			}
			vesselType = t.type;
			selectedToggle = t;
			t.Select();
		}
	}

	public void OnConfirm()
	{
		if (onOk != null)
		{
			ReturnItems returnItems = new ReturnItems();
			returnItems.setPublic = toggleWorkshopVisibility.isOn;
			returnItems.changeLog = steamChangeLog.text;
			returnItems.modsText = modsText.text;
			returnItems.vesselType = vesselType;
			returnItems.roboticTag = roboticTagButton.isOn;
			onOk(returnItems);
		}
		CloseDialog();
	}

	public void OnCancel()
	{
		if (onCancel != null)
		{
			onCancel(toggleWorkshopVisibility.isOn);
		}
		CloseDialog();
	}

	public void CloseDialog()
	{
		Object.Destroy(base.gameObject);
	}

	private void OnDestroy()
	{
		if (modal)
		{
			UIMasterController.Instance.UnregisterModalDialog(canvasGroup);
		}
		else
		{
			UIMasterController.Instance.UnregisterNonModalDialog(canvasGroup);
		}
		buttonConfirmation.onClick.RemoveListener(OnConfirm);
		buttonCancel.onClick.RemoveListener(OnCancel);
		steamLegalAgreement.onClick.RemoveListener(onBtnLegalAgreement);
		modsPopulateButton.onClick.RemoveListener(OnModsList);
	}

	private void onBtnLegalAgreement()
	{
		Process.Start("https://steamcommunity.com/sharedfiles/workshoplegalagreement");
	}

	private void OnModsList()
	{
		TextMeshProUGUI textMeshProUGUI = modsText;
		textMeshProUGUI.text = textMeshProUGUI.text + "\n" + Localizer.Format("#autoLOC_8001027");
		int i = 0;
		for (int count = GameDatabase.loadedModsInfo.Count; i < count; i++)
		{
			TextMeshProUGUI textMeshProUGUI2 = modsText;
			textMeshProUGUI2.text = textMeshProUGUI2.text + "\n" + GameDatabase.loadedModsInfo[i];
		}
	}
}
