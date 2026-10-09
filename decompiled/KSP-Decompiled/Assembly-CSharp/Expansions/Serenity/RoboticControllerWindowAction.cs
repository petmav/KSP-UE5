using System;
using System.Collections.Generic;
using KSP.Localization;
using KSP.UI;
using UnityEngine;

namespace Expansions.Serenity
{
	public class RoboticControllerWindowAction : RoboticControllerWindowBaseRow
	{
		[SerializeField]
		private CurvePanel curvePanel;

		private UIHoverText curvePanelTextHover;

		private FloatCurve timesCurve;

		private float floatCurveValue = 0.5f;

		private float headerWidth;

		private float axisWidth;

		private Vector2 headerSize;

		public ControlledAction Action { get; private set; }

		public static RoboticControllerWindowAction Spawn(RoboticControllerWindow window, ControlledAction action, Transform parent)
		{
			if (!ExpansionsLoader.IsExpansionInstalled("Serenity"))
			{
				return null;
			}
			UnityEngine.Object @object = SerenityUtils.SerenityPrefab("_UI5/Controller/Prefabs/ControllerAction.prefab");
			if (@object == null)
			{
				Debug.LogError("[RoboticControllerWindowAction]: Unable to load the Asset");
				return null;
			}
			if (action.Part == null)
			{
				Debug.LogWarning("[RoboticControllerWindowAction]: Action has no Part, not adding this axis");
				return null;
			}
			if (action.Action == null)
			{
				Debug.LogWarning("[RoboticControllerWindowAction]: Action has no Field, not adding this axis");
				return null;
			}
			RoboticControllerWindowAction component = ((GameObject)UnityEngine.Object.Instantiate(@object)).GetComponent<RoboticControllerWindowAction>();
			component.name = action.Part.partInfo.title + " - " + action.Action.guiName;
			component.transform.SetParent(parent);
			component.transform.localPosition = Vector3.zero;
			component.transform.localScale = Vector3.one;
			component.Setup(window, action);
			return component;
		}

		public void Setup(RoboticControllerWindow window, ControlledAction action)
		{
			AssignBaseReferenceVars(window, window.Controller, action);
			Action = action;
			string text = action.PartNickName;
			if (text == "")
			{
				text = action.Part.partInfo.title;
			}
			titleTextPart.text = text;
			titleTextField.text = action.Action.guiName;
			editNickButton.interactable = true;
			editNickButton.gameObject.SetActive(value: true);
			partNickNameInput.gameObject.SetActive(value: false);
			expanded = false;
		}

		protected override void OnRowStart()
		{
			headerWidth = headerTransform.rect.width;
			SetLayoutElements();
			if (curvePanel == null)
			{
				Debug.LogError("[RoboticControllerWindowAction] No CurvePanel to use when drawing em");
				return;
			}
			LoadTimesCurve();
			curvePanel.Setup(timesCurve, "Line-" + Action.Part.partInfo.title + "-" + Action.Action.guiName);
			curvePanel.SetLimits(new Vector2(0f, 1f));
			curvePanelTextHover = curvePanel.GetComponent<UIHoverText>();
			if (curvePanelTextHover != null && base.Window != null)
			{
				curvePanelTextHover.textTargetForHover = base.Window.StatusHelpLabel;
				curvePanelTextHover.hoverText = Localizer.Format("#autoLOC_8003257", Action.Action.guiName);
			}
			CurvePanel obj = curvePanel;
			obj.OnPointSelectionChanged = (Callback<CurvePanel, List<CurvePanelPoint>>)Delegate.Combine(obj.OnPointSelectionChanged, new Callback<CurvePanel, List<CurvePanelPoint>>(OnPointSelectionChanged));
			CurvePanel obj2 = curvePanel;
			obj2.OnPointDragging = (Callback<List<CurvePanelPoint>>)Delegate.Combine(obj2.OnPointDragging, new Callback<List<CurvePanelPoint>>(OnPointDragging));
			CurvePanel obj3 = curvePanel;
			obj3.OnPanelLeftClick = (Callback)Delegate.Combine(obj3.OnPanelLeftClick, new Callback(OnPanelLeftClick));
			CurvePanel obj4 = curvePanel;
			obj4.OnPointAdded = (Callback<CurvePanel.CurvePanelPointChangeInfo>)Delegate.Combine(obj4.OnPointAdded, new Callback<CurvePanel.CurvePanelPointChangeInfo>(OnPointAdded));
			CurvePanel obj5 = curvePanel;
			obj5.OnPointDeleted = (Callback<int>)Delegate.Combine(obj5.OnPointDeleted, new Callback<int>(OnPointDeleted));
			CurvePanel obj6 = curvePanel;
			obj6.OnPointMoved = (Callback<CurvePanel.CurvePanelPointChangeInfo>)Delegate.Combine(obj6.OnPointMoved, new Callback<CurvePanel.CurvePanelPointChangeInfo>(OnPointMoved));
			UpdateUILayout(recreateLine: true);
		}

