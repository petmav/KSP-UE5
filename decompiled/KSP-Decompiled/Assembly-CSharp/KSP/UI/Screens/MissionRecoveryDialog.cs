using System;
using System.Collections.Generic;
using KSP.Localization;
using KSP.UI.Screens.SpaceCenter.MissionSummaryDialog;
using KSP.UI.Util;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	public class MissionRecoveryDialog : MonoBehaviour
	{
		public enum Mode
		{
			None,
			Science,
			Parts,
			Crew
		}

		[SerializeField]
		private TextMeshProUGUI header;

		[SerializeField]
		private ToggleGroup tabGroup;

		[SerializeField]
		private Toggle toggleScience;

		[SerializeField]
		private Toggle toggleParts;

		[SerializeField]
		private Toggle toggleCrew;

		[SerializeField]
		private TextMeshProUGUI tabHeaderField;

		[SerializeField]
		private Button ButtonProceed;

		[SerializeField]
		private TextMeshProUGUI ButtonProceedCaption;

		[SerializeField]
		private RectTransform wContainerScience;

		[SerializeField]
		private RectTransform wContainerParts;

		[SerializeField]
		private RectTransform wContainerSpacer;

		[SerializeField]
		private RectTransform wContainerResources;

		[SerializeField]
		private RectTransform wContainerCrew;

		public Sprite dataIcon;

		public Sprite scienceIcon;

		public Sprite fundsIconGreen;

		public Sprite fundsIconRed;

		public Sprite reputationIconGreen;

		public Sprite reputationIconRed;

		private string windowTitle;

		private List<ScienceSubjectWidget> scienceWidgets;

		private List<PartWidget> partWidgets;

		private List<ResourceWidget> resourceWidgets;

		private List<CrewWidget> crewWidgets;

		[SerializeField]
		private ImgText fieldMissionEarned;

		[SerializeField]
		private ImgText fieldTotal;

		[SerializeField]
		private DragPanel dragPanel;

		public float beforeMissionScience;

		public float scienceEarned;

		public float totalScience;

		public double beforeMissionFunds;

		public double fundsEarned;

		public double totalFunds;

		public string FundsModifier = "";

		public string ScienceModifier = "";

		public string RepModifier = "";

		public string recoveryLocation = "";

		public string recoveryFactor = "";

		public float beforeMissionReputation;

		public float reputationEarned;

		public float totalReputation;

		public bool displayReputation;

		private bool MissionIsDebris;

		public bool RnDOperational;

		public Mode mode;

		public bool ScienceModeAvailable;

		public bool PartsModeAvailable;

		public bool CrewModeAvailable;

		private int partCount;

		private int rscCount;

		private string partCaption = "";

		public static MissionRecoveryDialog CreateScienceDialog(ProtoVessel pv, float beforeMissionScience, float currentScience)
		{
			MissionRecoveryDialog component = UnityEngine.Object.Instantiate(AssetBase.GetPrefab("MissionRecoveryDialog")).GetComponent<MissionRecoveryDialog>();
			component.name = pv.GetDisplayName() + " Recovery Dialog Handler";
			component.transform.SetParent(DialogCanvasUtil.DialogCanvasRect, worldPositionStays: false);
			component.transform.localPosition = Vector3.zero;
			component.windowTitle = "Science Summary for <i>" + pv.GetDisplayName() + "</i>";
			component.beforeMissionScience = beforeMissionScience;
			component.totalScience = currentScience;
			component.mode = Mode.Science;
			component.ScienceModeAvailable = true;
			return component;
		}

		public static MissionRecoveryDialog CreateFullDialog(ProtoVessel pv)
		{
			MissionRecoveryDialog component = UnityEngine.Object.Instantiate(AssetBase.GetPrefab("MissionRecoveryDialog")).GetComponent<MissionRecoveryDialog>();
			component.name = pv.GetDisplayName() + " Recovery Dialog Handler";
			component.transform.SetParent(DialogCanvasUtil.DialogCanvasRect, worldPositionStays: false);
			component.transform.localPosition = Vector3.zero;
			component.windowTitle = Localizer.Format("#autoLOC_419077", pv.GetDisplayName());
			component.ScienceModeAvailable = false;
			component.PartsModeAvailable = false;
			component.CrewModeAvailable = false;
			component.mode = Mode.None;
			component.MissionIsDebris = pv.vesselType == VesselType.Debris;
			component.displayReputation = Reputation.Instance != null;
			return component;
		}

		public void Awake()
		{
			scienceWidgets = new List<ScienceSubjectWidget>();
			partWidgets = new List<PartWidget>();
			resourceWidgets = new List<ResourceWidget>();
			crewWidgets = new List<CrewWidget>();
			GameEvents.onGUIRecoveryDialogSpawn.Fire(this);
			if (!HighLogic.LoadedSceneIsFlight)
			{
				InputLockManager.SetControlLock(ControlTypes.EDITOR_LOCK | ControlTypes.KSC_ALL | ControlTypes.TRACKINGSTATION_UI, "MissionSummaryDialog");
			}
			wContainerCrew.gameObject.SetActive(value: false);
			wContainerParts.gameObject.SetActive(value: false);
			wContainerSpacer.gameObject.SetActive(value: false);
			wContainerResources.gameObject.SetActive(value: false);
			wContainerScience.gameObject.SetActive(value: false);
			ButtonProceed.onClick.AddListener(OnProceedButtonClick);
			toggleScience.onValueChanged.AddListener(OnSciTabPress);
			toggleParts.onValueChanged.AddListener(OnPartsTabPress);
			toggleCrew.onValueChanged.AddListener(OnCrewTabPress);
			if (dragPanel != null)
			{
				if (HighLogic.LoadedSceneIsFlight)
				{
					dragPanel.enabled = true;
				}
				else
				{
					dragPanel.enabled = false;
				}
			}
		}

		public void Start()
		{
			GameEvents.onGameSceneLoadRequested.Add(OnGameSceneLoadRequested);
			if (!ScienceModeAvailable && !PartsModeAvailable && !CrewModeAvailable)
			{
				dismissDialog();
				return;
			}
			if (MissionIsUninteresting())
			{
				Debug.Log("[Mission Summary Dialog]: Skipping " + windowTitle + " as it contains only debris.");
				dismissDialog();
			}
			if (PartsModeAvailable)
			{
				partCount = 0;
				int count = partWidgets.Count;
				while (count-- > 0)
				{
					partCount += partWidgets[count].count;
				}
				rscCount = resourceWidgets.Count;
				partCaption = Localizer.Format("#autoLOC_5700000", partCount, partCount, rscCount);
				if (!string.IsNullOrEmpty(recoveryLocation))
				{
					partCaption += Localizer.Format("#autoLOC_5700001", recoveryLocation, recoveryFactor);
				}
				else
				{
					partCaption += ":";
				}
			}
			header.text = windowTitle;
			toggleScience.interactable = ScienceModeAvailable;
			toggleParts.interactable = PartsModeAvailable;
			toggleCrew.interactable = CrewModeAvailable;
			tabGroup.RegisterToggle(toggleScience);
			tabGroup.RegisterToggle(toggleParts);
			tabGroup.RegisterToggle(toggleCrew);
			if (ScienceModeAvailable)
			{
				SetMode(Mode.Science);
			}
			else if (PartsModeAvailable)
			{
				SetMode(Mode.Parts);
			}
			else if (CrewModeAvailable)
			{
				SetMode(Mode.Crew);
			}
		}

		public void Update()
		{
			if (Input.GetKeyUp(KeyCode.Escape))
			{
				dismissDialog();
			}
			else if (Input.GetKeyUp(KeyCode.Return) || Input.GetKeyUp(KeyCode.KeypadEnter))
			{
				OnProceedButtonClick();
			}
		}

		public void OnDestroy()
		{
			GameEvents.onGameSceneLoadRequested.Remove(OnGameSceneLoadRequested);
			GameEvents.onGUIRecoveryDialogDespawn.Fire(this);
			InputLockManager.RemoveControlLock("MissionSummaryDialog");
		}

		private void OnGameSceneLoadRequested(GameScenes scene)
		{
			UnityEngine.Object.Destroy(base.gameObject);
		}

		protected void OnSciTabPress(bool toggle)
		{
			if (toggle)
			{
				SetMode(Mode.Science);
			}
		}

		protected void OnPartsTabPress(bool toggle)
		{
			if (toggle)
			{
				SetMode(Mode.Parts);
			}
		}

		protected void OnCrewTabPress(bool toggle)
		{
			if (toggle)
			{
				SetMode(Mode.Crew);
			}
		}

		public bool MissionIsUninteresting()
		{
			if (PartsModeAvailable && !ScienceModeAvailable && !CrewModeAvailable)
			{
				return MissionIsDebris;
			}
			return false;
		}

		public void AddDataWidget(ScienceSubjectWidget widget)
		{
			scienceWidgets.Add(widget);
			widget.RTrf.SetParent(wContainerScience, worldPositionStays: false);
			ScienceModeAvailable = true;
		}

		public void AddPartWidget(PartWidget widget)
		{
			if (widget.partValue == 0f)
			{
				return;
			}
			if (!partWidgets.Contains(widget))
			{
				partWidgets.Add(widget);
				widget.RTrf.SetParent(wContainerParts, worldPositionStays: false);
			}
			else
			{
				partWidgets.Find((PartWidget pw) => pw.Equals(widget)).AddDuplicate(widget.resourcesValue);
			}
			PartsModeAvailable = true;
		}

		public void AddResourceWidget(ResourceWidget widget)
		{
			if (widget.unitValue == 0f)
			{
				return;
			}
			if (!resourceWidgets.Contains(widget))
			{
				resourceWidgets.Add(widget);
				widget.RTrf.SetParent(wContainerResources, worldPositionStays: false);
			}
			else
			{
				resourceWidgets.Find((ResourceWidget rw) => rw.Equals(widget)).AddAmount(widget.amount);
			}
			PartsModeAvailable = true;
		}

		public void AddCrewWidget(CrewWidget widget)
		{
			crewWidgets.Add(widget);
			widget.RTrf.SetParent(wContainerCrew, worldPositionStays: false);
			CrewModeAvailable = true;
		}

		private void dismissDialog()
		{
			UnityEngine.Object.Destroy(base.gameObject);
		}

		protected void OnProceedButtonClick()
		{
			switch (mode)
			{
			case Mode.Science:
				if (PartsModeAvailable)
				{
					SetMode(Mode.Parts);
				}
				else if (CrewModeAvailable)
				{
					SetMode(Mode.Crew);
				}
				else
				{
					dismissDialog();
				}
				break;
			case Mode.Parts:
				if (CrewModeAvailable)
				{
					SetMode(Mode.Crew);
				}
				else
				{
					dismissDialog();
				}
				break;
			default:
				dismissDialog();
				break;
			}
		}

		protected string GetProceedButtonCaption(Mode m)
		{
			if ((m == Mode.Science && (PartsModeAvailable || CrewModeAvailable)) || (m == Mode.Parts && CrewModeAvailable))
			{
				return Localizer.Format("#autoLOC_419334");
			}
			return Localizer.Format("#autoLOC_419338");
		}

		protected void SetMode(Mode m)
		{
			if (mode != m)
			{
				switch (mode)
				{
				case Mode.Science:
					wContainerScience.gameObject.SetActive(value: false);
					break;
				case Mode.Parts:
					wContainerParts.gameObject.SetActive(value: false);
					wContainerSpacer.gameObject.SetActive(value: false);
					wContainerResources.gameObject.SetActive(value: false);
					break;
				case Mode.Crew:
					wContainerCrew.gameObject.SetActive(value: false);
					break;
				}
				tabGroup.SetAllTogglesOff();
				mode = m;
				switch (m)
				{
				case Mode.Science:
					wContainerScience.gameObject.SetActive(value: true);
					updateScienceWindowContent();
					toggleScience.isOn = true;
					break;
				case Mode.Parts:
					wContainerParts.gameObject.SetActive(value: true);
					wContainerSpacer.gameObject.SetActive(value: true);
					wContainerResources.gameObject.SetActive(value: true);
					updatePartsWindowContent();
					toggleParts.isOn = true;
					break;
				case Mode.Crew:
					wContainerCrew.gameObject.SetActive(value: true);
					updateCrewWindowContent();
					toggleCrew.isOn = true;
					break;
				}
				ButtonProceedCaption.text = GetProceedButtonCaption(m);
			}
		}

		protected void updateScienceWindowContent()
		{
			tabHeaderField.text = Localizer.Format("#autoLOC_5700002", scienceWidgets.Count);
			int i = 0;
			for (int count = scienceWidgets.Count; i < count; i++)
			{
				scienceWidgets[i].UpdateFields();
			}
			fieldMissionEarned.sprite = dataIcon;
			if (RnDOperational)
			{
				fieldMissionEarned.text = Localizer.Format("#autoLOC_6001493", scienceEarned.ToString("0.0"));
			}
			else
			{
				fieldMissionEarned.text = Localizer.Format("#autoLOC_419416");
			}
			fieldTotal.sprite = scienceIcon;
			fieldTotal.text = Localizer.Format("#autoLOC_419420", Mathf.Floor(totalScience).ToString("0"));
		}

		protected void updatePartsWindowContent()
		{
			tabHeaderField.text = partCaption;
			int i = 0;
			for (int count = partWidgets.Count; i < count; i++)
			{
				partWidgets[i].UpdateFields();
			}
			int j = 0;
			for (int count2 = resourceWidgets.Count; j < count2; j++)
			{
				resourceWidgets[j].UpdateFields();
			}
			fieldMissionEarned.sprite = fundsIconRed;
			fieldMissionEarned.text = Localizer.Format("#autoLOC_419438", fundsEarned.ToString("N0"));
			fieldTotal.sprite = fundsIconGreen;
			fieldTotal.text = Localizer.Format("#autoLOC_419441", Math.Floor(totalFunds).ToString("N0"));
		}

		protected void updateCrewWindowContent()
		{
			tabHeaderField.text = Localizer.Format("#autoLOC_5700003", crewWidgets.Count);
			int i = 0;
			for (int count = crewWidgets.Count; i < count; i++)
			{
				crewWidgets[i].UpdateFields();
			}
			fieldMissionEarned.sprite = reputationIconRed;
			fieldMissionEarned.text = Localizer.Format("#autoLOC_419455", reputationEarned.ToString("0.0"));
			fieldTotal.sprite = reputationIconGreen;
			fieldTotal.text = Localizer.Format("#autoLOC_419458", totalReputation.ToString("0"));
		}
	}
}
