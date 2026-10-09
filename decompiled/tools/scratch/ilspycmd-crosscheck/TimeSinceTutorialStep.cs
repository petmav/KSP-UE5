using System;
using System.Collections.Generic;
using Expansions.Missions;
using Expansions.Missions.Editor;
using UnityEngine;

public class TimeSinceTutorialStep : IntermediateTutorialPageStep
{
	private const string intermediateTitleLoc = "#autoLOC_9990010";

	private const string noTime = "0";

	private const string oneDayTime = "1";

	private const int erronousIndex = -1;

	private const int timeNodeAttended = 1;

	private const int operatorAttended = 0;

	public TimeSinceTutorialStep(IntermediateTutorial tutorialScenario, TutorialScenario.TutorialFSM tutorialFsm)
		: base(tutorialScenario, tutorialFsm)
	{
	}

	protected override void AddTutorialStepConfig()
	{
		AddTutorialStepConfig("scoreIntro", "#autoLOC_9990010", "#autoLOC_9990044", base.OnEnterEmpty);
		AddTutorialStepConfig("timeSinceIntro", "#autoLOC_9990010", "#autoLOC_9990045", base.OnEnterEmpty);
		AddTutorialStepConfig("dragTimeSinceNode", "#autoLOC_9990010", "#autoLOC_9990046", OnEnterDragTimeSinceNode, METutorialScenario.TutorialButtonType.NoButton);
		AddTutorialStepConfig("dragTimeSinceNodeHelper", "#autoLOC_9990010", "#autoLOC_9990047", OnEnterDragTimeSinceNodeHelper, METutorialScenario.TutorialButtonType.NoButton);
		AddTutorialStepConfig("setTimeNodeIntro", "#autoLOC_9990010", "#autoLOC_9990048", base.OnEnterEmpty);
		AddTutorialStepConfig("OnEnterSetupTimeNode", "#autoLOC_9990010", "#autoLOC_9990049", OnEnterSetupTimeNode, METutorialScenario.TutorialButtonType.NoButton);
		AddTutorialStepConfig("OnEnterSetupTimeNodeHelper", "#autoLOC_9990010", "#autoLOC_9990050", OnEnterSetupTimeNodeHelper);
	}

	private void OnEnterDragTimeSinceNode(KFSMState kfsmState)
	{
		KFSMState currentState = tutorial.CurrentState;
		currentState.OnLeave = (KFSMStateChange)Delegate.Combine(currentState.OnLeave, new KFSMStateChange(OnLeaveDragTimeSinceNode));
		MissionEditorLogic instance = MissionEditorLogic.Instance;
		instance.OnSelectedGameObjectChange = (Action<GameObject>)Delegate.Combine(instance.OnSelectedGameObjectChange, new Action<GameObject>(OnSelectedGameObjectChange));
		tutorialScenario.DragNodeHelper(FilterTimeNodeCriteria, "Utility");
		GameEvents.Mission.onBuilderNodeFocused.Add(OnNodeFocused);
	}

	private bool FilterTimeNodeCriteria(MEGUINodeIcon meguiNodeIcon)
	{
		if (meguiNodeIcon.basicNode != null)
		{
			return meguiNodeIcon.basicNode.name == "TestTimeSinceNode".ToString();
		}
		return false;
	}

	private void OnNodeFocused(MENode data)
	{
		if (HasTimeSinceNode())
		{
			tutorial.GoToNextPage();
		}
	}

	private bool HasTimeSinceNode()
	{
		List<MEGUINode> mEGUINodes = tutorialScenario.GetMEGUINodes("TestTimeSinceNode");
		return ((mEGUINodes.Count == 1) ? tutorialScenario.GetMEGUINodeIndex(mEGUINodes[0]) : (-1)) == 8;
	}

	private void OnSelectedGameObjectChange(GameObject gameObject)
	{
		tutorial.GoToNextPage();
	}

	private void OnLeaveDragTimeSinceNode(KFSMState kfsmState)
	{
		KFSMState currentState = tutorial.CurrentState;
		currentState.OnLeave = (KFSMStateChange)Delegate.Remove(currentState.OnLeave, new KFSMStateChange(OnLeaveDragTimeSinceNode));
		MissionEditorLogic instance = MissionEditorLogic.Instance;
		instance.OnSelectedGameObjectChange = (Action<GameObject>)Delegate.Remove(instance.OnSelectedGameObjectChange, new Action<GameObject>(OnSelectedGameObjectChange));
		tutorialScenario.RemoveDragHelper();
		GameEvents.Mission.onBuilderNodeFocused.Remove(OnNodeFocused);
	}

