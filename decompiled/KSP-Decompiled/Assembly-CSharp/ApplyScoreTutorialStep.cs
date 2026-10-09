using System;
using System.Collections.Generic;
using Expansions.Missions;
using Expansions.Missions.Editor;
using UnityEngine;

public class ApplyScoreTutorialStep : IntermediateTutorialPageStep
{
	private const string intermediateTitleLoc = "#autoLOC_9990010";

	private const int erronousIndex = -1;

	private const int applyScoreNodeAttended = 1;

	private const int modifierCountAttended = 1;

	private const int nodeCountRequired = 1;

	private const int dialogNodeCountRequired = 2;

	private const int vesselNodeCountRequired = 2;

	private const int timeSinceNodeCountRequired = 1;

	private const int applyNodeCountRequired = 1;

	public ApplyScoreTutorialStep(IntermediateTutorial tutorialScenario, TutorialScenario.TutorialFSM tutorialFsm)
		: base(tutorialScenario, tutorialFsm)
	{
	}

	protected override void AddTutorialStepConfig()
	{
		AddTutorialStepConfig("dragApplyScoreNode", "#autoLOC_9990010", "#autoLOC_9990051", OnEnterDragApplyScoreNode, METutorialScenario.TutorialButtonType.NoButton);
		AddTutorialStepConfig("dragApplyScoreNodeHelper", "#autoLOC_9990010", "#autoLOC_9990052", OnEnterDragApplyScoreNodeHelper, METutorialScenario.TutorialButtonType.NoButton);
		AddTutorialStepConfig("setupApplyScoreNode", "#autoLOC_9990010", "#autoLOC_9990053", OnEnterSetupApplyScoreNode, METutorialScenario.TutorialButtonType.NoButton);
		AddTutorialStepConfig("setupApplyScoreNodeHelper", "#autoLOC_9990010", "#autoLOC_9990054", OnEnterSetupApplyScoreNodeHelper, METutorialScenario.TutorialButtonType.NoButton);
		AddTutorialStepConfig("linkAllIntro", "#autoLOC_9990010", "#autoLOC_9990055", base.OnEnterEmpty);
		AddTutorialStepConfig("linkExplanation", "#autoLOC_9990010", "#autoLOC_9990056", OnEnterLinkExplanation, METutorialScenario.TutorialButtonType.NoButton);
		AddTutorialStepConfig("linkExplanationHelper", "#autoLOC_9990010", "#autoLOC_9990057", OnEnterLinkExplanation, METutorialScenario.TutorialButtonType.NoButton);
	}

	private void OnEnterDragApplyScoreNode(KFSMState kfsmState)
	{
		KFSMState currentState = tutorial.CurrentState;
		currentState.OnLeave = (KFSMStateChange)Delegate.Combine(currentState.OnLeave, new KFSMStateChange(OnLeaveDragApplyScoreNode));
		tutorialScenario.DragNodeHelper(FilterApplyNodeCriteria, "CurrencyAndScore");
		GameEvents.Mission.onBuilderNodeFocused.Add(OnNodeFocused);
	}

	private bool FilterApplyNodeCriteria(MEGUINodeIcon meguiNodeIcon)
	{
		if (meguiNodeIcon.basicNode != null && meguiNodeIcon.basicNode.name == "MissionScore".ToString())
		{
			return meguiNodeIcon.basicNode.actions.Contains("ActionMissionScore");
		}
		return false;
	}

	private void OnNodeFocused(MENode data)
	{
		if (HasApplyScoreNode())
		{
			tutorial.GoToNextPage();
		}
	}

	private bool HasApplyScoreNode()
	{
		List<MEGUINode> mEGUINodes = tutorialScenario.GetMEGUINodes("MissionScore");
		return ((mEGUINodes.Count == 1) ? tutorialScenario.GetMEGUINodeIndex(mEGUINodes[0]) : (-1)) == 9;
	}

	private void OnSelectedGameObjectChange(GameObject gameObject)
	{
		if (IsLinksConnected())
		{
			tutorial.GoToNextPage();
		}
	}

