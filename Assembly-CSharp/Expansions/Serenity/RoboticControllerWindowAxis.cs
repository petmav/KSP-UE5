using System;
using System.Collections.Generic;
using KSP.Localization;
using KSP.UI;
using UnityEngine;
using UnityEngine.EventSystems;

namespace Expansions.Serenity
{
	public class RoboticControllerWindowAxis : RoboticControllerWindowBaseRow
	{
		public RectTransform axisLimits;

		private bool showLimitsEditorWidget = true;

		private IAxisFieldLimits fieldLimits;

		[SerializeField]
		private CurvePanel curvePanel;

		private UIHoverText curvePanelTextHover;

		[SerializeField]
		private AxisLimitLine axisMinLimit;

		private UIHoverText axisMinLimitTextHover;

		[SerializeField]
		private AxisLimitLine axisMaxLimit;

		private UIHoverText axisMaxLimitTextHover;

		[SerializeField]
		private float axisMinMaxDeadzone = 1f;

		[SerializeField]
		private float clampSpace = 0.01f;

		private bool normalized;

		private float headerWidth;

		private float axisWidth;

		private Vector2 headerSize;

		private Vector2 moveAxisCache;

		public ControlledAxis Axis { get; private set; }

		public static RoboticControllerWindowAxis Spawn(RoboticControllerWindow window, ControlledAxis axis, Transform parent)
		{
			if (!ExpansionsLoader.IsExpansionInstalled("Serenity"))
			{
				return null;
			}
			UnityEngine.Object @object = SerenityUtils.SerenityPrefab("_UI5/Controller/Prefabs/ControllerAxis.prefab");
			if (@object == null)
			{
				Debug.LogError("[RoboticControllerWindowAxis]: Unable to load the Asset");
				return null;
			}
			if (axis.Part == null)
			{
				Debug.LogWarning("[RoboticControllerWindowAxis]: Axis has no Part, not adding this axis");
				return null;
			}
			if (axis.AxisField == null)
			{
				Debug.LogWarning("[RoboticControllerWindowAxis]: Axis has no Field, not adding this axis");
				return null;
			}
			RoboticControllerWindowAxis component = ((GameObject)UnityEngine.Object.Instantiate(@object)).GetComponent<RoboticControllerWindowAxis>();
			component.name = axis.Part.partInfo.title + " - " + axis.AxisField.guiName;
			component.transform.SetParent(parent);
			component.transform.localPosition = Vector3.zero;
			component.transform.localScale = Vector3.one;
			component.Setup(window, axis);
			if (HighLogic.LoadedSceneIsFlight)
			{
				component.showLimitsEditorWidget = false;
			}
			return component;
		}

		public void Setup(RoboticControllerWindow window, ControlledAxis axis)
		{
			AssignBaseReferenceVars(window, window.Controller, axis);
			Axis = axis;
			string text = axis.PartNickName;
			ModuleRoboticController moduleRoboticController = axis.AxisField.module as ModuleRoboticController;
			if (text == "")
			{
				text = ((!(moduleRoboticController != null)) ? axis.Part.partInfo.title : moduleRoboticController.displayName);
			}
			normalized = moduleRoboticController != null;
			titleTextPart.text = text;
			titleTextField.text = axis.AxisField.guiName;
			editNickButton.interactable = true;
			editNickButton.gameObject.SetActive(value: true);
			partNickNameInput.gameObject.SetActive(value: false);
			expanded = false;
			if (axis.AxisField != null)
			{
				fieldLimits = axis.AxisField.module as IAxisFieldLimits;
				if (fieldLimits != null && fieldLimits.HasAxisFieldLimit(axis.AxisField.name))
				{
					IAxisFieldLimits axisFieldLimits = fieldLimits;
					axisFieldLimits.LimitsChanged = (Callback<AxisFieldLimit>)Delegate.Combine(axisFieldLimits.LimitsChanged, new Callback<AxisFieldLimit>(OnLimitsChanged));
				}
			}
			axisMinLimit.Setup(this, axisMinMaxDeadzone, Axis.axisMin);
			axisMaxLimit.Setup(this, axisMinMaxDeadzone, Axis.axisMax);
		}

