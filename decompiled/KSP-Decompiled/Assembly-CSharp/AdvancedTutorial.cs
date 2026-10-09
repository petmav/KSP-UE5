using System;
using System.Collections.Generic;
using Expansions.Missions.Editor;
using UnityEngine;
using UnityEngine.UI;

public class AdvancedTutorial : METutorialScenario
{
	public enum TutorialStep
	{
		none = -1,
		welcome,
		crewStep1_intro,
		crewStep2_clickCreateVesselNode,
		crewStep3a_clickAdd,
		crewStep3b_clickAdd,
		crewStep4a_crewEqual2,
		crewStep4b_crewEqual2,
		crewStep5_exit,
		partStep1_intro,
		partStep2a_clickRequiredPart,
		partStep2b_clickRequiredPart,
		partStep3a_selectMysteryGoo,
		partStep3b_selectMysteryGoo,
		vesselStep1_intro,
		vesselStep2a_dragVesselNode,
		vesselStep2b_dragVesselNode,
		vesselStep3a_selectVesselName,
		vesselStep3b_selectVesselName,
		vesselStep4a_dragDialogMessage,
		vesselStep4b_dragDialogMessage,
		vesselStep5a_writeMessage,
		vesselStep5b_writeMessage,
		vesselStep6a_linkVesselDestroyed,
		vesselStep6b_linkVesselDestroyed,
		vesselStep7a_drag2DialogMessage,
		vesselStep7b_drag2DialogMessage,
		vesselStep8a_linkDialogMessage,
		vesselStep8b_linkDialogMessage,
		vesselStep9a_writeMessage,
		vesselStep9b_writeMessage,
		validationStep1_intro,
		validationStep2_misisonError,
		validationStep3a_clickRedDot,
		validationStep3b_clickRedDot,
		validationStep4_markEndNode,
		validationStep5a_setAsEndNode,
		validationStep5b_setAsEndNode,
		exportingStep1_intro,
		exportingStep2a_clickExport,
		exportingStep2b_clickExport,
		exportingStep3_award,
		exportingStep4_mods,
		exportingStep5_banners,
		conclusion_exit
	}

	public new enum NodeDefinition
	{
		None = -1,
		StartNode = 0,
		CreateVessel = 1,
		VesselLandedKerbin = 8,
		VesselLandedMun = 9,
		VesselDestroyed = 10,
		DialogMessage1 = 11,
		DialogMessage2 = 12,
		DialogMessage3 = 13
	}

	private struct Typekey
	{
		public Type type;

		public string value;

		public Typekey(Type typekey, string valuekey)
		{
			type = typekey;
			value = valuekey;
		}
	}

	private const string lockIdMissionBuilder = "missionBuilder_start";

	private List<Typekey> typekeys = new List<Typekey>
	{
		new Typekey(typeof(MEGUIParameterDropdownList), "TotalCrew"),
		new Typekey(typeof(MEGUIParameterDropdownList), "Equal"),
		new Typekey(typeof(MEGUIParameterNumberRange), "2")
	};

	private int nodeCount;

	private const int HEADER_PARAMETER_GROUP = 0;

	private const int DEFAULT_PARAMETER_GROUP = 1;

	private const int MESSAGE_PARAMETER_INDEX = 1;

	private const int END_CHECKBOX_PARAMETER = 4;

	private TutorialStep currentstep;

	private string validationLockid = "missionBuilder_validation";

	private List<NodeDefinition> dialogNodes = new List<NodeDefinition>
	{
		NodeDefinition.DialogMessage1,
		NodeDefinition.DialogMessage2,
		NodeDefinition.DialogMessage3
	};

	private Dictionary<NodeDefinition, string> textNodes = new Dictionary<NodeDefinition, string>();

	private int m_TutorialShown = -1;

	protected override void OnTutorialSetup()
	{
		base.OnTutorialSetup();
		MissionEditorLogic.Instance.PreventNodeDestruction = true;
		MissionEditorLogic.Instance.Unlock("missionBuilder_start");
		MissionEditorLogic instance = MissionEditorLogic.Instance;
		instance.OnLeave = (Action)Delegate.Combine(instance.OnLeave, new Action(OnExitMissionEditorScreen));
		MissionEditorLogic instance2 = MissionEditorLogic.Instance;
		instance2.OnExitEditor = (Action)Delegate.Combine(instance2.OnExitEditor, new Action(OnExitCancelSave));
		GameEvents.Mission.onMissionBriefingCreated.Add(OnMissionBriefingShow);
		SaveOnClickOnSearchListener();
		RemoveOnClickOnSearchListener();
		LockBar();
	}

	private void OnMissionBriefingShow(MissionBriefingDialog dialog)
	{
		GameEvents.Mission.onMissionBriefingCreated.Remove(OnMissionBriefingShow);
		dialog.DisableExportButon();
	}