	private void OnLeaveDragApplyScoreNode(KFSMState kfsmState)
	{
		KFSMState currentState = tutorial.CurrentState;
		currentState.OnLeave = (KFSMStateChange)Delegate.Remove(currentState.OnLeave, new KFSMStateChange(OnLeaveDragApplyScoreNode));
		tutorialScenario.RemoveDragHelper();
		GameEvents.Mission.onBuilderNodeFocused.Remove(OnNodeFocused);
	}

	private void OnEnterDragApplyScoreNodeHelper(KFSMState kfsmState)
	{
		KFSMState currentState = tutorial.CurrentState;
		currentState.OnLeave = (KFSMStateChange)Delegate.Combine(currentState.OnLeave, new KFSMStateChange(OnLeaveDragApplyScoreNodeHelper));
		if (!HasApplyScoreNode())
		{
			tutorialScenario.DragNodeHelper(FilterApplyNodeCriteria);
			GameEvents.Mission.onBuilderNodeFocused.Add(OnNodeFocused);
		}
		else
		{
			tutorial.GoToNextPage();
		}
	}

	private void OnLeaveDragApplyScoreNodeHelper(KFSMState kfsmState)
	{
		KFSMState currentState = tutorial.CurrentState;
		currentState.OnLeave = (KFSMStateChange)Delegate.Remove(currentState.OnLeave, new KFSMStateChange(OnLeaveDragApplyScoreNodeHelper));
		tutorialScenario.RemoveDragHelper();
		GameEvents.Mission.onBuilderNodeFocused.Remove(OnNodeFocused);
	}

	private void OnEnterSetupApplyScoreNode(KFSMState kfsmState)
	{
		KFSMState currentState = tutorial.CurrentState;
		currentState.OnUpdate = (KFSMCallback)Delegate.Combine(currentState.OnUpdate, new KFSMCallback(OnUpdateSetupApplyScoreNode));
		KFSMState currentState2 = tutorial.CurrentState;
		currentState2.OnLeave = (KFSMStateChange)Delegate.Combine(currentState2.OnLeave, new KFSMStateChange(OnLeaveSetupApplyScoreNode));
		tutorialScenario.LockNodeSettings(locked: false);
	}

	private void OnUpdateSetupApplyScoreNode()
	{
		if (!CanHighlightApplyScoreSetup())
		{
			tutorial.GoToNextPage();
		}
	}

	private bool CanHighlightApplyScoreSetup()
	{
		if (!(tutorialScenario.GetSelectedNodeTemplateName() != "MissionScore".ToString()) && tutorialScenario.GetSelectedNodeIndex() == 9)
		{
			bool result = true;
			MEGUIParameterDynamicModuleList mEGUIParameterDynamicModuleList = tutorialScenario.GetApplyScoreScoreParam() as MEGUIParameterDynamicModuleList;
			if (mEGUIParameterDynamicModuleList != null && mEGUIParameterDynamicModuleList.FieldValue.activeModules.Count == 1 && mEGUIParameterDynamicModuleList.FieldValue.activeModules[0] is ScoreModule_Completion scoreModule_Completion && scoreModule_Completion.bonusScore != 0f)
			{
				result = false;
			}
			return result;
		}
		tutorialScenario.HideAllTutorialSelectors();
		return true;
	}

	private void OnLeaveSetupApplyScoreNode(KFSMState kfsmState)
	{
		KFSMState currentState = tutorial.CurrentState;
		currentState.OnUpdate = (KFSMCallback)Delegate.Remove(currentState.OnUpdate, new KFSMCallback(OnUpdateSetupApplyScoreNode));
		KFSMState currentState2 = tutorial.CurrentState;
		currentState2.OnLeave = (KFSMStateChange)Delegate.Remove(currentState2.OnLeave, new KFSMStateChange(OnLeaveSetupApplyScoreNode));
		tutorialScenario.LockNodeSettings(locked: true);
	}