		protected override void OnRowStart()
		{
			headerWidth = headerTransform.rect.width;
			axisWidth = axisLimits.rect.width;
			SetLayoutElements();
			if (curvePanel == null)
			{
				Debug.LogError("[RoboticControllerWindowAxis] No CurvePanel to use when drawing em");
				return;
			}
			if (normalized)
			{
				curvePanel.Setup(Axis.timeValue, "Line-" + Axis.Part.partInfo.title + "-" + Axis.AxisField.guiName, 0f, 1f);
			}
			else
			{
				curvePanel.Setup(Axis.timeValue, "Line-" + Axis.Part.partInfo.title + "-" + Axis.AxisField.guiName, Axis.axisMin, Axis.axisMax);
			}
			curvePanel.clampSpace = clampSpace / base.Controller.SequenceLength;
			if (fieldLimits != null && fieldLimits.HasAxisFieldLimit(Axis.AxisField.name))
			{
				fieldLimits.GetSoftLimits(Axis.AxisField.name);
				curvePanel.SetLimits(fieldLimits.GetHardLimits(Axis.AxisField.name), fieldLimits.GetSoftLimits(Axis.AxisField.name));
				curvePanel.ShowLimits();
			}
			curvePanelTextHover = curvePanel.GetComponent<UIHoverText>();
			if (curvePanelTextHover != null && base.Window != null)
			{
				curvePanelTextHover.textTargetForHover = base.Window.StatusHelpLabel;
				curvePanelTextHover.hoverText = Localizer.Format("#autoLOC_8003257", Axis.AxisField.guiName);
			}
			axisMaxLimitTextHover = axisMaxLimit.GetComponent<UIHoverText>();
			if (axisMaxLimitTextHover != null && base.Window != null)
			{
				axisMaxLimitTextHover.textTargetForHover = base.Window.StatusHelpLabel;
			}
			axisMinLimitTextHover = axisMinLimit.GetComponent<UIHoverText>();
			if (axisMinLimitTextHover != null && base.Window != null)
			{
				axisMinLimitTextHover.textTargetForHover = base.Window.StatusHelpLabel;
			}
			CurvePanel obj = curvePanel;
			obj.OnPointSelectionChanged = (Callback<CurvePanel, List<CurvePanelPoint>>)Delegate.Combine(obj.OnPointSelectionChanged, new Callback<CurvePanel, List<CurvePanelPoint>>(OnPointSelectionChanged));
			CurvePanel obj2 = curvePanel;
			obj2.OnPointDragging = (Callback<List<CurvePanelPoint>>)Delegate.Combine(obj2.OnPointDragging, new Callback<List<CurvePanelPoint>>(OnPointDragging));
			CurvePanel obj3 = curvePanel;
			obj3.OnPanelLeftClick = (Callback)Delegate.Combine(obj3.OnPanelLeftClick, new Callback(OnPanelLeftClick));
			UpdateUILayout(recreateLine: true);
			if (fieldLimits != null && fieldLimits.HasAxisFieldLimit(Axis.AxisField.name))
			{
				Vector2 softLimits = fieldLimits.GetSoftLimits(Axis.AxisField.name);
				axisMinLimit.UpdateYPosition(curvePanel.PanelYPosFromValue(softLimits.x));
				axisMaxLimit.UpdateYPosition(curvePanel.PanelYPosFromValue(softLimits.y));
			}
		}

		protected override void OnRowDestroy()
		{
			CurvePanel obj = curvePanel;
			obj.OnPointSelectionChanged = (Callback<CurvePanel, List<CurvePanelPoint>>)Delegate.Remove(obj.OnPointSelectionChanged, new Callback<CurvePanel, List<CurvePanelPoint>>(OnPointSelectionChanged));
			CurvePanel obj2 = curvePanel;
			obj2.OnPointDragging = (Callback<List<CurvePanelPoint>>)Delegate.Remove(obj2.OnPointDragging, new Callback<List<CurvePanelPoint>>(OnPointDragging));
			CurvePanel obj3 = curvePanel;
			obj3.OnPanelLeftClick = (Callback)Delegate.Remove(obj3.OnPanelLeftClick, new Callback(OnPanelLeftClick));
			if (fieldLimits != null && fieldLimits.HasAxisFieldLimit(Axis.AxisField.name))
			{
				IAxisFieldLimits axisFieldLimits = fieldLimits;
				axisFieldLimits.LimitsChanged = (Callback<AxisFieldLimit>)Delegate.Remove(axisFieldLimits.LimitsChanged, new Callback<AxisFieldLimit>(OnLimitsChanged));
			}
			fieldLimits = null;
			Axis = null;
		}

