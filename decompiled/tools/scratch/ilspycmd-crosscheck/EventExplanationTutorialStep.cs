using System;
using Expansions.Missions.Editor;
using UnityEngine;

public class EventExplanationTutorialStep : IntermediateTutorialPageStep
{
	private const string intermediateTitleLoc = "#autoLOC_9990010";

	private const string celestialBodyAttended = "Mun";

	private const int experimentAttended = 3;

	private const int situationAttended = 4;

	public EventExplanationTutorialStep(IntermediateTutorial tutorialScenario, TutorialScenario.TutorialFSM tutorialFsm)
		: base(tutorialScenario, tutorialFsm)
	{
	}

	protected override void AddTutorialStepConfig()
	{
		AddTutorialStepConfig("setEventNodeIntro", "#autoLOC_9990010", "#autoLOC_9990031", base.OnEnterEmpty);
		AddTutorialStepConfig("setEventNodeSetting", "#autoLOC_9990010", "#autoLOC_9990032", OnEnterSetEventNodeSetting, METutorialScenario.TutorialButtonType.NoButton);
		AddTutorialStepConfig("setEventNodeSettingHelper", "#autoLOC_9990010", "#autoLOC_9990033", OnEnterSetEventNodeSettingHelper, METutorialScenario.TutorialButtonType.NoButton);
	}

	private void OnEnterSetEventNodeSetting(KFSMState kfsmState)
	{
		KFSMState currentState = tutorial.CurrentState;
		currentState.OnUpdate = (KFSMCallback)Delegate.Combine(currentState.OnUpdate, new KFSMCallback(OnUpdateSetEventNodeSetting));
		KFSMState currentState2 = tutorial.CurrentState;
		currentState2.OnLeave = (KFSMStateChange)Delegate.Combine(currentState2.OnLeave, new KFSMStateChange(OnLeaveSetEventNodeSetting));
		MissionEditorLogic instance = MissionEditorLogic.Instance;
		instance.OnSelectedGameObjectChange = (Action<GameObject>)Delegate.Combine(instance.OnSelectedGameObjectChange, new Action<GameObject>(OnSelectedGameObjectChangeSetEvent));
		tutorialScenario.LockNodeSettings(locked: false);
	}

	private void OnUpdateSetEventNodeSetting()
	{
		CanShowSetEventSelector();
	}

	private bool CanShowSetEventSelector()
	{
		if (tutorialScenario.GetSelectedNodeTemplateName() != "ScienceExperiment".ToString())
		{
			tutorialScenario.HideAllTutorialSelectors();
			return true;
		}
		MEGUIParameterDropdownList mEGUIParameterDropdownList = tutorialScenario.GetScienceExperimentParam() as MEGUIParameterDropdownList;
		MEGUIParameterDropdownList mEGUIParameterDropdownList2 = tutorialScenario.GetScienceSituationParam() as MEGUIParameterDropdownList;
		MEGUIParameterCelestialBody_Biomes mEGUIParameterCelestialBody_Biomes = tutorialScenario.GetScienceCelestialBodyParam() as MEGUIParameterCelestialBody_Biomes;
		bool result = false;
		if (!(mEGUIParameterDropdownList == null) && mEGUIParameterDropdownList.FieldValue == 3)
		{
			tutorialScenario.HideTutorialSelection(mEGUIParameterDropdownList);
		}
		else
		{
			result = true;
			tutorialScenario.ShowTutorialSelection(mEGUIParameterDropdownList);
		}
		if (!(mEGUIParameterDropdownList2 == null) && mEGUIParameterDropdownList2.FieldValue == 4)
		{
			tutorialScenario.HideTutorialSelection(mEGUIParameterDropdownList2);
		}
		else
		{
			result = true;
			tutorialScenario.ShowTutorialSelection(mEGUIParameterDropdownList2);
		}
		if (!(mEGUIParameterCelestialBody_Biomes == null) && !(mEGUIParameterCelestialBody_Biomes.FieldValue.body.bodyName != "Mun"))
		{
			tutorialScenario.HideTutorialSelection(mEGUIParameterCelestialBody_Biomes);
		}
		else
		{
			result = true;
			tutorialScenario.ShowTutorialSelection(mEGUIParameterCelestialBody_Biomes);
		}
		return result;
	}

	private void OnSelectedGameObjectChangeSetEvent(GameObject gameObject)
	{
		tutorial.GoToNextPage();
	}

	private void OnLeaveSetEventNodeSetting(KFSMState kfsmState)
	{
		KFSMState currentState = tutorial.CurrentState;
		currentState.OnUpdate = (KFSMCallback)Delegate.Remove(currentState.OnUpdate, new KFSMCallback(OnUpdateSetEventNodeSetting));
		KFSMState currentState2 = tutorial.CurrentState;
		currentState2.OnLeave = (KFSMStateChange)Delegate.Remove(currentState2.OnLeave, new KFSMStateChange(OnLeaveSetEventNodeSetting));
		MissionEditorLogic instance = MissionEditorLogic.Instance;
		instance.OnSelectedGameObjectChange = (Action<GameObject>)Delegate.Remove(instance.OnSelectedGameObjectChange, new Action<GameObject>(OnSelectedGameObjectChangeSetEvent));
		tutorialScenario.HideAllTutorialSelectors();
		tutorialScenario.LockNodeSettings(locked: true);
		tutorialScenario.ResetNodeSettingsMask();
	}

	private void OnEnterSetEventNodeSettingHelper(KFSMState kfsmState)
	{
		KFSMState currentState = tutorial.CurrentState;
		currentState.OnUpdate = (KFSMCallback)Delegate.Combine(currentState.OnUpdate, new KFSMCallback(OnUpdateSetEventNodeSettingHelper));
		KFSMState currentState2 = tutorial.CurrentState;
		currentState2.OnLeave = (KFSMStateChange)Delegate.Combine(currentState2.OnLeave, new KFSMStateChange(OnLeaveSetEventNodeSettingHelper));
		tutorialScenario.LockNodeSettings(locked: false);
	}

	private void OnUpdateSetEventNodeSettingHelper()
	{
		if (!CanShowSetEventSelector())
		{
			tutorial.GoToNextPage();
		}
	}

	private void OnLeaveSetEventNodeSettingHelper(KFSMState kfsmState)
	{
		KFSMState currentState = tutorial.CurrentState;
		currentState.OnUpdate = (KFSMCallback)Delegate.Remove(currentState.OnUpdate, new KFSMCallback(OnUpdateSetEventNodeSettingHelper));
		KFSMState currentState2 = tutorial.CurrentState;
		currentState2.OnLeave = (KFSMStateChange)Delegate.Remove(currentState2.OnLeave, new KFSMStateChange(OnLeaveSetEventNodeSettingHelper));
		tutorialScenario.HideAllTutorialSelectors();
		tutorialScenario.LockNodeSettings(locked: true);
		tutorialScenario.ResetNodeSettingsMask();
	}
}
