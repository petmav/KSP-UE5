using System;
using System.Collections.Generic;
using Expansions.Missions;
using Expansions.Missions.Editor;

public class IntermediateTutorial : METutorialScenario
{
	private const string intermediateTitleLoc = "#autoLOC_9990010";

	protected override void CreateTutorialPages()
	{
		tutorialPages = new List<TutorialPage>();
		tutorialPages.Add(AddTutorialPage("welcome", "#autoLOC_9990010", "#autoLOC_9990011", base.OnEnterEmpty));
		tutorialPages.AddRange(AddTutorialPage(new BiomeTutorialStep(this, Tutorial)));
		tutorialPages.AddRange(AddTutorialPage(new EventStartTutorialStep(this, Tutorial)));
		tutorialPages.AddRange(AddTutorialPage(new ScienceNodeTutorialStep(this, Tutorial)));
		tutorialPages.AddRange(AddTutorialPage(new EventSelectionTutorialStep(this, Tutorial)));
		tutorialPages.AddRange(AddTutorialPage(new DialogOrbitTutorialStep(this, Tutorial)));
		tutorialPages.AddRange(AddTutorialPage(new EventExplanationTutorialStep(this, Tutorial)));
		tutorialPages.AddRange(AddTutorialPage(new DialogScienceTutorialStep(this, Tutorial)));
		tutorialPages.AddRange(AddTutorialPage(new VesselTutorialStep(this, Tutorial)));
		tutorialPages.AddRange(AddTutorialPage(new TimeSinceTutorialStep(this, Tutorial)));
		tutorialPages.AddRange(AddTutorialPage(new ApplyScoreTutorialStep(this, Tutorial)));
		tutorialPages.Add(AddTutorialPage("endIntermediateTutorial", "#autoLOC_9990010", "#autoLOC_9990058", base.OnEnterEmpty, (TutorialButtonType)12));
	}

	protected override void OnTutorialSetup()
	{
		base.OnTutorialSetup();
		SetupEditorForTutorial();
	}

	private void SetupEditorForTutorial()
	{
		MissionEditorLogic.Instance.SetLock(ControlTypes.All, add: true, lockId);
		MissionEditorLogic.Instance.SetLock(ControlTypes.EDITOR_EXIT, add: false, lockId);
		MissionEditorLogic.Instance.PreventNodeDestruction = true;
		MissionEditorLogic instance = MissionEditorLogic.Instance;
		instance.OnExitEditor = (Action)Delegate.Combine(instance.OnExitEditor, new Action(OnExitCancelSave));
		MissionEditorLogic instance2 = MissionEditorLogic.Instance;
		instance2.OnLeave = (Action)Delegate.Combine(instance2.OnLeave, new Action(OnExitMissionEditorScreen));
		GameEvents.Mission.onBuilderNodeSelectionChanged.Add(OnNodeChange);
		SaveOnClickOnSearchListener();
		RemoveOnClickOnSearchListener();
	}

	private void SetupEditorToDefault()
	{
		MissionEditorLogic.Instance.SetLock(ControlTypes.All, add: false, lockId);
		MissionEditorLogic.Instance.Unlock(lockId);
		MissionEditorLogic.Instance.PreventNodeDestruction = false;
		MissionEditorLogic instance = MissionEditorLogic.Instance;
		instance.OnExitEditor = (Action)Delegate.Remove(instance.OnExitEditor, new Action(OnExitCancelSave));
		MissionEditorLogic instance2 = MissionEditorLogic.Instance;
		instance2.OnLeave = (Action)Delegate.Remove(instance2.OnLeave, new Action(OnExitMissionEditorScreen));
		RestoreOnClickOnSearchListener();
		RemoveDragHelper();
		ResetNodeFilterMask();
		ResetNodeListMask();
		ResetNodeSettingsMask();
	}

	private void OnExitCancelSave()
	{
		MissionEditorLogic instance = MissionEditorLogic.Instance;
		instance.OnExitEditor = (Action)Delegate.Remove(instance.OnExitEditor, new Action(OnExitCancelSave));
		MissionEditorHistory.Clear();
	}

	private void OnExitMissionEditorScreen()
	{
		DeleteTutorial();
		SetupEditorToDefault();
		MissionEditorLogic.Instance.Unlock(lockId);
		METutorialScenario.ShowMissionPlayDialog();
		GameEvents.Mission.onBuilderNodeSelectionChanged.Remove(OnNodeChange);
	}

	private void DeleteTutorial()
	{
		HideAllTutorialSelectors();
		EnableAllTutorialPageButtons(enable: false);
		if (Tutorial.CurrentState != null)
		{
			Tutorial.CurrentState.OnLeave(null);
			Tutorial.CurrentState.OnLeave = null;
			Tutorial.CurrentState.OnUpdate = null;
		}
	}

	protected override void OnDoneButtonClick()
	{
		complete = true;
		MissionEditorLogic.Instance.OnExit();
		base.OnDoneButtonClick();
	}

	protected override void OnContinueButtonClick()
	{
		MissionEditorLogic.Instance.Unlock(lockId);
		MissionEditorLogic.Instance.SetLock(ControlTypes.All, add: false, lockId);
		MissionEditorLogic instance = MissionEditorLogic.Instance;
		instance.OnLeave = (Action)Delegate.Remove(instance.OnLeave, new Action(OnExitMissionEditorScreen));
		base.OnContinueButtonClick();
		GoToNextTutorial();
	}

	protected override string GetNextTutorialName()
	{
		return "/AdvancedTutorial/";
	}

	public override void OnSave(ConfigNode node)
	{
		stateName = GetCurrentStateName();
		if (stateName != null)
		{
			node.AddValue("statename", stateName);
		}
		node.AddValue("complete", complete);
	}

	public override void OnLoad(ConfigNode node)
	{
		if (node.HasValue("statename"))
		{
			stateName = node.GetValue("statename");
		}
		if (node.HasValue("complete"))
		{
			complete = bool.Parse(node.GetValue("complete"));
		}
	}

	private void OnNodeChange(MENode selectedNode)
	{
		LockSapParameters((NodeDefinition)GetSelectedNodeIndex());
	}
}