	private void OnEnterSetupApplyScoreNodeHelper(KFSMState kfsmState)
	{
		KFSMState currentState = tutorial.CurrentState;
		currentState.OnUpdate = (KFSMCallback)Delegate.Combine(currentState.OnUpdate, new KFSMCallback(OnUpdateSetupApplyScoreNodeHelper));
		KFSMState currentState2 = tutorial.CurrentState;
		currentState2.OnLeave = (KFSMStateChange)Delegate.Combine(currentState2.OnLeave, new KFSMStateChange(OnLeaveSetupApplyScoreNodeHelper));
		tutorialScenario.LockNodeSettings(locked: false);
	}

	private void OnUpdateSetupApplyScoreNodeHelper()
	{
		if (!CanHighlightApplyScoreSetup())
		{
			tutorial.GoToNextPage();
		}
	}

	private void OnLeaveSetupApplyScoreNodeHelper(KFSMState kfsmState)
	{
		KFSMState currentState = tutorial.CurrentState;
		currentState.OnUpdate = (KFSMCallback)Delegate.Remove(currentState.OnUpdate, new KFSMCallback(OnUpdateSetupApplyScoreNodeHelper));
		KFSMState currentState2 = tutorial.CurrentState;
		currentState2.OnLeave = (KFSMStateChange)Delegate.Remove(currentState2.OnLeave, new KFSMStateChange(OnLeaveSetupApplyScoreNodeHelper));
		tutorialScenario.LockNodeSettings(locked: true);
	}

	private void OnEnterLinkExplanation(KFSMState kfsmState)
	{
		KFSMState currentState = tutorial.CurrentState;
		currentState.OnUpdate = (KFSMCallback)Delegate.Combine(currentState.OnUpdate, new KFSMCallback(OnUpdateLinkExplanation));
		KFSMState currentState2 = tutorial.CurrentState;
		currentState2.OnLeave = (KFSMStateChange)Delegate.Combine(currentState2.OnLeave, new KFSMStateChange(OnLeaveLinkExplanation));
		MissionEditorLogic instance = MissionEditorLogic.Instance;
		instance.OnSelectedGameObjectChange = (Action<GameObject>)Delegate.Combine(instance.OnSelectedGameObjectChange, new Action<GameObject>(OnSelectedGameObjectChange));
		tutorialScenario.LockCanvas(locked: false);
	}

	private void OnUpdateLinkExplanation()
	{
		if (IsLinksConnected())
		{
			tutorial.GoToNextPage();
		}
	}

	private bool IsLinksConnected()
	{
		if (tutorialScenario.IsAllNodeLinked(METutorialScenario.NodeDefinition.ScienceExperiment, METutorialScenario.NodeDefinition.TimeSinceNode) && tutorialScenario.IsAllNodeLinked(METutorialScenario.NodeDefinition.TimeSinceNode, METutorialScenario.NodeDefinition.ApplyScore))
		{
			return tutorialScenario.IsAllNodeLinked(METutorialScenario.NodeDefinition.ApplyScore, METutorialScenario.NodeDefinition.DialogMessage2);
		}
		return false;
	}

	private void OnLeaveLinkExplanation(KFSMState kfsmState)
	{
		KFSMState currentState = tutorial.CurrentState;
		currentState.OnUpdate = (KFSMCallback)Delegate.Remove(currentState.OnUpdate, new KFSMCallback(OnUpdateLinkExplanation));
		KFSMState currentState2 = tutorial.CurrentState;
		currentState2.OnLeave = (KFSMStateChange)Delegate.Remove(currentState2.OnLeave, new KFSMStateChange(OnLeaveLinkExplanation));
		MissionEditorLogic instance = MissionEditorLogic.Instance;
		instance.OnSelectedGameObjectChange = (Action<GameObject>)Delegate.Remove(instance.OnSelectedGameObjectChange, new Action<GameObject>(OnSelectedGameObjectChange));
		tutorialScenario.LockCanvas(locked: true);
	}
}