	protected override void CreateTutorialPages()
	{
		tutorialPages = new List<TutorialPage>
		{
			AddTutorialPage(TutorialStep.welcome.ToString(), "#autoLOC_8400509", "#autoLOC_8400510", base.OnEnterEmpty),
			AddTutorialPage(TutorialStep.crewStep1_intro.ToString(), "#autoLOC_8400509", "#autoLOC_8400511", base.OnEnterEmpty),
			AddTutorialPage(TutorialStep.crewStep2_clickCreateVesselNode.ToString(), "#autoLOC_8400509", "#autoLOC_8400512", OnEnterCrew),
			AddTutorialPage(TutorialStep.crewStep3a_clickAdd.ToString(), "#autoLOC_8400509", "#autoLOC_8400513", OnUpdateCrew),
			AddTutorialPage(TutorialStep.crewStep3b_clickAdd.ToString(), "#autoLOC_8400509", "#autoLOC_8400514", OnUpdateCrew),
			AddTutorialPage(TutorialStep.crewStep4a_crewEqual2.ToString(), "#autoLOC_8400509", "#autoLOC_8400515", OnUpdateCrew),
			AddTutorialPage(TutorialStep.crewStep4b_crewEqual2.ToString(), "#autoLOC_8400509", "#autoLOC_8400516", OnUpdateCrew),
			AddTutorialPage(TutorialStep.crewStep5_exit.ToString(), "#autoLOC_8400509", "#autoLOC_8400517", OnExitCrew),
			AddTutorialPage(TutorialStep.partStep1_intro.ToString(), "#autoLOC_8400509", "#autoLOC_8400518", base.OnEnterEmpty),
			AddTutorialPage(TutorialStep.partStep2a_clickRequiredPart.ToString(), "#autoLOC_8400509", "#autoLOC_8400519", OnUpdatePart),
			AddTutorialPage(TutorialStep.partStep2b_clickRequiredPart.ToString(), "#autoLOC_8400509", "#autoLOC_8400520", OnUpdatePart),
			AddTutorialPage(TutorialStep.partStep3a_selectMysteryGoo.ToString(), "#autoLOC_8400509", "#autoLOC_8400521", OnUpdatePart),
			AddTutorialPage(TutorialStep.partStep3b_selectMysteryGoo.ToString(), "#autoLOC_8400509", "#autoLOC_8400522", OnUpdatePart),
			AddTutorialPage(TutorialStep.vesselStep1_intro.ToString(), "#autoLOC_8400509", "#autoLOC_8400523", base.OnEnterEmpty),
			AddTutorialPage(TutorialStep.vesselStep2a_dragVesselNode.ToString(), "#autoLOC_8400509", "#autoLOC_8400524", OnEnterVessel, TutorialButtonType.NoButton),
			AddTutorialPage(TutorialStep.vesselStep2b_dragVesselNode.ToString(), "#autoLOC_8400509", "#autoLOC_8400525", OnEnterVessel, TutorialButtonType.NoButton),
			AddTutorialPage(TutorialStep.vesselStep3a_selectVesselName.ToString(), "#autoLOC_8400509", "#autoLOC_8400542", OnEnterVessel),
			AddTutorialPage(TutorialStep.vesselStep3b_selectVesselName.ToString(), "#autoLOC_8400509", "#autoLOC_8400541", OnEnterVessel),
			AddTutorialPage(TutorialStep.vesselStep4a_dragDialogMessage.ToString(), "#autoLOC_8400509", "#autoLOC_8400528", OnEnterVessel),
			AddTutorialPage(TutorialStep.vesselStep4b_dragDialogMessage.ToString(), "#autoLOC_8400509", "#autoLOC_8400529", OnEnterVessel),
			AddTutorialPage(TutorialStep.vesselStep5a_writeMessage.ToString(), "#autoLOC_8400509", "#autoLOC_8400543", OnEnterVessel),
			AddTutorialPage(TutorialStep.vesselStep5b_writeMessage.ToString(), "#autoLOC_8400509", "#autoLOC_8400544", OnEnterVessel),
			AddTutorialPage(TutorialStep.vesselStep6a_linkVesselDestroyed.ToString(), "#autoLOC_8400509", "#autoLOC_8400533", OnEnterVessel),
			AddTutorialPage(TutorialStep.vesselStep6b_linkVesselDestroyed.ToString(), "#autoLOC_8400509", "#autoLOC_8400534", OnEnterVessel),
			AddTutorialPage(TutorialStep.vesselStep7a_drag2DialogMessage.ToString(), "#autoLOC_8400509", "#autoLOC_8400535", OnEnterVessel),
			AddTutorialPage(TutorialStep.vesselStep7b_drag2DialogMessage.ToString(), "#autoLOC_8400509", "#autoLOC_8400536", OnEnterVessel),
			AddTutorialPage(TutorialStep.vesselStep8a_linkDialogMessage.ToString(), "#autoLOC_8400509", "#autoLOC_8400537", OnEnterVessel),
			AddTutorialPage(TutorialStep.vesselStep8b_linkDialogMessage.ToString(), "#autoLOC_8400509", "#autoLOC_8400538", OnEnterVessel),
			AddTutorialPage(TutorialStep.vesselStep9a_writeMessage.ToString(), "#autoLOC_8400509", "#autoLOC_8400539", OnEnterVessel),
			AddTutorialPage(TutorialStep.vesselStep9b_writeMessage.ToString(), "#autoLOC_8400509", "#autoLOC_8400540", OnEnterVessel),
			AddTutorialPage(TutorialStep.validationStep1_intro.ToString(), "#autoLOC_8400509", "#autoLOC_8400545", OnEnterValidation),
			AddTutorialPage(TutorialStep.validationStep2_misisonError.ToString(), "#autoLOC_8400509", "#autoLOC_8400546", base.OnEnterEmpty),
			AddTutorialPage(TutorialStep.validationStep3a_clickRedDot.ToString(), "#autoLOC_8400509", "#autoLOC_8400547", OnEnterValidation),
			AddTutorialPage(TutorialStep.validationStep3b_clickRedDot.ToString(), "#autoLOC_8400509", "#autoLOC_8400548", OnEnterValidation),
			AddTutorialPage(TutorialStep.validationStep4_markEndNode.ToString(), "#autoLOC_8400509", "#autoLOC_8400549", OnEnterValidation),
			AddTutorialPage(TutorialStep.validationStep5a_setAsEndNode.ToString(), "#autoLOC_8400509", "#autoLOC_8400550", OnEnterValidation),
			AddTutorialPage(TutorialStep.validationStep5b_setAsEndNode.ToString(), "#autoLOC_8400509", "#autoLOC_8400551", OnEnterValidation),
			AddTutorialPage(TutorialStep.exportingStep1_intro.ToString(), "#autoLOC_8400509", "#autoLOC_8400552", OnEnterExport),
			AddTutorialPage(TutorialStep.exportingStep2a_clickExport.ToString(), "#autoLOC_8400509", "#autoLOC_8400553", OnEnterExport),
			AddTutorialPage(TutorialStep.exportingStep2b_clickExport.ToString(), "#autoLOC_8400509", "#autoLOC_8400554", OnEnterExport),
			AddTutorialPage(TutorialStep.exportingStep3_award.ToString(), "#autoLOC_8400509", "#autoLOC_8400555", OnEnterExport),
			AddTutorialPage(TutorialStep.exportingStep4_mods.ToString(), "#autoLOC_8400509", "#autoLOC_8400556", OnEnterExport),
			AddTutorialPage(TutorialStep.exportingStep5_banners.ToString(), "#autoLOC_8400509", "#autoLOC_8400557", OnEnterExport),
			AddTutorialPage(TutorialStep.conclusion_exit.ToString(), "#autoLOC_8400509", "#autoLOC_8400532", OnEnterConclusion, TutorialButtonType.Done)
		};
	}