		protected override void OnRowDestroy()
		{
			CurvePanel obj = curvePanel;
			obj.OnPointSelectionChanged = (Callback<CurvePanel, List<CurvePanelPoint>>)Delegate.Remove(obj.OnPointSelectionChanged, new Callback<CurvePanel, List<CurvePanelPoint>>(OnPointSelectionChanged));
			CurvePanel obj2 = curvePanel;
			obj2.OnPointDragging = (Callback<List<CurvePanelPoint>>)Delegate.Remove(obj2.OnPointDragging, new Callback<List<CurvePanelPoint>>(OnPointDragging));
			CurvePanel obj3 = curvePanel;
			obj3.OnPanelLeftClick = (Callback)Delegate.Remove(obj3.OnPanelLeftClick, new Callback(OnPanelLeftClick));
			CurvePanel obj4 = curvePanel;
			obj4.OnPointAdded = (Callback<CurvePanel.CurvePanelPointChangeInfo>)Delegate.Remove(obj4.OnPointAdded, new Callback<CurvePanel.CurvePanelPointChangeInfo>(OnPointAdded));
			CurvePanel obj5 = curvePanel;
			obj5.OnPointDeleted = (Callback<int>)Delegate.Remove(obj5.OnPointDeleted, new Callback<int>(OnPointDeleted));
			CurvePanel obj6 = curvePanel;
			obj6.OnPointMoved = (Callback<CurvePanel.CurvePanelPointChangeInfo>)Delegate.Remove(obj6.OnPointMoved, new Callback<CurvePanel.CurvePanelPointChangeInfo>(OnPointMoved));
			Action = null;
		}

		private void LoadTimesCurve()
		{
			timesCurve = new FloatCurve();
			for (int i = 0; i < Action.times.Count; i++)
			{
				timesCurve.Add(Action.times[i], floatCurveValue);
			}
			timesCurve.Add(-0.1f, floatCurveValue);
			timesCurve.Add(1.1f, floatCurveValue);
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
				curvePanelTextHover.hoverText = Localizer.Format("#autoLOC_8003257", Action.Action.guiName);
			}
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
				curvePanelTextHover.hoverText = Localizer.Format("#autoLOC_8003258", Action.Action.guiName);
				curvePanelTextHover.UpdateText();
			}
			SelectPointAtTime(base.Controller.SequencePosition);
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
			LoadTimesCurve();
			curvePanel.SetCurve(timesCurve);
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
			Action.ReverseTimes();
			ReloadCurve();
			if (list.Count > 0)
			{
				for (int i = 0; i < list.Count; i++)
				{
					curvePanel.SelectPoint(Action.times.Count - 1 - list[i]);
				}
			}
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

		private void OnPointAdded(CurvePanel.CurvePanelPointChangeInfo info)
		{
			Action.times.Insert(info.index - 1, info.time);
		}

		private void OnPointDeleted(int pointIndex)
		{
			Action.times.RemoveAt(pointIndex - 1);
		}

		private void OnPointMoved(CurvePanel.CurvePanelPointChangeInfo info)
		{
			Action.times[info.index - 1] = info.time;
		}

		private void OnPanelLeftClick()
		{
			if (!curvePanel.Editable)
			{
				Expand();
			}
		}
	}
}