	private void OnEnterDragTimeSinceNodeHelper(KFSMState kfsmState)
	{
		KFSMState currentState = tutorial.CurrentState;
		currentState.OnLeave = (KFSMStateChange)Delegate.Combine(currentState.OnLeave, new KFSMStateChange(OnLeaveDragTimeSinceNodeHelper));
		if (!HasTimeSinceNode())
		{
			tutorialScenario.DragNodeHelper(FilterTimeNodeCriteria, "Utility");
			GameEvents.Mission.onBuilderNodeFocused.Add(OnNodeFocused);
		}
		else
		{
			tutorial.GoToNextPage();
		}
	}

	private void OnLeaveDragTimeSinceNodeHelper(KFSMState kfsmState)
	{
		KFSMState currentState = tutorial.CurrentState;
		currentState.OnLeave = (KFSMStateChange)Delegate.Remove(currentState.OnLeave, new KFSMStateChange(OnLeaveDragTimeSinceNodeHelper));
		tutorialScenario.RemoveDragHelper();
		GameEvents.Mission.onBuilderNodeFocused.Remove(OnNodeFocused);
	}

	private void OnEnterSetupTimeNode(KFSMState kfsmState)
	{
		KFSMState currentState = tutorial.CurrentState;
		currentState.OnUpdate = (KFSMCallback)Delegate.Combine(currentState.OnUpdate, new KFSMCallback(OnUpdateSetupTimeNode));
		KFSMState currentState2 = tutorial.CurrentState;
		currentState2.OnLeave = (KFSMStateChange)Delegate.Combine(currentState2.OnLeave, new KFSMStateChange(OnLeaveSetupTimeNode));
		MissionEditorLogic instance = MissionEditorLogic.Instance;
		instance.OnSelectedGameObjectChange = (Action<GameObject>)Delegate.Combine(instance.OnSelectedGameObjectChange, new Action<GameObject>(OnSelectedGameObjectChange));
		tutorialScenario.LockNodeSettings(locked: false);
	}

	private void OnUpdateSetupTimeNode()
	{
		if (IsTimeNodeSetted())
		{
			tutorial.GoToNextPage();
		}
	}

	private bool IsTimeNodeSetted()
	{
		bool flag = CanHighLightStartNode();
		return !((CanHighlightOperator() | CanHighLightTime()) || flag);
	}

	private bool CanHighLightStartNode()
	{
		if (tutorialScenario.GetSelectedNodeTemplateName() != "TestTimeSinceNode")
		{
			tutorialScenario.HideAllTutorialSelectors();
			return true;
		}
		MEGUIParameterNodeDropdownList mEGUIParameterNodeDropdownList = tutorialScenario.GetTimeNodeStartNode() as MEGUIParameterNodeDropdownList;
		int num;
		if (!(mEGUIParameterNodeDropdownList == null))
		{
			num = ((!TimeStartNodeSetupped(mEGUIParameterNodeDropdownList.FieldValue)) ? 1 : 0);
			if (num == 0)
			{
				tutorialScenario.HideTutorialSelection(mEGUIParameterNodeDropdownList);
				return (byte)num != 0;
			}
		}
		else
		{
			num = 1;
		}
		tutorialScenario.ShowTutorialSelection(mEGUIParameterNodeDropdownList);
		return (byte)num != 0;
	}

	private bool TimeStartNodeSetupped(int indexSelected)
	{
		List<MENode>.Enumerator listEnumerator = MissionEditorLogic.Instance.EditorMission.nodes.GetListEnumerator();
		int num = 0;
		while (true)
		{
			if (listEnumerator.MoveNext())
			{
				if (listEnumerator.Current.basicNodeSource == "ScienceExperiment" && num == indexSelected)
				{
					break;
				}
				num++;
				continue;
			}
			return false;
		}
		return true;
	}