	private void OnExitMissionEditorScreen()
	{
		MissionEditorLogic.Instance.PreventNodeDestruction = false;
		if (Tutorial.CurrentState != null)
		{
			Tutorial.CurrentState.OnLeave(null);
			Tutorial.CurrentState.OnLeave = null;
			Tutorial.CurrentState.OnUpdate = null;
		}
		MissionEditorLogic.Instance.SetLock(ControlTypes.All, add: false, lockId);
		MissionEditorLogic.Instance.Unlock(lockId);
		RestoreOnClickOnSearchListener();
		EnableAllTutorialPageButtons(enable: false);
		MissionEditorLogic instance = MissionEditorLogic.Instance;
		instance.OnLeave = (Action)Delegate.Remove(instance.OnLeave, new Action(OnExitMissionEditorScreen));
		GameEvents.Mission.onMissionBriefingCreated.Remove(OnMissionBriefingShow);
		METutorialScenario.ShowMissionPlayDialog();
	}

	private void OnExitCancelSave()
	{
		MissionEditorLogic instance = MissionEditorLogic.Instance;
		instance.OnExitEditor = (Action)Delegate.Remove(instance.OnExitEditor, new Action(OnExitCancelSave));
		MissionEditorHistory.Clear();
	}

	private void LockBar()
	{
		MissionEditorLogic.Instance.SetLock(ControlTypes.All, add: true, lockId);
		MissionEditorLogic.Instance.SetLock(ControlTypes.EDITOR_EXIT, add: false, lockId);
	}

	private void ArrangeGraph()
	{
		MissionEditorLogic.Instance.ArrangeGraphNodes();
	}

	protected override void OnDoneButtonClick()
	{
		complete = true;
		MissionBriefingDialog.Hide(MissionEditorLogic.Instance.EditorMission);
		MissionEditorLogic.Instance.OnExit();
		base.OnDoneButtonClick();
	}

	private void OnEnterCrew(KFSMState state)
	{
		currentstep = GetCurrentTutorialStep();
		EnableAllTutorialPageButtons(enable: false);
		OnInitCreateVesel();
		SetEditorLock();
		LockNodeSettings(locked: false);
	}

	private void OnInitCreateVesel()
	{
		if (Tutorial.CurrentState is TutorialPage)
		{
			HighlightNode(1, active: true);
			MissionEditorLogic instance = MissionEditorLogic.Instance;
			instance.OnSelectedGameObjectChange = (Action<GameObject>)Delegate.Combine(instance.OnSelectedGameObjectChange, new Action<GameObject>(OnSelectedNodeChange));
		}
	}

	private void OnSelectedNodeChange(GameObject objectchange)
	{
		MEGUINode component = objectchange.GetComponent<MEGUINode>();
		if (component != null && component.Node != null)
		{
			if (component.Node.basicNodeSource == "CreateVessel")
			{
				HighlightNode(1, active: false);
				MissionEditorLogic instance = MissionEditorLogic.Instance;
				instance.OnSelectedGameObjectChange = (Action<GameObject>)Delegate.Remove(instance.OnSelectedGameObjectChange, new Action<GameObject>(OnSelectedNodeChange));
				Tutorial.GoToNextPage();
			}
			else
			{
				HighlightNode(1, active: true);
			}
		}
	}

	private void OnUpdateCrew(KFSMState state)
	{
		currentstep = GetCurrentTutorialStep();
		EnableAllTutorialPageButtons(enable: false);
		if (Tutorial.CurrentState is TutorialPage tutorialPage)
		{
			tutorialPage.OnUpdate = (KFSMCallback)Delegate.Combine(tutorialPage.OnUpdate, new KFSMCallback(OnUpdateSelectedCrewNodeChange));
			tutorialPage.OnLeave = (KFSMStateChange)Delegate.Combine(tutorialPage.OnLeave, new KFSMStateChange(OnLeaveCrew));
		}
	}

	private void OnUpdateSelectedCrewNodeChange()
	{
		if (GetSelectedNodeIndex() == 1 && Tutorial.CurrentState is TutorialPage tutorialPage)
		{
			tutorialPage.OnUpdate = (KFSMCallback)Delegate.Remove(tutorialPage.OnUpdate, new KFSMCallback(OnUpdateSelectedCrewNodeChange));
			tutorialPage.OnUpdate = (KFSMCallback)Delegate.Combine(tutorialPage.OnUpdate, new KFSMCallback(OnUpdateCrew));
		}
	}

	private void OnUpdateCrew()
	{
		if (GetSelectedNodeIndex() == 1)
		{
			int count = 1;
			if (currentstep == TutorialStep.crewStep4a_crewEqual2 || currentstep == TutorialStep.crewStep4b_crewEqual2)
			{
				count = typekeys.Count;
			}
			bool flag = IsParametersCorrect(GetCurrentNodeParameters(1), typekeys.GetRange(0, count));
			EnableAllTutorialPageButtons(flag);
			if ((flag && currentstep == TutorialStep.crewStep3b_clickAdd) || (flag && currentstep == TutorialStep.crewStep4b_crewEqual2))
			{
				Tutorial.GoToNextPage();
			}
		}
		OnUpdateCrewErrors();
	}

	private void OnExitCrew(KFSMState state)
	{
		if (Tutorial.CurrentState is TutorialPage tutorialPage)
		{
			tutorialPage.OnUpdate = (KFSMCallback)Delegate.Remove(tutorialPage.OnUpdate, new KFSMCallback(OnUpdateCrew));
		}
	}

	private void OnLeaveCrew(KFSMState state)
	{
		if (Tutorial.CurrentState is TutorialPage tutorialPage)
		{
			tutorialPage.OnUpdate = (KFSMCallback)Delegate.Remove(tutorialPage.OnUpdate, new KFSMCallback(OnUpdateSelectedCrewNodeChange));
			tutorialPage.OnLeave = (KFSMStateChange)Delegate.Remove(tutorialPage.OnLeave, new KFSMStateChange(OnLeaveCrew));
		}
	}

	private void OnUpdateCrewErrors()
	{
		if (GetSelectedNodeIndex() != 1)
		{
			MissionEditorLogic.Instance.SimulateOnNodeClick(1);
			if (currentstep == TutorialStep.crewStep3a_clickAdd || currentstep == TutorialStep.crewStep4a_crewEqual2)
			{
				Tutorial.GoToNextPage();
			}
		}
	}