		protected override void OnRowCollapsed()
		{
			if (curvePanel != null)
			{
				curvePanel.SetEditable(editable: false);
				curvePanel.ClearSelectedPoints();
			}
			if (curvePanelTextHover != null)
			{
				curvePanelTextHover.hoverText = Localizer.Format("#autoLOC_8003257", Axis.AxisField.guiName);
			}
			Axis.ClearPresetRefs();
			UpdateUILayout();
		}

		protected override void OnRowExpanded()
		{
			if (curvePanel != null)
			{
				curvePanel.SetEditable(editable: true);
			}
			if (curvePanelTextHover != null)
			{
				curvePanelTextHover.hoverText = Localizer.Format("#autoLOC_8003258", Axis.AxisField.guiName);
				curvePanelTextHover.UpdateText();
			}
			SelectPointAtTime(base.Controller.SequencePosition);
			if (fieldLimits != null && fieldLimits.HasAxisFieldLimit(Axis.AxisField.name))
			{
				Vector2 softLimits = fieldLimits.GetSoftLimits(Axis.AxisField.name);
				axisMinLimit.UpdateYPosition(curvePanel.PanelYPosFromValue(softLimits.x));
				axisMaxLimit.UpdateYPosition(curvePanel.PanelYPosFromValue(softLimits.y));
			}
		}

		protected override void UpdateUILayout(bool recreateLine = false)
		{
			SetLayoutElements();
			Canvas.ForceUpdateCanvases();
			base.Window.OnWindowAxisChanged();
			curvePanel.DrawAll(recreateLine);
		}

		internal override void RedrawCurve()
		{
			Canvas.ForceUpdateCanvases();
			curvePanel.DrawAll(newline: false);
		}

		internal override void ReloadCurve()
		{
			Canvas.ForceUpdateCanvases();
			curvePanel.SetCurve(Axis.timeValue);
			curvePanel.DrawAll();
		}

		private void SetLayoutElements()
		{
			for (int i = 0; i < hideWhenCollapsed.Count; i++)
			{
				hideWhenCollapsed[i].SetActive(expanded);
			}
			if (expanded)
			{
				layout.preferredHeight = heightExpanded;
			}
			else
			{
				layout.preferredHeight = heightCollapsed;
			}
			headerSize = headerTransform.sizeDelta;
			if (expanded && showLimitsEditorWidget && HighLogic.LoadedSceneIsEditor && fieldLimits != null && fieldLimits.HasAxisFieldLimit(Axis.AxisField.name))
			{
				axisLimits.gameObject.SetActive(value: true);
				headerSize.x = headerWidth;
			}
			else
			{
				axisLimits.gameObject.SetActive(value: false);
				headerSize.x = headerWidth + axisWidth;
			}
			headerTransform.sizeDelta = headerSize;
		}

		public void ShowPoints()
		{
			if (curvePanel != null)
			{
				curvePanel.ShowPoints();
			}
		}

		public void HidePoints()
		{
			if (curvePanel != null)
			{
				curvePanel.HidePoints();
			}
		}

		public override void InsertPoint(float timeValue)
		{
			curvePanel.InsertPoint(timeValue / base.Controller.SequenceLength);
		}

		public override void ReverseCurve()
		{
			List<int> list = new List<int>(curvePanel.SelectedPointIndexes);
			Axis.ReverseCurve();
			ReloadCurve();
			if (list.Count > 0)
			{
				for (int i = 0; i < list.Count; i++)
				{
					curvePanel.SelectPoint(Axis.timeValue.Curve.length - 1 - list[i]);
				}
			}
		}

		public void InvertCurve()
		{
			List<int> list = new List<int>(curvePanel.SelectedPointIndexes);
			Axis.InvertCurve();
			ReloadCurve();
			if (list.Count > 0)
			{
				for (int i = 0; i < list.Count; i++)
				{
					curvePanel.SelectPoint(list[i]);
				}
			}
		}

		public void AlignCurveEnds()
		{
			List<int> list = new List<int>(curvePanel.SelectedPointIndexes);
			Axis.AlignCurveEnds();
			ReloadCurve();
			if (list.Count > 0)
			{
				for (int i = 0; i < list.Count; i++)
				{
					curvePanel.SelectPoint(list[i]);
				}
			}
		}

