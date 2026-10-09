using System;
using System.Collections.Generic;
using KSP.Localization;
using KSP.UI;
using KSP.UI.Screens;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace Expansions.Serenity
{
	public class RoboticControllerWindow : MonoBehaviour
	{
		private enum presetButtonMode
		{
			None,
			Sine,
			Square,
			Triangle,
			Saw,
			RevSaw,
			Flat
		}

		public TMP_InputField nameInput;

		public TMP_InputField lengthInput;

		[SerializeField]
		private Button playButton;

		private UIStateImage playButtonImageState;

		[SerializeField]
		private Button directionButton;

		private UIStateImage directionButtonImageState;

		[SerializeField]
		private Button loopButton;

		private UIStateImage loopButtonImageState;

		private UIHoverText loopButtonTextHover;

		[SerializeField]
		private Button resizeLengthModeButton;

		private UIStateImage resizeLengthButtonImageState;

		private UIHoverText resizeLengthModeTextHover;

		[SerializeField]
		private Button closeButton;

		[SerializeField]
		private Button skipStartButton;

		[SerializeField]
		private Button skipPrevButton;

		[SerializeField]
		private Button skipNextButton;

		[SerializeField]
		private Button skipEndButton;

		[SerializeField]
		private Button openActionGroupsButton;

		[SerializeField]
		private RectTransform openActionGroupsRow;

		[SerializeField]
		private Transform pointInfoFooter;

		[SerializeField]
		private bool showPointInfo;

		[SerializeField]
		private TextMeshProUGUI pointInfoIndexLabel;

		[SerializeField]
		private TMP_InputField pointInfoTimeField;

		[SerializeField]
		private TMP_InputField pointInfoValueField;

		[SerializeField]
		private TextMeshProUGUI pointInfoValueLabel;

		[SerializeField]
		private RectTransform pointValuesHolder;

		[SerializeField]
		private RectTransform pointMultiHolder;

		[SerializeField]
		private TextMeshProUGUI pointMultiLabel;

		[SerializeField]
		private Button axisUp;

		[SerializeField]
		private Button axisDown;

		[SerializeField]
		private Button axisCopy;

		[SerializeField]
		private Button axisPaste;

		[SerializeField]
		private Button axisFlipHorizontal;

		[SerializeField]
		private Button axisFlipVertical;

		[SerializeField]
		private Button axisAlignEnds;

		[SerializeField]
		private Button axisSelectAll;

		[SerializeField]
		private Button axisClampValues;

		[SerializeField]
		private RectTransform presetPanel;

		[SerializeField]
		private Button axisPreset;

		[SerializeField]
		private ToggleGroup presetsGroup;

		[SerializeField]
		private Toggle axisPresetFlat;

		[SerializeField]
		private Toggle axisPresetSine;

		[SerializeField]
		private Toggle axisPresetSquare;

		[SerializeField]
		private Toggle axisPresetTriangle;

		[SerializeField]
		private Toggle axisPresetSaw;

		[SerializeField]
		private Toggle axisPresetRevSaw;

		[SerializeField]
		private Slider axisPresetCycles;

		[SerializeField]
		private Slider axisPresetPhase;

		[SerializeField]
		private TextMeshProUGUI axisPresetCyclesLabel;

		[SerializeField]
		private TextMeshProUGUI axisPresetPhaseLabel;

		[SerializeField]
		private Button pointAdd;

		[SerializeField]
		private Button pointDelete;

		[SerializeField]
		private Button pointValueSharp;

		[SerializeField]
		private Button pointValueSmooth;

		[SerializeField]
		private Button pointClampValue;

		[SerializeField]
		private Slider sequencePlaySpeed;

		[SerializeField]
		private TextMeshProUGUI sequencePlaySpeedLabel;

		[SerializeField]
		private TextMeshProUGUI statusHelpLabel;

		public Transform axesParent;

		public Transform sequencePlayPositionRow;

		[SerializeField]
		private Slider sequencePlayPosition;

		[SerializeField]
		private RectTransform sequencePlayLine;

		[SerializeField]
		private float sequencePlayLineOffsetFlight = 10f;

		[SerializeField]
		private float sequencePlayLineOffsetEditor = 45f;

		[SerializeField]
		private UIWindow uiWindow;

		private List<RoboticControllerWindowBaseRow> rowControls;

		private Vector2 sequencePlayLineDimensions;

		private CurvePanelPoint selectedPoint;

		private List<CurvePanelPoint> selectedPoints;

		private bool sliderChanging;

		private bool speedSliderChanging;

		private float newLength;

		private bool resizeLengthMaintainTimes;

		private float tempPointInfoTime;

		private float tempPointInfoValue;

		private presetButtonMode selectedPreset;

		public TextMeshProUGUI StatusHelpLabel => statusHelpLabel;

		public bool IsRowExpanded => ExpandedRow != null;

		public RoboticControllerWindowBaseRow ExpandedRow { get; private set; }

		[Obsolete("Use ExpandedRow.IsAxis/IsAction - this will be removed in a future release")]
		public bool IsAxisExpanded => ExpandedAxis != null;

		[Obsolete("Use ExpandedRow - this will be removed in a future release")]
		public RoboticControllerWindowAxis ExpandedAxis => ExpandedRow as RoboticControllerWindowAxis;

		public bool Ready { get; private set; }

		public ModuleRoboticController Controller { get; private set; }

		public void Awake()
		{
			if (!ExpansionsLoader.IsExpansionInstalled("Serenity"))
			{
				UnityEngine.Object.Destroy(base.gameObject);
			}
		}

		public void Start()
		{
			if (!(Controller == null))
			{
				playButtonImageState = playButton.GetComponent<UIStateImage>();
				directionButtonImageState = directionButton.GetComponent<UIStateImage>();
				loopButtonImageState = loopButton.GetComponent<UIStateImage>();
				resizeLengthButtonImageState = resizeLengthModeButton.GetComponent<UIStateImage>();
				pointInfoFooter.gameObject.SetActive(showPointInfo);
				closeButton.onClick.AddListener(OnCloseClick);
				playButton.onClick.AddListener(OnPlayPauseClick);
				directionButton.onClick.AddListener(OnDirectionClick);
				loopButton.onClick.AddListener(OnLoopClick);
				resizeLengthModeButton.onClick.AddListener(OnResizeLengthModeClick);
				skipStartButton.onClick.AddListener(OnSkipStartClick);
				skipPrevButton.onClick.AddListener(OnSkipPrevClick);
				skipNextButton.onClick.AddListener(OnSkipNextClick);
				skipEndButton.onClick.AddListener(OnSkipEndClick);
				openActionGroupsButton.onClick.AddListener(OnOpenActionGroupsClick);
				nameInput.onEndEdit.AddListener(OnNameEdited);
				nameInput.onSelect.AddListener(AddInputFieldLock);
				lengthInput.onEndEdit.AddListener(OnLengthEdited);
				lengthInput.onSelect.AddListener(AddInputFieldLock);
				axisUp.onClick.AddListener(OnAxisUpClick);
				axisDown.onClick.AddListener(OnAxisDownClick);
				axisCopy.onClick.AddListener(OnAxisCopyClick);
				axisPaste.onClick.AddListener(OnAxisPasteClick);
				axisFlipHorizontal.onClick.AddListener(OnAxisFlipHorizontalClick);
				axisFlipVertical.onClick.AddListener(OnAxisFlipVerticalClick);
				axisAlignEnds.onClick.AddListener(OnAxisAlignEndsClick);
				axisClampValues.onClick.AddListener(OnAxisClampValuesClick);
				axisSelectAll.onClick.AddListener(OnAxisSelectAllClick);
				axisPreset.onClick.AddListener(OnAxisPresetClick);
				axisPresetFlat.onValueChanged.AddListener(OnAxisPresetFlatClick);
				axisPresetSine.onValueChanged.AddListener(OnAxisPresetSineClick);
				axisPresetSquare.onValueChanged.AddListener(OnAxisPresetSquareClick);
				axisPresetTriangle.onValueChanged.AddListener(OnAxisPresetTriangleClick);
				axisPresetSaw.onValueChanged.AddListener(OnAxisPresetSawClick);
				axisPresetRevSaw.onValueChanged.AddListener(OnAxisPresetRevSawClick);
				axisPresetCycles.onValueChanged.AddListener(OnPresetCyclesSliderChanged);
				axisPresetPhase.onValueChanged.AddListener(OnPresetPhaseSliderChanged);
				presetPanel.gameObject.SetActive(value: false);
				pointInfoTimeField.onEndEdit.AddListener(OnPointInfoEdited);
				pointInfoTimeField.onSelect.AddListener(AddInputFieldLock);
				pointInfoValueField.onEndEdit.AddListener(OnPointInfoEdited);
				pointInfoValueField.onSelect.AddListener(AddInputFieldLock);
				pointAdd.onClick.AddListener(OnPointAddClick);
				pointDelete.onClick.AddListener(OnPointDeleteClick);
				pointValueSharp.onClick.AddListener(OnPointValueSharpClick);
				pointValueSmooth.onClick.AddListener(OnPointValueSmoothClick);
				pointClampValue.onClick.AddListener(OnPointClampValuesClick);
				sequencePlaySpeed.onValueChanged.AddListener(OnPlaySpeedSliderChanged);
				sequencePlayPosition.onValueChanged.AddListener(OnPositionSliderChanged);
				loopButtonTextHover = loopButton.GetComponent<UIHoverText>();
				if (loopButtonTextHover != null)
				{
					loopButtonTextHover.hoverText = Localizer.Format("#autoLOC_8003240");
				}
				resizeLengthModeTextHover = resizeLengthModeButton.GetComponent<UIHoverText>();
				if (resizeLengthModeTextHover != null)
				{
					resizeLengthModeTextHover.hoverText = Localizer.Format("#autoLOC_8003327");
				}
				selectedPoints = new List<CurvePanelPoint>();
				UIWindow uIWindow = uiWindow;
				uIWindow.OnWindowResize = (Callback<RectTransform>)Delegate.Combine(uIWindow.OnWindowResize, new Callback<RectTransform>(OnWindowResize));
				UIWindow uIWindow2 = uiWindow;
				uIWindow2.OnWindowMove = (Callback<RectTransform>)Delegate.Combine(uIWindow2.OnWindowMove, new Callback<RectTransform>(OnWindowMove));
				GameEvents.onEditorStarted.Add(OnEditorStarted);
				GameEvents.onGameSceneLoadRequested.Add(OnSceneLoadRequested);
				GameEvents.onRoboticControllerAxesChanged.Add(OnControllerItemsChanged);
				GameEvents.onRoboticControllerActionsChanged.Add(OnControllerItemsChanged);
				GameEvents.onVesselChange.Add(OnVesselChanged);
				Vector3 localPosition = base.transform.localPosition;
				localPosition.z = 3f;
				base.transform.localPosition = localPosition;
				Ready = true;
				UpdateUIFromController();
			}
		}

		public void OnDestroy()
		{
			closeButton.onClick.RemoveListener(OnCloseClick);
			playButton.onClick.RemoveListener(OnPlayPauseClick);
			directionButton.onClick.RemoveListener(OnDirectionClick);
			loopButton.onClick.RemoveListener(OnLoopClick);
			resizeLengthModeButton.onClick.RemoveListener(OnResizeLengthModeClick);
			skipStartButton.onClick.RemoveListener(OnSkipStartClick);
			skipPrevButton.onClick.RemoveListener(OnSkipPrevClick);
			skipNextButton.onClick.RemoveListener(OnSkipNextClick);
			skipEndButton.onClick.RemoveListener(OnSkipEndClick);
			openActionGroupsButton.onClick.RemoveListener(OnOpenActionGroupsClick);
			nameInput.onEndEdit.RemoveListener(OnNameEdited);
			nameInput.onSelect.RemoveListener(AddInputFieldLock);
			lengthInput.onEndEdit.RemoveListener(OnLengthEdited);
			lengthInput.onSelect.RemoveListener(AddInputFieldLock);
			axisUp.onClick.RemoveListener(OnAxisUpClick);
			axisDown.onClick.RemoveListener(OnAxisDownClick);
			axisCopy.onClick.RemoveListener(OnAxisCopyClick);
			axisPaste.onClick.RemoveListener(OnAxisPasteClick);
			axisFlipHorizontal.onClick.RemoveListener(OnAxisFlipHorizontalClick);
			axisFlipVertical.onClick.RemoveListener(OnAxisFlipVerticalClick);
			axisAlignEnds.onClick.RemoveListener(OnAxisAlignEndsClick);
			axisClampValues.onClick.RemoveListener(OnAxisClampValuesClick);
			axisSelectAll.onClick.RemoveListener(OnAxisSelectAllClick);
			axisPreset.onClick.RemoveListener(OnAxisPresetClick);
			axisPresetFlat.onValueChanged.RemoveListener(OnAxisPresetFlatClick);
			axisPresetSine.onValueChanged.RemoveListener(OnAxisPresetSineClick);
			axisPresetSquare.onValueChanged.RemoveListener(OnAxisPresetSquareClick);
			axisPresetTriangle.onValueChanged.RemoveListener(OnAxisPresetTriangleClick);
			axisPresetSaw.onValueChanged.RemoveListener(OnAxisPresetSawClick);
			axisPresetRevSaw.onValueChanged.RemoveListener(OnAxisPresetRevSawClick);
			axisPresetCycles.onValueChanged.RemoveListener(OnPresetCyclesSliderChanged);
			axisPresetPhase.onValueChanged.RemoveListener(OnPresetPhaseSliderChanged);
			pointInfoTimeField.onEndEdit.RemoveListener(OnPointInfoEdited);
			pointInfoTimeField.onSelect.RemoveListener(AddInputFieldLock);
			pointInfoValueField.onEndEdit.RemoveListener(OnPointInfoEdited);
			pointInfoValueField.onSelect.RemoveListener(AddInputFieldLock);
			pointAdd.onClick.RemoveListener(OnPointAddClick);
			pointDelete.onClick.RemoveListener(OnPointDeleteClick);
			pointValueSharp.onClick.RemoveListener(OnPointValueSharpClick);
			pointValueSmooth.onClick.RemoveListener(OnPointValueSmoothClick);
			pointClampValue.onClick.RemoveListener(OnPointClampValuesClick);
			sequencePlaySpeed.onValueChanged.RemoveListener(OnPlaySpeedSliderChanged);
			sequencePlayPosition.onValueChanged.RemoveListener(OnPositionSliderChanged);
			UIWindow uIWindow = uiWindow;
			uIWindow.OnWindowResize = (Callback<RectTransform>)Delegate.Remove(uIWindow.OnWindowResize, new Callback<RectTransform>(OnWindowResize));
			UIWindow uIWindow2 = uiWindow;
			uIWindow2.OnWindowMove = (Callback<RectTransform>)Delegate.Remove(uIWindow2.OnWindowMove, new Callback<RectTransform>(OnWindowMove));
			GameEvents.onEditorStarted.Remove(OnEditorStarted);
			GameEvents.onGameSceneLoadRequested.Remove(OnSceneLoadRequested);
			GameEvents.onRoboticControllerAxesChanged.Remove(OnControllerItemsChanged);
			GameEvents.onRoboticControllerActionsChanged.Remove(OnControllerItemsChanged);
			GameEvents.onVesselChange.Remove(OnVesselChanged);
			if (Controller != null)
			{
				Controller.SetWindowSizeAndPosition(base.transform as RectTransform);
			}
			Controller = null;
		}

		public static RoboticControllerWindow Spawn(ModuleRoboticController controller)
		{
			if (!ExpansionsLoader.IsExpansionInstalled("Serenity"))
			{
				return null;
			}
			if (RoboticControllerManager.Instance == null)
			{
				Debug.LogError("[RoboticControllerWindow]: There is no RoboticControllerManager running");
				return null;
			}
			if (RoboticControllerManager.Instance.windows.ContainsKey(controller.PartPersistentId))
			{
				Debug.LogFormat("[RoboticControllerWindow]: Window is already open for {0}({1})", controller.displayName, controller.PartPersistentId);
				return null;
			}
			UnityEngine.Object @object = SerenityUtils.SerenityPrefab("_UI5/Controller/Prefabs/ControllerWindow.prefab");
			if (@object == null)
			{
				Debug.LogError("[RoboticControllerWindow]: Unable to load the Asset");
				return null;
			}
			RoboticControllerWindow component = ((GameObject)UnityEngine.Object.Instantiate(@object)).GetComponent<RoboticControllerWindow>();
			component.name = controller.displayName + " - Editor Window";
			RectTransform obj = component.transform as RectTransform;
			obj.SetParent(DialogCanvasUtil.DialogCanvasRect, worldPositionStays: false);
			obj.localPosition = new Vector3(obj.localPosition.x + (float)UnityEngine.Random.Range(-30, 30), (float)(-Screen.height / 2) / GameSettings.UI_SCALE, 0f);
			obj.localScale = Vector3.one;
			RoboticControllerManager.Instance.windows.Add(controller.PartPersistentId, component);
			component.Setup(controller);
			return component;
		}

		public void Setup(ModuleRoboticController controller)
		{
			Controller = controller;
			Controller.window = this;
			if (controller.HasWindowDimensions)
			{
				(base.transform as RectTransform).anchoredPosition = controller.WindowPosition;
				(base.transform as RectTransform).sizeDelta = Vector2.Max(uiWindow.minSize, controller.WindowSize);
			}
			uiWindow.ClampToScreen();
			sequencePlayLineDimensions = new Vector2(sequencePlayLine.sizeDelta.x, sequencePlayLine.sizeDelta.y);
			nameInput.text = controller.displayName;
			lengthInput.text = Controller.SequenceLength.ToString("F1");
			sliderChanging = true;
			sequencePlayPosition.minValue = 0f;
			sequencePlayPosition.maxValue = Controller.SequenceLength;
			sequencePlayPosition.value = Controller.SequencePosition;
			sliderChanging = false;
			RebuildAxesControls();
		}

		internal void CollapseOtherRows(uint partId, string fieldName, uint partModuleId)
		{
			for (int i = 0; i < rowControls.Count; i++)
			{
				if (rowControls[i].PartPersistentId != partId || rowControls[i].RowName != fieldName || rowControls[i].PartModulePersistentId != partModuleId)
				{
					rowControls[i].Collapse();
				}
			}
		}

		public virtual void AddInputFieldLock(string val)
		{
			InputLockManager.SetControlLock(ControlTypes.KEYBOARDINPUT, "UIRoboticControllerWindowTrackEditor");
		}

		public virtual void RemoveInputfieldLock()
		{
			InputLockManager.RemoveControlLock("UIRoboticControllerWindowTrackEditor");
		}

		internal void OnControllerChanged()
		{
			UpdateUIFromController();
		}

		internal void OnWindowAxisChanged()
		{
			UpdateUIFromController();
		}

		private void RebuildAxesControls()
		{
			if (rowControls == null)
			{
				rowControls = new List<RoboticControllerWindowBaseRow>();
			}
			else
			{
				int count = rowControls.Count;
				while (count-- > 0)
				{
					RemoveRow(rowControls[count].ControlledItem, updateUIElements: false);
				}
			}
			for (int i = 0; i < Controller.ControlledAxes.Count; i++)
			{
				AddRow(Controller.ControlledAxes[i], updateUIElements: false);
			}
			for (int j = 0; j < Controller.ControlledActions.Count; j++)
			{
				AddRow(Controller.ControlledActions[j], updateUIElements: false);
			}
			for (int k = 0; k < rowControls.Count; k++)
			{
				if (rowControls[k].rowIndex < 0)
				{
					rowControls[k].transform.SetAsLastSibling();
				}
				else
				{
					rowControls[k].transform.SetSiblingIndex(rowControls[k].rowIndex);
				}
			}
			openActionGroupsRow.SetAsLastSibling();
			sequencePlayPositionRow.SetAsLastSibling();
			SaveRowIndexes();
			if (rowControls.Count < 1)
			{
				UpdateSequencePlayLineDimensions();
			}
		}

		internal void SaveRowIndexes()
		{
			for (int i = 0; i < rowControls.Count; i++)
			{
				rowControls[i].rowIndex = rowControls[i].transform.GetSiblingIndex();
				rowControls[i].ControlledItem.rowIndex = rowControls[i].rowIndex;
			}
		}

		private void AddRow(ControlledAxis axis, bool updateUIElements)
		{
			RoboticControllerWindowAxis newBaseRow = RoboticControllerWindowAxis.Spawn(this, axis, axesParent);
			AddRow(newBaseRow, updateUIElements);
		}

		private void AddRow(ControlledAction action, bool updateUIElements)
		{
			RoboticControllerWindowAction newBaseRow = RoboticControllerWindowAction.Spawn(this, action, axesParent);
			AddRow(newBaseRow, updateUIElements);
		}

		private void AddRow(RoboticControllerWindowBaseRow newBaseRow, bool updateUIElements)
		{
			if (!(newBaseRow == null))
			{
				rowControls.Add(newBaseRow);
				openActionGroupsRow.SetAsLastSibling();
				sequencePlayPositionRow.SetAsLastSibling();
				if (updateUIElements)
				{
					UpdateUIFromController();
				}
			}
		}

		internal void RemoveRow(ControlledBase baseRow, bool updateUIElements)
		{
			int count = rowControls.Count;
			do
			{
				if (count-- <= 0)
				{
					return;
				}
			}
			while (rowControls[count].PartPersistentId != baseRow.PartPersistentId || !(rowControls[count].RowName == baseRow.BaseName) || rowControls[count].PartModulePersistentId != baseRow.moduleId);
			UnityEngine.Object.Destroy(rowControls[count].gameObject);
			rowControls.RemoveAt(count);
			if (updateUIElements)
			{
				UpdateUIFromController();
			}
		}

		private void UpdateUIFromController()
		{
			if (Ready)
			{
				nameInput.text = Controller.displayName;
				lengthInput.text = Controller.SequenceLength.ToString("F1");
				sliderChanging = true;
				sequencePlayPosition.minValue = 0f;
				sequencePlayPosition.maxValue = Controller.SequenceLength;
				sequencePlayPosition.value = Controller.SequencePosition;
				sliderChanging = false;
				sequencePlayPosition.interactable = !Controller.SequenceIsPlaying;
				lengthInput.interactable = !Controller.SequenceIsPlaying;
				sequencePlaySpeed.value = Controller.SequencePlaySpeed;
				sequencePlaySpeedLabel.text = Controller.SequencePlaySpeed.ToString("F0");
				playButtonImageState.SetState(Controller.SequenceIsPlaying ? "Playing" : "Paused");
				directionButtonImageState.SetState(Controller.SequenceDirection.ToString());
				loopButtonImageState.SetState(Controller.SequenceLoop.ToString());
				UpdateResizeLengthStateImage();
				UpdateSequencePlayLineDimensions();
				if (showPointInfo)
				{
					UpdatePointInfo();
				}
				UpdateExpandedWindowAxis();
				pointAdd.interactable = IsRowExpanded;
				pointDelete.interactable = IsRowExpanded;
				pointValueSmooth.interactable = IsRowExpanded && ExpandedRow.IsAxis;
				pointValueSharp.interactable = IsRowExpanded && ExpandedRow.IsAxis;
				pointClampValue.interactable = IsRowExpanded && ExpandedRow.IsAxis;
				axisUp.interactable = IsRowExpanded;
				axisDown.interactable = IsRowExpanded;
				axisCopy.interactable = IsRowExpanded;
				axisPaste.interactable = IsRowExpanded && ExpandedRow.rowType == RoboticControllerManager.copyCacheType;
				axisFlipHorizontal.interactable = IsRowExpanded;
				axisFlipVertical.interactable = IsRowExpanded && ExpandedRow.IsAxis;
				axisAlignEnds.interactable = IsRowExpanded && ExpandedRow.IsAxis;
				axisClampValues.interactable = IsRowExpanded && ExpandedRow.IsAxis;
				axisSelectAll.interactable = IsRowExpanded;
				axisPreset.interactable = IsRowExpanded && ExpandedRow.IsAxis;
				if (ExpandedRow == null && presetPanel.gameObject.activeSelf)
				{
					presetPanel.gameObject.SetActive(value: false);
				}
			}
		}

		private void UpdateSequencePlayLineDimensions()
		{
			if (rowControls.Count > 0)
			{
				sequencePlayLineDimensions.y = (axesParent as RectTransform).sizeDelta.y - (HighLogic.LoadedSceneIsEditor ? sequencePlayLineOffsetEditor : sequencePlayLineOffsetFlight);
			}
			else
			{
				sequencePlayLineDimensions.y = 0f;
			}
			sequencePlayLine.sizeDelta = sequencePlayLineDimensions;
		}

		private void OnControllerItemsChanged(ModuleRoboticController controller)
		{
			if (controller.PartPersistentId == Controller.PartPersistentId)
			{
				RebuildAxesControls();
				UpdateUIFromController();
			}
		}

		private void UpdatePointInfo()
		{
			bool flag = selectedPoints.Count < 2;
			pointValuesHolder.gameObject.SetActive(flag);
			pointMultiHolder.gameObject.SetActive(!flag);
			if (flag)
			{
				pointInfoTimeField.interactable = selectedPoints.Count == 1 && selectedPoint != null;
				pointInfoValueField.interactable = selectedPoints.Count == 1 && selectedPoint != null;
			}
			if (selectedPoints.Count == 1 && selectedPoint != null)
			{
				pointInfoIndexLabel.text = selectedPoint.Panel.SelectedPointIndexes[0].ToString();
				pointInfoTimeField.text = (selectedPoint.Keyframe.time / selectedPoint.Panel.PanelTimeDuration * Controller.SequenceLength).ToString("0.00");
				pointInfoValueField.text = selectedPoint.Keyframe.value.ToString("0.00");
				pointInfoValueLabel.gameObject.SetActive(!selectedPoint.noValue);
				pointInfoValueField.gameObject.SetActive(!selectedPoint.noValue);
			}
			else if (selectedPoints.Count > 1)
			{
				pointMultiLabel.text = Localizer.Format("#autoLOC_8003330", selectedPoints.Count);
			}
			else
			{
				pointInfoIndexLabel.text = "";
				pointInfoTimeField.text = "";
				pointInfoValueField.text = "";
				pointInfoValueLabel.gameObject.SetActive(value: true);
				pointInfoValueField.gameObject.SetActive(value: true);
			}
		}

		private void UpdateExpandedWindowAxis()
		{
			int num = 0;
			while (true)
			{
				if (num < rowControls.Count)
				{
					if (rowControls[num].Expanded)
					{
						break;
					}
					num++;
					continue;
				}
				ExpandedRow = null;
				return;
			}
			ExpandedRow = rowControls[num];
		}

		internal void OnPointSelected(RoboticControllerWindowBaseRow row, List<CurvePanelPoint> points)
		{
			selectedPoints = points;
			if (points.Count < 1)
			{
				selectedPoint = null;
			}
			else if (points.Count == 1)
			{
				selectedPoint = points[0];
			}
			else
			{
				selectedPoint = null;
			}
			if (showPointInfo)
			{
				UpdatePointInfo();
			}
			UpdateUIFromController();
		}

		internal void OnPointDragging(RoboticControllerWindowBaseRow row, List<CurvePanelPoint> points)
		{
			ClearSelectedPresetMode();
			selectedPoints = points;
			if (points.Count < 1)
			{
				selectedPoint = null;
			}
			else if (points.Count == 1)
			{
				selectedPoint = points[0];
			}
			else
			{
				selectedPoint = null;
			}
			if (showPointInfo)
			{
				UpdatePointInfo();
			}
			if (HighLogic.LoadedSceneIsEditor && !Controller.SequenceIsPlaying)
			{
				Controller.SetSequencePosition(Controller.SequencePosition);
			}
		}

		private void ClearSelectedPresetMode()
		{
			selectedPreset = presetButtonMode.None;
			presetsGroup.SetAllTogglesOff();
		}

		private void OnPositionSliderChanged(float newValue)
		{
			if (Ready && !sliderChanging)
			{
				sliderChanging = true;
				Controller.SetSequencePosition(newValue);
				sliderChanging = false;
			}
		}

		private void OnPlaySpeedSliderChanged(float newValue)
		{
			if (Ready && !speedSliderChanging)
			{
				speedSliderChanging = true;
				Controller.SetPlaySpeed(newValue);
				speedSliderChanging = false;
			}
		}

		private void OnCloseClick()
		{
			CloseWindow();
		}

		private void OnEditorStarted()
		{
			CloseWindow();
		}

		private void OnSceneLoadRequested(GameScenes scene)
		{
			CloseWindow();
		}

		internal void CloseWindow()
		{
			if (RoboticControllerManager.Instance != null && RoboticControllerManager.Instance.windows.ContainsKey(Controller.PartPersistentId))
			{
				RoboticControllerManager.Instance.windows.Remove(Controller.PartPersistentId);
			}
			if (Controller != null)
			{
				Controller.window = null;
			}
			UnityEngine.Object.Destroy(base.gameObject);
		}

		internal void OnVesselChanged(Vessel vessel)
		{
			if (Controller != null && Controller.part.vessel.id != vessel.id)
			{
				CloseWindow();
			}
		}

		private void OnWindowMove(RectTransform windowRect)
		{
			if (Controller != null)
			{
				Controller.SetWindowSizeAndPosition(windowRect);
			}
		}

		private void OnPlayPauseClick()
		{
			Controller.TogglePlay();
		}

		private void OnDirectionClick()
		{
			Controller.ToggleDirection();
		}

		private void OnLoopClick()
		{
			Controller.CycleLoopMode();
			switch (Controller.SequenceLoop)
			{
			default:
				loopButtonTextHover.hoverText = Localizer.Format("#autoLOC_8003240");
				loopButtonTextHover.UpdateText();
				break;
			case ModuleRoboticController.SequenceLoopOptions.Repeat:
				loopButtonTextHover.hoverText = Localizer.Format("#autoLOC_8003241");
				loopButtonTextHover.UpdateText();
				break;
			case ModuleRoboticController.SequenceLoopOptions.PingPong:
				loopButtonTextHover.hoverText = Localizer.Format("#autoLOC_8003242");
				loopButtonTextHover.UpdateText();
				break;
			case ModuleRoboticController.SequenceLoopOptions.OnceRestart:
				loopButtonTextHover.hoverText = Localizer.Format("#autoLOC_8003619");
				loopButtonTextHover.UpdateText();
				break;
			}
		}

		private void OnSkipStartClick()
		{
			Controller.SetSequencePositionStart();
			SelectPointAtTime();
		}

		private void OnSkipEndClick()
		{
			Controller.SetSequencePositionEnd();
			SelectPointAtTime();
		}

		private void OnSkipPrevClick()
		{
			Controller.SetSequencePositionPrevKey();
			SelectPointAtTime();
		}

		private void OnSkipNextClick()
		{
			Controller.SetSequencePositionNextKey();
			SelectPointAtTime();
		}

		private void SelectPointAtTime()
		{
			if (IsRowExpanded)
			{
				ExpandedRow.SelectPointAtTime(Controller.SequencePosition);
			}
		}

		private void OnNameEdited(string newValue)
		{
			Controller.SetDisplayName(newValue);
			if (HighLogic.LoadedSceneIsEditor && EditorLogic.fetch.editorScreen == EditorScreen.Actions)
			{
				EditorActionGroups.Instance.RebuildLists(fullRebuild: true, keepSelection: true);
			}
			RemoveInputfieldLock();
		}

		private void OnLengthEdited(string newValue)
		{
			if (float.TryParse(newValue, out newLength))
			{
				Controller.SetLength(newLength, resizeLengthMaintainTimes);
				if (resizeLengthMaintainTimes)
				{
					RebuildAxesControls();
				}
			}
			RemoveInputfieldLock();
		}

		private void OnResizeLengthModeClick()
		{
			resizeLengthMaintainTimes = !resizeLengthMaintainTimes;
			UpdateResizeLengthStateImage();
		}

		private void UpdateResizeLengthStateImage()
		{
			if (resizeLengthMaintainTimes)
			{
				resizeLengthButtonImageState.SetState("Maintain");
				resizeLengthModeTextHover.hoverText = Localizer.Format("#autoLOC_8003328");
				resizeLengthModeTextHover.UpdateText();
			}
			else
			{
				resizeLengthButtonImageState.SetState("Scale");
				resizeLengthModeTextHover.hoverText = Localizer.Format("#autoLOC_8003327");
				resizeLengthModeTextHover.UpdateText();
			}
		}

		private void OnPointInfoEdited(string newValue)
		{
			if (float.TryParse(pointInfoTimeField.text, out tempPointInfoTime) && float.TryParse(pointInfoValueField.text, out tempPointInfoValue))
			{
				ClearSelectedPresetMode();
				selectedPoint.Panel.MovePoint(selectedPoint, tempPointInfoTime / Controller.SequenceLength * selectedPoint.Panel.PanelTimeDuration, tempPointInfoValue);
				if (showPointInfo)
				{
					UpdatePointInfo();
				}
			}
			RemoveInputfieldLock();
		}

		private void OnWindowResize(RectTransform rect)
		{
			int count = rowControls.Count;
			while (count-- > 0)
			{
				rowControls[count].RedrawCurve();
			}
			if (Controller != null)
			{
				Controller.SetWindowSizeAndPosition(rect);
			}
		}

		private void OnAxisUpClick()
		{
			if (IsRowExpanded)
			{
				int siblingIndex = ExpandedRow.transform.GetSiblingIndex();
				if (siblingIndex > 0)
				{
					ExpandedRow.transform.SetSiblingIndex(siblingIndex - 1);
				}
				SaveRowIndexes();
			}
		}

		private void OnAxisDownClick()
		{
			if (IsRowExpanded)
			{
				int siblingIndex = ExpandedRow.transform.GetSiblingIndex();
				if (siblingIndex < Controller.ControlledActions.Count + Controller.ControlledAxes.Count - 1)
				{
					ExpandedRow.transform.SetSiblingIndex(siblingIndex + 1);
				}
				SaveRowIndexes();
			}
		}

		private void OnAxisCopyClick()
		{
			if (IsRowExpanded)
			{
				if (ExpandedRow.IsAxis)
				{
					RoboticControllerManager.copyCacheType = RoboticControllerWindowBaseRow.rowTypes.Axis;
					RoboticControllerManager.copyCacheAxis = new FloatCurve((ExpandedRow as RoboticControllerWindowAxis).Axis.timeValue.Curve.keys);
				}
				else if (ExpandedRow.IsAction)
				{
					RoboticControllerManager.copyCacheType = RoboticControllerWindowBaseRow.rowTypes.Action;
					RoboticControllerManager.copyCacheAction = new List<float>((ExpandedRow as RoboticControllerWindowAction).Action.times);
				}
				axisPaste.interactable = IsRowExpanded && ExpandedRow.rowType == RoboticControllerManager.copyCacheType;
			}
		}

		private void OnAxisPasteClick()
		{
			if (IsRowExpanded)
			{
				if (ExpandedRow.IsAxis && RoboticControllerManager.copyCacheType == RoboticControllerWindowBaseRow.rowTypes.Axis)
				{
					(ExpandedRow as RoboticControllerWindowAxis).Axis.timeValue = new FloatCurve(RoboticControllerManager.copyCacheAxis.Curve.keys);
					ExpandedRow.ReloadCurve();
				}
				else if (ExpandedRow.IsAction && RoboticControllerManager.copyCacheType == RoboticControllerWindowBaseRow.rowTypes.Action)
				{
					(ExpandedRow as RoboticControllerWindowAction).Action.times = new List<float>(RoboticControllerManager.copyCacheAction);
					ExpandedRow.ReloadCurve();
				}
			}
		}

		private void OnAxisFlipHorizontalClick()
		{
			if (IsRowExpanded)
			{
				ExpandedRow.ReverseCurve();
			}
		}

		private void OnAxisFlipVerticalClick()
		{
			if (IsRowExpanded && ExpandedRow.IsAxis)
			{
				(ExpandedRow as RoboticControllerWindowAxis).InvertCurve();
			}
		}

		private void OnAxisAlignEndsClick()
		{
			if (IsRowExpanded && ExpandedRow.IsAxis)
			{
				(ExpandedRow as RoboticControllerWindowAxis).AlignCurveEnds();
			}
		}

		private void OnAxisClampValuesClick()
		{
			if (IsRowExpanded && ExpandedRow.IsAxis)
			{
				(ExpandedRow as RoboticControllerWindowAxis).ClampAllPointValues();
			}
		}

		private void OnPointClampValuesClick()
		{
			if (IsRowExpanded && ExpandedRow.IsAxis)
			{
				(ExpandedRow as RoboticControllerWindowAxis).ClampPointValues();
			}
		}

		private void OnAxisSelectAllClick()
		{
			if (IsRowExpanded)
			{
				ExpandedRow.SelectAllPoints();
			}
		}

		private void OnAxisPresetClick()
		{
			presetPanel.gameObject.SetActive(!presetPanel.gameObject.activeSelf);
			if (IsRowExpanded && ExpandedRow.IsAxis)
			{
				ClearSelectedPresetMode();
				(ExpandedRow as RoboticControllerWindowAxis).ClearPresetRefs();
			}
		}

		private void PresetToggleChanged(bool isOn, presetButtonMode mode, float cycles, float phase)
		{
			selectedPreset = mode;
			if (IsRowExpanded && ExpandedRow.IsAxis && isOn)
			{
				switch (selectedPreset)
				{
				case presetButtonMode.Sine:
					(ExpandedRow as RoboticControllerWindowAxis).PresetSine(axisPresetCycles.value, axisPresetPhase.value);
					break;
				case presetButtonMode.Square:
					(ExpandedRow as RoboticControllerWindowAxis).PresetSquare(axisPresetCycles.value, axisPresetPhase.value);
					break;
				case presetButtonMode.Triangle:
					(ExpandedRow as RoboticControllerWindowAxis).PresetTriangle(axisPresetCycles.value, axisPresetPhase.value);
					break;
				case presetButtonMode.Saw:
					(ExpandedRow as RoboticControllerWindowAxis).PresetSaw(axisPresetCycles.value, axisPresetPhase.value);
					break;
				case presetButtonMode.RevSaw:
					(ExpandedRow as RoboticControllerWindowAxis).PresetRevSaw(axisPresetCycles.value, axisPresetPhase.value);
					break;
				default:
					(ExpandedRow as RoboticControllerWindowAxis).PresetFlat();
					break;
				case presetButtonMode.None:
					break;
				}
			}
			else if (!presetsGroup.AnyTogglesOn())
			{
				selectedPreset = presetButtonMode.None;
			}
		}

		private void OnAxisPresetFlatClick(bool isOn)
		{
			PresetToggleChanged(isOn, presetButtonMode.Flat, 0f, 0f);
		}

		private void OnAxisPresetSineClick(bool isOn)
		{
			PresetToggleChanged(isOn, presetButtonMode.Sine, axisPresetCycles.value, axisPresetPhase.value);
		}

		private void OnAxisPresetSquareClick(bool isOn)
		{
			PresetToggleChanged(isOn, presetButtonMode.Square, axisPresetCycles.value, axisPresetPhase.value);
		}

		private void OnAxisPresetTriangleClick(bool isOn)
		{
			PresetToggleChanged(isOn, presetButtonMode.Triangle, axisPresetCycles.value, axisPresetPhase.value);
		}

		private void OnAxisPresetSawClick(bool isOn)
		{
			PresetToggleChanged(isOn, presetButtonMode.Saw, axisPresetCycles.value, axisPresetPhase.value);
		}

		private void OnAxisPresetRevSawClick(bool isOn)
		{
			PresetToggleChanged(isOn, presetButtonMode.RevSaw, axisPresetCycles.value, axisPresetPhase.value);
		}

		private void OnPresetCyclesSliderChanged(float newValue)
		{
			axisPresetCyclesLabel.text = newValue.ToString("F1");
			if (selectedPreset != 0 && IsRowExpanded && ExpandedRow.IsAxis)
			{
				(ExpandedRow as RoboticControllerWindowAxis).UpdatePreset(axisPresetCycles.value, axisPresetPhase.value);
			}
		}

		private void OnPresetPhaseSliderChanged(float newValue)
		{
			axisPresetPhaseLabel.text = newValue.ToString("F0");
			if (selectedPreset != 0 && IsRowExpanded && ExpandedRow.IsAxis)
			{
				(ExpandedRow as RoboticControllerWindowAxis).UpdatePreset(axisPresetCycles.value, axisPresetPhase.value);
			}
		}

		private void OnPointAddClick()
		{
			if (rowControls != null && rowControls.Count >= 1)
			{
				float timeValue = Mathf.Clamp(Controller.SequencePosition, 0.01f, Controller.SequenceLength - 0.01f);
				if (IsRowExpanded)
				{
					ExpandedRow.InsertPoint(timeValue);
				}
			}
		}

		private void OnPointDeleteClick()
		{
			if (selectedPoints.Count > 0)
			{
				selectedPoints[0].Panel.DeleteSelectedPoints();
			}
		}

		private void OnPointValueSharpClick()
		{
			int count = selectedPoints.Count;
			while (count-- > 0)
			{
				selectedPoints[count].Panel.SetTangentSharp(selectedPoints[count]);
			}
		}

		private void OnPointValueSmoothClick()
		{
			int count = selectedPoints.Count;
			while (count-- > 0)
			{
				selectedPoints[count].Panel.SetTangentSmooth(selectedPoints[count]);
			}
		}

		private void OnOpenActionGroupsClick()
		{
			if (HighLogic.LoadedSceneIsEditor && EditorLogic.fetch != null)
			{
				EditorLogic.fetch.SelectPanelActions();
				EditorActionGroups.Instance.SelectController(Controller);
			}
			if (HighLogic.LoadedSceneIsFlight && ActionGroupsFlightController.Instance != null)
			{
				ActionGroupsFlightController.Instance.SelectPanelActions();
				EditorActionGroups.Instance.SelectController(Controller);
			}
		}

		public bool AnyTextFieldHasFocus()
		{
			int num = 0;
			while (true)
			{
				if (num < rowControls.Count)
				{
					if (rowControls[num].AnyTextFieldHasFocus())
					{
						break;
					}
					num++;
					continue;
				}
				if (!nameInput.isFocused && !lengthInput.isFocused && !pointInfoTimeField.isFocused)
				{
					return pointInfoValueField.isFocused;
				}
				return true;
			}
			return true;
		}
	}
}