	private void OnUpdatePart(KFSMState state)
	{
		currentstep = GetCurrentTutorialStep();
		EnableAllTutorialPageButtons(enable: false);
		if (Tutorial.CurrentState is TutorialPage tutorialPage)
		{
			tutorialPage.OnUpdate = (KFSMCallback)Delegate.Combine(tutorialPage.OnUpdate, new KFSMCallback(OnUpdatePart));
			tutorialPage.OnLeave = (KFSMStateChange)Delegate.Combine(tutorialPage.OnLeave, new KFSMStateChange(OnLeavePart));
		}
		OnEnterPart();
	}

	private void OnEnterPart()
	{
		if (GetSelectedNodeIndex() != 1)
		{
			return;
		}
		ShowTutorialSelection(GetPartPickerParam());
		if (currentstep == TutorialStep.partStep2b_clickRequiredPart && GetPartPickerParam().IsSelected)
		{
			Tutorial.GoToNextPage();
		}
		if (currentstep == TutorialStep.partStep3b_selectMysteryGoo)
		{
			MEGUIParameterPartPicker mEGUIParameterPartPicker = GetPartPickerParam() as MEGUIParameterPartPicker;
			bool flag = false;
			if (mEGUIParameterPartPicker != null)
			{
				flag = mEGUIParameterPartPicker.FieldValue.Count == 1 && mEGUIParameterPartPicker.FieldValue[0] == "GooExperiment";
			}
			if (flag)
			{
				Tutorial.GoToNextPage();
			}
		}
	}

	private void OnUpdatePart()
	{
		if (GetSelectedNodeIndex() == 1)
		{
			if (currentstep == TutorialStep.partStep2a_clickRequiredPart || currentstep == TutorialStep.partStep2b_clickRequiredPart)
			{
				EnableAllTutorialPageButtons(GetPartPickerParam().IsSelected);
			}
			if (currentstep == TutorialStep.partStep3a_selectMysteryGoo || currentstep == TutorialStep.partStep3b_selectMysteryGoo)
			{
				MEGUIParameterPartPicker mEGUIParameterPartPicker = GetPartPickerParam() as MEGUIParameterPartPicker;
				bool enable = false;
				if (mEGUIParameterPartPicker != null)
				{
					enable = mEGUIParameterPartPicker.FieldValue.Count == 1 && mEGUIParameterPartPicker.FieldValue[0] == "GooExperiment";
				}
				EnableAllTutorialPageButtons(enable);
			}
		}
		OnUpdatePartError();
	}

	private void OnUpdatePartError()
	{
		if (GetSelectedNodeIndex() != 1)
		{
			MissionEditorLogic.Instance.SimulateOnNodeClick(1);
			if (currentstep == TutorialStep.partStep2a_clickRequiredPart || currentstep == TutorialStep.partStep3a_selectMysteryGoo)
			{
				Tutorial.GoToNextPage();
			}
		}
		else
		{
			if (currentstep != TutorialStep.partStep3a_selectMysteryGoo && currentstep != TutorialStep.partStep3b_selectMysteryGoo)
			{
				return;
			}
			MEGUIParameterPartPicker mEGUIParameterPartPicker = GetPartPickerParam() as MEGUIParameterPartPicker;
			if (mEGUIParameterPartPicker != null && (mEGUIParameterPartPicker.FieldValue.Count > 1 || (mEGUIParameterPartPicker.FieldValue.Count == 1 && mEGUIParameterPartPicker.FieldValue[0] != "GooExperiment")))
			{
				if (currentstep == TutorialStep.partStep3a_selectMysteryGoo)
				{
					Tutorial.GoToNextPage();
				}
				mEGUIParameterPartPicker.FieldValue.Clear();
				mEGUIParameterPartPicker.RefreshUI();
				mEGUIParameterPartPicker.UpdateDisplayedParts();
			}
		}
	}

	private void OnLeavePart(KFSMState state)
	{
		if (Tutorial.CurrentState is TutorialPage tutorialPage)
		{
			tutorialPage.OnUpdate = (KFSMCallback)Delegate.Remove(tutorialPage.OnUpdate, new KFSMCallback(OnUpdatePart));
			tutorialPage.OnLeave = (KFSMStateChange)Delegate.Remove(tutorialPage.OnLeave, new KFSMStateChange(OnLeavePart));
		}
	}

	private void OnEnterVessel(KFSMState state)
	{
		currentstep = GetCurrentTutorialStep();
		EnableAllTutorialPageButtons(enable: false);
		if (Tutorial.CurrentState is TutorialPage tutorialPage)
		{
			tutorialPage.OnUpdate = (KFSMCallback)Delegate.Combine(tutorialPage.OnUpdate, new KFSMCallback(OnUpdateVessel));
			tutorialPage.OnLeave = (KFSMStateChange)Delegate.Combine(tutorialPage.OnLeave, new KFSMStateChange(OnLeaveVessel));
		}
		OnEnterVessel();
	}

	private void OnEnterVessel()
	{
		EnterVesselDrag();
		EnterVesselName();
		EnterVesselWriteMain();
		EnterVesselWriteError();
		EnterVesselLink();
		EnterVesselMessage();
	}

	private void OnUpdateVessel()
	{
		UpdateVesselDrag();
		UpdateVesselSelectName();
		UpdateVesselLink();
		UpdateVesselError();
	}

	private void UpdateVesselError()
	{
		if (currentstep != TutorialStep.vesselStep3a_selectVesselName && currentstep != TutorialStep.vesselStep3b_selectVesselName)
		{
			if (currentstep != TutorialStep.vesselStep5a_writeMessage && currentstep != TutorialStep.vesselStep5b_writeMessage)
			{
				if (currentstep != TutorialStep.vesselStep9a_writeMessage && currentstep != TutorialStep.vesselStep9b_writeMessage)
				{
					return;
				}
				int selectedNodeIndex = GetSelectedNodeIndex();
				bool flag = IsTextValueValid(NodeDefinition.DialogMessage2);
				bool flag2 = IsTextValueValid(NodeDefinition.DialogMessage3);
				if (!flag || !flag2)
				{
					int num = -1;
					if (!flag && selectedNodeIndex == 12)
					{
						num = 12;
					}
					else if (!flag2 && selectedNodeIndex == 13)
					{
						num = 13;
					}
					if (num != -1 && m_TutorialShown != num)
					{
						m_TutorialShown = num;
						GetMessageParameter((NodeDefinition)num);
					}
				}
			}
			else if (GetSelectedNodeIndex() != 11)
			{
				MissionEditorLogic.Instance.SimulateOnNodeClick(11);
				MEGUIParameterTextArea messageParameter = GetMessageParameter((NodeDefinition)GetSelectedNodeIndex());
				if (messageParameter != null)
				{
					ShowTutorialSelection(messageParameter);
				}
				if (currentstep == TutorialStep.vesselStep5a_writeMessage)
				{
					Tutorial.GoToNextPage();
				}
			}
		}
		else if (GetSelectedNodeIndex() != 10)
		{
			MissionEditorLogic.Instance.SimulateOnNodeClick(10);
			ShowTutorialSelection(GetVesselDestroyedVesselParam());
			if (currentstep == TutorialStep.vesselStep3a_selectVesselName)
			{
				Tutorial.GoToNextPage();
			}
		}
	}