		public void ClampAllPointValues()
		{
			List<int> list = new List<int>(curvePanel.SelectedPointIndexes);
			Axis.ClampAllPointValues();
			ReloadCurve();
			if (list.Count > 0)
			{
				for (int i = 0; i < list.Count; i++)
				{
					curvePanel.SelectPoint(list[i]);
				}
			}
		}

		public void ClampPointValues()
		{
			List<int> list = new List<int>(curvePanel.SelectedPointIndexes);
			for (int i = 0; i < list.Count; i++)
			{
				Axis.ClampPointValue(list[i]);
			}
			ReloadCurve();
			if (list.Count > 0)
			{
				for (int j = 0; j < list.Count; j++)
				{
					curvePanel.SelectPoint(list[j]);
				}
			}
		}

		public void PresetFlat()
		{
			Axis.PresetFlat();
			ReloadCurve();
		}

		public void PresetSine(float cycles, float phase)
		{
			Axis.PresetSine(cycles, phase);
			ReloadCurve();
		}

		public void PresetSquare(float cycles, float phase)
		{
			Axis.PresetSquare(cycles, phase);
			ReloadCurve();
		}

		public void PresetTriangle(float cycles, float phase)
		{
			Axis.PresetTriangle(cycles, phase);
			ReloadCurve();
		}

		public void PresetSaw(float cycles, float phase)
		{
			Axis.PresetSaw(cycles, phase);
			ReloadCurve();
		}

		public void PresetRevSaw(float cycles, float phase)
		{
			Axis.PresetRevSaw(cycles, phase);
			ReloadCurve();
		}

		public void UpdatePreset(float cycles, float phase)
		{
			Axis.UpdatePreset(cycles, phase);
			ReloadCurve();
		}

		internal void ClearPresetRefs()
		{
			Axis.ClearPresetRefs();
		}

		public override void SelectAllPoints()
		{
			curvePanel.SelectAllPoints();
		}

		public override void SelectPointAtTime(float timeValue)
		{
			curvePanel.SelectPoint(timeValue / base.Controller.SequenceLength);
		}

		protected override void OnPointSelectionChanged(CurvePanel panel, List<CurvePanelPoint> points)
		{
			base.Window.OnPointSelected(this, points);
		}

		protected override void OnPointDragging(List<CurvePanelPoint> points)
		{
			base.Window.OnPointDragging(this, points);
		}

		private void OnPanelLeftClick()
		{
			if (!curvePanel.Editable)
			{
				Expand();
			}
		}

		private void OnLimitsChanged(AxisFieldLimit newLimits)
		{
			if (newLimits == null)
			{
				curvePanel.SetLimits(new Vector2(Axis.axisMin, Axis.axisMax));
			}
			else
			{
				curvePanel.SetLimits(newLimits.hardLimits, newLimits.softLimits);
			}
			axisMinLimit.UpdateYPosition(curvePanel.PanelYPosFromValue(newLimits.softLimits.x));
			axisMaxLimit.UpdateYPosition(curvePanel.PanelYPosFromValue(newLimits.softLimits.y));
			curvePanel.DrawAll();
		}

		internal void BeginDragAxisLimit(AxisLimitLine line, PointerEventData eventData)
		{
		}

		internal void EndDragAxisLimit(AxisLimitLine line, PointerEventData eventData)
		{
		}

		internal void OnDragAxisLimit(AxisLimitLine line, PointerEventData eventData)
		{
			float yPos = line.transform.localPosition.y + eventData.delta.y - line.deadZone;
			float num = curvePanel.CurveValueFromYPos(yPos);
			line.SetToolTipValue(num);
			MoveAxisLimits(line.LimitType, num);
		}

		private void MoveAxisLimits(AxisLimitLine.LimitOptions limitType, float newValue)
		{
			if (Axis.AxisField != null && Axis.AxisField.module is IAxisFieldLimits axisFieldLimits)
			{
				moveAxisCache = axisFieldLimits.GetSoftLimits(Axis.axisName);
				if (limitType == AxisLimitLine.LimitOptions.Max)
				{
					moveAxisCache.y = Mathf.Max(newValue, moveAxisCache.x);
				}
				else
				{
					moveAxisCache.x = Mathf.Min(newValue, moveAxisCache.y);
				}
				Axis.UpdateSoftLimits(moveAxisCache);
			}
		}
	}
}