	private bool CanHighLightTime()
	{
		if (tutorialScenario.GetSelectedNodeTemplateName() != "TestTimeSinceNode".ToString())
		{
			tutorialScenario.HideAllTutorialSelectors();
			return true;
		}
		MEGUIParameterTime mEGUIParameterTime = tutorialScenario.GetTimeNodeTime() as MEGUIParameterTime;
		int num;
		if (!(mEGUIParameterTime == null) && !(mEGUIParameterTime.timeControl == null))
		{
			num = ((!TimeEqualOneDay(mEGUIParameterTime.timeControl)) ? 1 : 0);
			if (num == 0)
			{
				tutorialScenario.HideTutorialSelection(mEGUIParameterTime);
				return (byte)num != 0;
			}
		}
		else
		{
			num = 1;
		}
		tutorialScenario.ShowTutorialSelection(mEGUIParameterTime);
		return (byte)num != 0;
	}

	private bool TimeEqualOneDay(MEGUITimeControl timeControl)
	{
		if (timeControl.timeDays.text == "1" && timeControl.timeYears.text == "0" && timeControl.timeHours.text == "0" && timeControl.timeMins.text == "0")
		{
			return timeControl.timeSecs.text == "0";
		}
		return false;
	}

	private bool CanHighlightOperator()
	{
		if (tutorialScenario.GetSelectedNodeTemplateName() != "TestTimeSinceNode".ToString())
		{
			tutorialScenario.HideAllTutorialSelectors();
			return true;
		}
		MEGUIParameterDropdownList mEGUIParameterDropdownList = tutorialScenario.GetTimeNodeOperator() as MEGUIParameterDropdownList;
		int num;
		if (!(mEGUIParameterDropdownList == null))
		{
			num = ((mEGUIParameterDropdownList.FieldValue != 0) ? 1 : 0);
			if (num == 0)
			{
				tutorialScenario.HideTutorialSelection(mEGUIParameterDropdownList);
				return (byte)num != 0;
			}
		}
		else
		{
			num = 1;
		}
		tutorialScenario.ShowTutorialSelection(mEGUIParameterDropdownList);
		return (byte)num != 0;
	}

	private void OnLeaveSetupTimeNode(KFSMState kfsmState)
	{
		MissionEditorLogic instance = MissionEditorLogic.Instance;
		instance.OnSelectedGameObjectChange = (Action<GameObject>)Delegate.Remove(instance.OnSelectedGameObjectChange, new Action<GameObject>(OnSelectedGameObjectChange));
		tutorialScenario.HideAllTutorialSelectors();
		KFSMState currentState = tutorial.CurrentState;
		currentState.OnUpdate = (KFSMCallback)Delegate.Remove(currentState.OnUpdate, new KFSMCallback(OnUpdateSetupTimeNode));
		KFSMState currentState2 = tutorial.CurrentState;
		currentState2.OnLeave = (KFSMStateChange)Delegate.Remove(currentState2.OnLeave, new KFSMStateChange(OnLeaveSetupTimeNode));
		tutorialScenario.LockNodeSettings(locked: true);
		tutorialScenario.ResetNodeSettingsMask();
	}

	private void OnEnterSetupTimeNodeHelper(KFSMState kfsmState)
	{
		KFSMState currentState = tutorial.CurrentState;
		currentState.OnUpdate = (KFSMCallback)Delegate.Combine(currentState.OnUpdate, new KFSMCallback(OnUpdateSetupTimeNodeHelper));
		KFSMState currentState2 = tutorial.CurrentState;
		currentState2.OnLeave = (KFSMStateChange)Delegate.Combine(currentState2.OnLeave, new KFSMStateChange(OnLeaveSetupTimeNodeHelper));
		tutorialScenario.LockNodeSettings(locked: false);
	}

	private void OnUpdateSetupTimeNodeHelper()
	{
		tutorialScenario.EnableAllTutorialPageButtons(IsTimeNodeSetted());
	}

	private void OnLeaveSetupTimeNodeHelper(KFSMState kfsmState)
	{
		tutorialScenario.HideAllTutorialSelectors();
		tutorialScenario.LockNodeSettings(locked: true);
		tutorialScenario.ResetNodeSettingsMask();
		KFSMState currentState = tutorial.CurrentState;
		currentState.OnUpdate = (KFSMCallback)Delegate.Remove(currentState.OnUpdate, new KFSMCallback(OnUpdateSetupTimeNodeHelper));
		KFSMState currentState2 = tutorial.CurrentState;
		currentState2.OnLeave = (KFSMStateChange)Delegate.Remove(currentState2.OnLeave, new KFSMStateChange(OnLeaveSetupTimeNodeHelper));
	}
}