	private void OnLeaveVessel(KFSMState state)
	{
		if (currentstep == TutorialStep.vesselStep2b_dragVesselNode || currentstep == TutorialStep.vesselStep4b_dragDialogMessage || currentstep == TutorialStep.vesselStep7b_drag2DialogMessage)
		{
			RemoveDrag();
			if (currentstep == TutorialStep.vesselStep2b_dragVesselNode)
			{
				MissionEditorLogic.Instance.SimulateOnNodeClick(10);
				ShowTutorialSelection(GetVesselDestroyedVesselParam());
			}
			else if (currentstep == TutorialStep.vesselStep4b_dragDialogMessage)
			{
				MissionEditorLogic.Instance.SimulateOnNodeClick(11);
				MEGUIParameterTextArea messageParameter = GetMessageParameter((NodeDefinition)GetSelectedNodeIndex());
				if (messageParameter != null)
				{
					ShowTutorialSelection(messageParameter);
				}
			}
			else if (currentstep == TutorialStep.vesselStep7b_drag2DialogMessage)
			{
				MissionEditorLogic.Instance.SimulateOnNodeClick(12);
			}
		}
		if (currentstep == TutorialStep.vesselStep5b_writeMessage || currentstep == TutorialStep.vesselStep9b_writeMessage)
		{
			List<NodeDefinition> list = GetTextNodes();
			for (int i = 0; i < list.Count; i++)
			{
				MEGUIParameterTextArea messageParameter2 = GetMessageParameter(list[i]);
				if (messageParameter2 != null)
				{
					messageParameter2.inputField.onValueChanged.RemoveListener(OnTextValueChanged);
				}
				HideTutorialSelection(messageParameter2);
			}
			textNodes.Clear();
		}
		if (currentstep == TutorialStep.vesselStep8b_linkDialogMessage)
		{
			List<int> start = new List<int>();
			List<int> end = new List<int>();
			GetLinkNodes(ref start, ref end);
			for (int j = 0; j < start.Count; j++)
			{
				GetMEGUINode(start[j]).ToggleOutputHolderHighlighter(state: false);
				GetMEGUINode(end[j]).ToggleInputHolderHighlighter(state: false);
			}
		}
		if (Tutorial.CurrentState is TutorialPage tutorialPage)
		{
			tutorialPage.OnUpdate = (KFSMCallback)Delegate.Remove(tutorialPage.OnUpdate, new KFSMCallback(OnUpdateVessel));
			tutorialPage.OnLeave = (KFSMStateChange)Delegate.Remove(tutorialPage.OnLeave, new KFSMStateChange(OnLeaveVessel));
		}
	}

	private void EnterVesselDrag()
	{
		if (IsDragMainStep())
		{
			string filterednode = "";
			string highlightCategory = "";
			if (currentstep == TutorialStep.vesselStep2a_dragVesselNode)
			{
				highlightCategory = "Vessel";
				filterednode = "Crashed";
			}
			else if (currentstep == TutorialStep.vesselStep4a_dragDialogMessage || currentstep == TutorialStep.vesselStep7a_drag2DialogMessage)
			{
				highlightCategory = "Utility";
				filterednode = "DialogMessage";
			}
			OnEnterDragStep(filterednode, highlightCategory);
		}
	}

	private void EnterVesselName()
	{
		if (currentstep == TutorialStep.vesselStep3b_selectVesselName)
		{
			MEGUIParameterVesselDropdownList vesselDropDown = GetVesselDropDown();
			if (vesselDropDown != null && vesselDropDown.FieldValue > 0)
			{
				Tutorial.GoToNextPage();
			}
		}
	}

	private void EnterVesselWriteMain()
	{
		if (!IsWriteMainStep())
		{
			return;
		}
		List<NodeDefinition> list = GetTextNodes();
		for (int i = 0; i < list.Count; i++)
		{
			MEGUIParameterTextArea messageParameter = GetMessageParameter(list[i]);
			if (messageParameter != null)
			{
				messageParameter.inputField.onValueChanged.AddListener(OnTextValueChanged);
			}
		}
		textNodes.Clear();
	}

	private void EnterVesselWriteError()
	{
		if (!IsWriteErrorStep())
		{
			return;
		}
		List<NodeDefinition> list = GetTextNodes();
		if (IsAllTextValueValid(list))
		{
			Tutorial.GoToNextPage();
			return;
		}
		for (int i = 0; i < list.Count; i++)
		{
			MEGUIParameterTextArea messageParameter = GetMessageParameter(list[i]);
			if (messageParameter != null)
			{
				messageParameter.inputField.onValueChanged.AddListener(OnTextValueChanged);
			}
		}
		textNodes.Clear();
	}

	private void EnterVesselLink()
	{
		if ((IsLinkMainStep() || IsLinkErrorStep()) && IsLinkErrorStep())
		{
			List<int> start = new List<int>();
			List<int> end = new List<int>();
			GetLinkNodes(ref start, ref end);
			if (IsAllNodeLinked(start, end, uniquestartlink: false))
			{
				Tutorial.GoToNextPage();
			}
		}
	}

	private void EnterVesselMessage()
	{
		if (currentstep != TutorialStep.vesselStep9a_writeMessage && currentstep != TutorialStep.vesselStep9b_writeMessage)
		{
			return;
		}
		List<NodeDefinition> list = GetTextNodes();
		foreach (NodeDefinition item in list)
		{
			MEGUIParameterTextArea messageParameter = GetMessageParameter(item);
			if (messageParameter != null)
			{
				textNodes[item] = messageParameter.inputField.text;
			}
		}
		if (IsAllTextValueValid(list))
		{
			EnableAllTutorialPageButtons(enable: true);
		}
	}

	private void OnEnterDragStep(string filterednode, string highlightCategory)
	{
		Func<MEGUINodeIcon, bool> filterCriteria = (MEGUINodeIcon icon) => icon.basicNode != null && icon.basicNode.name == filterednode;
		DragNodeHelper(filterCriteria, highlightCategory);
		EnableAllTutorialPageButtons(enable: false);
		OnInitCreateVesel();
		SetEditorLock();
		LockNodeSettings(locked: false);
		nodeCount = MissionEditorLogic.Instance.GetNodeListCout();
	}

	private void UpdateVesselDrag()
	{
		if (IsDragMainStep() || IsDragErrorStep())
		{
			int nodeListCout = MissionEditorLogic.Instance.GetNodeListCout();
			int num = 1;
			if (currentstep == TutorialStep.vesselStep7a_drag2DialogMessage || currentstep == TutorialStep.vesselStep7b_drag2DialogMessage)
			{
				num = 2;
			}
			if (nodeListCout == nodeCount + num && MissionEditorLogic.Instance.CurrentSelectedNode != null)
			{
				Tutorial.GoToNextPage();
			}
		}
	}

	private void UpdateVesselSelectName()
	{
		if (currentstep == TutorialStep.vesselStep3a_selectVesselName || currentstep == TutorialStep.vesselStep3b_selectVesselName)
		{
			MEGUIParameterVesselDropdownList vesselDropDown = GetVesselDropDown();
			EnableAllTutorialPageButtons(vesselDropDown != null && vesselDropDown.FieldValue > 0);
		}
	}

	private void UpdateVesselLink()
	{
		if (IsLinkMainStep() || IsLinkErrorStep())
		{
			List<int> start = new List<int>();
			List<int> end = new List<int>();
			GetLinkNodes(ref start, ref end);
			EnableAllTutorialPageButtons(IsAllNodeLinked(start, end, uniquestartlink: false));
		}
	}

	private bool IsDragMainStep()
	{
		if (currentstep != TutorialStep.vesselStep2a_dragVesselNode && currentstep != TutorialStep.vesselStep4a_dragDialogMessage)
		{
			return currentstep == TutorialStep.vesselStep7a_drag2DialogMessage;
		}
		return true;
	}

	private bool IsDragErrorStep()
	{
		if (currentstep != TutorialStep.vesselStep2b_dragVesselNode && currentstep != TutorialStep.vesselStep4b_dragDialogMessage)
		{
			return currentstep == TutorialStep.vesselStep7b_drag2DialogMessage;
		}
		return true;
	}

	private bool IsLinkMainStep()
	{
		if (currentstep != TutorialStep.vesselStep6a_linkVesselDestroyed)
		{
			return currentstep == TutorialStep.vesselStep8a_linkDialogMessage;
		}
		return true;
	}

	private bool IsLinkErrorStep()
	{
		if (currentstep != TutorialStep.vesselStep6b_linkVesselDestroyed)
		{
			return currentstep == TutorialStep.vesselStep8b_linkDialogMessage;
		}
		return true;
	}

	private bool IsWriteMainStep()
	{
		if (currentstep != TutorialStep.vesselStep5a_writeMessage)
		{
			return currentstep == TutorialStep.vesselStep9a_writeMessage;
		}
		return true;
	}

	private bool IsWriteErrorStep()
	{
		if (currentstep != TutorialStep.vesselStep5b_writeMessage)
		{
			return currentstep == TutorialStep.vesselStep9b_writeMessage;
		}
		return true;
	}

	private void OnSideBarNodeFiltered()
	{
		MissionEditorLogic.Instance.HighLightDisplayedNode(isActive: true);
	}

	private MEGUIParameterVesselDropdownList GetVesselDropDown()
	{
		return GetVesselDestroyedVesselParam() as MEGUIParameterVesselDropdownList;
	}

	private MEGUIParameterTextArea GetMessageParameter(NodeDefinition node)
	{
		int selectedNodeIndex = GetSelectedNodeIndex();
		MissionEditorLogic.Instance.SimulateOnNodeClick((int)node);
		MEGUIParameter[] currentNodeParameters = GetCurrentNodeParameters(1);
		MissionEditorLogic.Instance.SimulateOnNodeClick(selectedNodeIndex);
		if (1 >= currentNodeParameters.Length)
		{
			return null;
		}
		ShowTutorialSelection(currentNodeParameters[1]);
		return currentNodeParameters[1] as MEGUIParameterTextArea;
	}

	private void OnTextValueChanged(string newname)
	{
		NodeDefinition selectedNodeIndex = (NodeDefinition)GetSelectedNodeIndex();
		textNodes[selectedNodeIndex] = newname;
		EnableAllTutorialPageButtons(IsAllTextValueValid(GetTextNodes()));
	}

	private List<NodeDefinition> GetTextNodes()
	{
		List<NodeDefinition> list = new List<NodeDefinition>();
		if (currentstep != TutorialStep.vesselStep5a_writeMessage && currentstep != TutorialStep.vesselStep5b_writeMessage)
		{
			if (currentstep == TutorialStep.vesselStep9a_writeMessage || currentstep == TutorialStep.vesselStep9b_writeMessage)
			{
				list.Add(NodeDefinition.DialogMessage2);
				list.Add(NodeDefinition.DialogMessage3);
			}
		}
		else
		{
			list.Add(NodeDefinition.DialogMessage1);
		}
		return list;
	}

	private void GetLinkNodes(ref List<int> start, ref List<int> end)
	{
		if (currentstep != TutorialStep.vesselStep6a_linkVesselDestroyed && currentstep != TutorialStep.vesselStep6b_linkVesselDestroyed)
		{
			if ((currentstep == TutorialStep.vesselStep8a_linkDialogMessage) | (currentstep == TutorialStep.vesselStep8b_linkDialogMessage))
			{
				start.Add(8);
				start.Add(9);
				end.Add(12);
				end.Add(13);
			}
		}
		else
		{
			start.Add(10);
			end.Add(11);
		}
	}

	private bool IsAllTextValueValid(List<NodeDefinition> nodes)
	{
		int num = 0;
		int num2 = 0;
		while (true)
		{
			if (num2 < nodes.Count)
			{
				NodeDefinition key = nodes[num2];
				if (textNodes.ContainsKey(key) && textNodes[key].Length > 0)
				{
					num++;
					if (num == nodes.Count)
					{
						break;
					}
				}
				num2++;
				continue;
			}
			return false;
		}
		return true;
	}

	private bool IsTextValueValid(NodeDefinition node)
	{
		if (textNodes.ContainsKey(node))
		{
			return textNodes[node].Length > 0;
		}
		return false;
	}

	private void OnEnterValidation(KFSMState state)
	{
		currentstep = GetCurrentTutorialStep();
		EnableAllTutorialPageButtons(enable: false);
		if (Tutorial.CurrentState is TutorialPage tutorialPage)
		{
			tutorialPage.OnUpdate = (KFSMCallback)Delegate.Combine(tutorialPage.OnUpdate, new KFSMCallback(OnUpdateValidation));
			tutorialPage.OnLeave = (KFSMStateChange)Delegate.Combine(tutorialPage.OnLeave, new KFSMStateChange(OnLeaveValidation));
		}
		OnEnterValidation();
	}

	private void OnEnterValidation()
	{
		if (currentstep == TutorialStep.validationStep1_intro)
		{
			EnableAllTutorialPageButtons(enable: true);
			MissionEditorLogic.Instance.RunValidator();
		}
		if (currentstep == TutorialStep.validationStep3b_clickRedDot && IsValidationDialogOpen())
		{
			Tutorial.GoToNextPage();
		}
		if (currentstep == TutorialStep.validationStep4_markEndNode || currentstep == TutorialStep.validationStep5a_setAsEndNode || currentstep == TutorialStep.validationStep5b_setAsEndNode)
		{
			if (currentstep == TutorialStep.validationStep4_markEndNode)
			{
				EnableAllTutorialPageButtons(enable: true);
			}
			if (currentstep == TutorialStep.validationStep5a_setAsEndNode || currentstep == TutorialStep.validationStep5b_setAsEndNode)
			{
				AddEndNodeListener();
				MissionEditorLogic.Instance.SimulateOnNodeClick(11);
				if (IsAllDialogNodeEnd() && currentstep == TutorialStep.validationStep5b_setAsEndNode)
				{
					Tutorial.GoToNextPage();
				}
			}
			SetPagePositionTopLeft();
		}
		if (currentstep == TutorialStep.validationStep3a_clickRedDot || currentstep == TutorialStep.validationStep3b_clickRedDot)
		{
			RemoveDrag();
			MissionEditorLogic.Instance.SetLock(ControlTypes.EDITOR_EDIT_NAME_FIELDS, add: false, lockId);
		}
		if (currentstep == TutorialStep.validationStep5a_setAsEndNode)
		{
			MissionEditorLogic.Instance.SetLock(ControlTypes.EDITOR_EDIT_NAME_FIELDS, add: true, lockId);
		}
	}

	private void OnUpdateValidation()
	{
		UpdateValidationClickReport();
		UpdateValidationError();
	}

	private void UpdateValidationError()
	{
		if (currentstep == TutorialStep.validationStep5a_setAsEndNode)
		{
			NodeDefinition selectedNodeIndex = (NodeDefinition)GetSelectedNodeIndex();
			if (selectedNodeIndex != NodeDefinition.DialogMessage1 || selectedNodeIndex != NodeDefinition.DialogMessage2 || selectedNodeIndex != NodeDefinition.DialogMessage3)
			{
				Tutorial.GoToNextPage();
			}
		}
	}

	private void OnLeaveValidation(KFSMState state)
	{
		if (currentstep == TutorialStep.validationStep5b_setAsEndNode)
		{
			HighlightNode(11, active: false);
			HighlightNode(12, active: false);
			HighlightNode(13, active: false);
			for (int i = 0; i < dialogNodes.Count; i++)
			{
				MEGUIParameterCheckbox nodeEndParameter = GetNodeEndParameter(dialogNodes[i]);
				if (nodeEndParameter != null)
				{
					nodeEndParameter.toggle.onValueChanged.RemoveListener(OnCheckboxValueChanged);
				}
			}
		}
		if (Tutorial.CurrentState is TutorialPage tutorialPage)
		{
			tutorialPage.OnUpdate = (KFSMCallback)Delegate.Remove(tutorialPage.OnUpdate, new KFSMCallback(OnUpdateValidation));
			tutorialPage.OnLeave = (KFSMStateChange)Delegate.Remove(tutorialPage.OnLeave, new KFSMStateChange(OnLeaveValidation));
		}
	}

	private void UpdateValidationClickReport()
	{
		if (currentstep == TutorialStep.validationStep3a_clickRedDot || currentstep == TutorialStep.validationStep3b_clickRedDot)
		{
			MissionEditorLogic.Instance.buttonTS.interactable = false;
			if (IsValidationDialogOpen())
			{
				Tutorial.GoToNextPage();
			}
		}
	}

	private void AddEndNodeListener()
	{
		HighlightNode(11, active: true);
		HighlightNode(12, active: true);
		HighlightNode(13, active: true);
		for (int i = 0; i < dialogNodes.Count; i++)
		{
			MEGUIParameterCheckbox nodeEndParameter = GetNodeEndParameter(dialogNodes[i]);
			if (nodeEndParameter != null)
			{
				nodeEndParameter.toggle.onValueChanged.AddListener(OnCheckboxValueChanged);
			}
		}
	}

	private void OnCheckboxValueChanged(bool newvalue)
	{
		EnableAllTutorialPageButtons(IsAllDialogNodeEnd());
	}

	private bool IsAllDialogNodeEnd()
	{
		int num = 0;
		int num2 = 0;
		while (true)
		{
			if (num2 < dialogNodes.Count)
			{
				MEGUIParameterCheckbox nodeEndParameter = GetNodeEndParameter(dialogNodes[num2]);
				if (!(nodeEndParameter != null) || !nodeEndParameter.toggle.isOn)
				{
					break;
				}
				num++;
				if (num != dialogNodes.Count)
				{
					num2++;
					continue;
				}
				return true;
			}
			return false;
		}
		return false;
	}

	private MEGUIParameterCheckbox GetNodeEndParameter(NodeDefinition node)
	{
		int selectedNodeIndex = GetSelectedNodeIndex();
		MissionEditorLogic.Instance.SimulateOnNodeClick((int)node);
		MEGUIParameter[] currentNodeParameters = GetCurrentNodeParameters(0);
		MissionEditorLogic.Instance.SimulateOnNodeClick(selectedNodeIndex);
		if (4 > currentNodeParameters.Length - 1)
		{
			return null;
		}
		ShowTutorialSelection(currentNodeParameters[4]);
		return currentNodeParameters[4] as MEGUIParameterCheckbox;
	}

	private bool IsValidationDialogOpen()
	{
		return (InputLockManager.GetControlLock(validationLockid) & ControlTypes.EDITOR_UI_TOPRIGHT) == ControlTypes.EDITOR_UI_TOPRIGHT;
	}

	private void OnEnterExport(KFSMState state)
	{
		currentstep = GetCurrentTutorialStep();
		EnableAllTutorialPageButtons(enable: false);
		if (Tutorial.CurrentState is TutorialPage tutorialPage)
		{
			tutorialPage.OnUpdate = (KFSMCallback)Delegate.Combine(tutorialPage.OnUpdate, new KFSMCallback(OnUpdateExport));
			tutorialPage.OnLeave = (KFSMStateChange)Delegate.Combine(tutorialPage.OnLeave, new KFSMStateChange(OnLeaveExport));
		}
		OnEnterExport();
	}

	private void OnEnterExport()
	{
		MissionEditorLogic.Instance.SetLock(ControlTypes.EDITOR_LAUNCH, add: false, lockId);
		HighlightTitleBarExportButton(enable: true);
		SetPagePositionTopLeft();
		if (currentstep == TutorialStep.exportingStep3_award || currentstep == TutorialStep.exportingStep4_mods || currentstep == TutorialStep.exportingStep5_banners)
		{
			EnableAllTutorialPageButtons(enable: true);
		}
	}

	private void OnLeaveExport(KFSMState state)
	{
		HighlightTitleBarExportButton(enable: false);
		if (Tutorial.CurrentState is TutorialPage tutorialPage)
		{
			tutorialPage.OnUpdate = (KFSMCallback)Delegate.Remove(tutorialPage.OnUpdate, new KFSMCallback(OnUpdateExport));
			tutorialPage.OnLeave = (KFSMStateChange)Delegate.Remove(tutorialPage.OnLeave, new KFSMStateChange(OnLeaveExport));
		}
	}

	private void OnUpdateExport()
	{
		if ((currentstep == TutorialStep.exportingStep1_intro || currentstep == TutorialStep.exportingStep2a_clickExport || currentstep == TutorialStep.exportingStep2b_clickExport) && IsExportDialogOpen())
		{
			Tutorial.GoToNextPage();
		}
		SetTestButtonInteractibility(interactable: false);
	}

	private void HighlightTitleBarExportButton(bool enable)
	{
		Button buttonExport = MissionEditorLogic.Instance.buttonExport;
		if ((bool)buttonExport && (bool)buttonExport.GetComponent<ButtonHighlighter>())
		{
			buttonExport.GetComponentInChildren<ButtonHighlighter>().Enable(enable);
		}
	}

	private bool IsExportDialogOpen()
	{
		return (InputLockManager.GetControlLock("missionBuilder_export") & ControlTypes.EDITOR_UI_TOPRIGHT) == ControlTypes.EDITOR_UI_TOPRIGHT;
	}

	private void SetTestButtonInteractibility(bool interactable)
	{
		MissionEditorLogic.Instance.buttonTest.interactable = interactable;
	}

	private void OnEnterConclusion(KFSMState aState)
	{
		MissionEditorLogic.Instance.SetLock(ControlTypes.EDITOR_LAUNCH, add: true, lockId);
	}

	private MEGUIParameter[] GetCurrentNodeParameters(int parametergroup)
	{
		Transform contentRoot = MissionEditorLogic.Instance.actionPane.SAPPanel.ContentRoot;
		int childCount = contentRoot.childCount;
		List<MEGUIParameterGroup> list = new List<MEGUIParameterGroup>();
		for (int i = 0; i < childCount; i++)
		{
			Transform child = contentRoot.GetChild(i);
			if (child != null)
			{
				MEGUIParameterGroup component = child.GetComponent<MEGUIParameterGroup>();
				if (component != null)
				{
					list.Add(component);
				}
			}
		}
		if (parametergroup >= list.Count)
		{
			return null;
		}
		return list[parametergroup].containerChilden.GetComponentsInChildren<MEGUIParameter>();
	}

	private bool IsParametersCorrect(MEGUIParameter[] baseParameters, List<Typekey> parameters)
	{
		int num = 0;
		if (baseParameters == null)
		{
			return false;
		}
		for (int i = 0; i < baseParameters.Length; i++)
		{
			if (IsParameterValid(baseParameters[i], parameters[num].type, parameters[num].value))
			{
				num++;
				if (num == parameters.Count)
				{
					break;
				}
			}
			else if (num > 0)
			{
				num = 0;
			}
		}
		return num == parameters.Count;
	}

	private bool IsParameterValid(MEGUIParameter parameter, Type type, string value)
	{
		if (!(parameter == null) && !(type == null))
		{
			if (type.IsAssignableFrom(parameter.GetType()))
			{
				if (type == typeof(MEGUIParameterDropdownList))
				{
					return (parameter as MEGUIParameterDropdownList).SelectedValue.ToString().Equals(value);
				}
				if (type == typeof(MEGUIParameterNumberRange))
				{
					return (parameter as MEGUIParameterNumberRange).valueText.text.Equals(value);
				}
			}
			return false;
		}
		return false;
	}

	private TutorialStep GetCurrentTutorialStep()
	{
		TutorialPage tutorialPage = Tutorial.CurrentState as TutorialPage;
		int num = 0;
		while (true)
		{
			if (num < Tutorial.pages.Count)
			{
				if (tutorialPage == Tutorial.pages[num])
				{
					break;
				}
				num++;
				continue;
			}
			return TutorialStep.none;
		}
		return (TutorialStep)num;
	}

	private void SetPagePositionTopLeft()
	{
		if (Tutorial.CurrentState is TutorialPage tutorialPage)
		{
			tutorialPage.dialog.dialogRect.x = 0f;
			tutorialPage.dialog.dialogRect.y = 1f;
		}
	}

	private void RemoveDrag()
	{
		OnEnterDragStep("", "");
		RemoveDragHelper();
		LockNodeSettings(locked: false);
	}
}
