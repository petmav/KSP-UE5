using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using Expansions.Missions.Flow;
using Expansions.Missions.Runtime;
using KSP.Localization;
using KSP.UI;
using KSP.UI.Util;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace Expansions.Missions.Editor
{
	public class CheckpointBrowserDialog : MonoBehaviour
	{
		public delegate void SelectFileCallback(string fullPath);

		public delegate void CancelledCallback();

		[SerializeField]
		private TextMeshProUGUI headerTitle;

		public CheckpointEntry mePrefab;

		protected string title;

		public SelectFileCallback OnFileSelected;

		public CancelledCallback OnBrowseCancelled;

		[SerializeField]
		private Button btnCancel;

		[SerializeField]
		private Button btnTest;

		[SerializeField]
		private Button btnReset;

		[SerializeField]
		private Button btnRemoveDirty;

		[SerializeField]
		private ToggleGroup listGroup;

		[SerializeField]
		private RectTransform listContainer;

		[SerializeField]
		private ScrollRect scrollRect;

		[SerializeField]
		protected UISkinDefSO uiSkin;

		protected UISkinDef skin;

		protected List<CheckpointEntry> checkpointList;

		protected CheckpointEntry selectedEntry;

		public CheckpointBrowserDialog Spawn(SelectFileCallback onFileSelected, CancelledCallback onCancel)
		{
			MissionImporting.ImportNewZips();
			CheckpointBrowserDialog component = UnityEngine.Object.Instantiate(this).GetComponent<CheckpointBrowserDialog>();
			component.transform.SetParent(DialogCanvasUtil.DialogCanvasRect, worldPositionStays: false);
			component.OnBrowseCancelled = onCancel;
			component.OnFileSelected = onFileSelected;
			component.title = Localizer.Format("#autoLOC_8003160");
			return component;
		}

		public void Start()
		{
			skin = uiSkin.SkinDef;
			headerTitle.text = title;
			btnTest.onClick.AddListener(OnButtonTest);
			btnCancel.onClick.AddListener(OnButtonCancel);
			btnReset.onClick.AddListener(OnButtonReset);
			btnRemoveDirty.onClick.AddListener(OnButtonRemoveDirty);
			GameEvents.onInputLocksModified.Add(OnInputLocksModified);
			MEFlowParser.ParseMission(MissionEditorLogic.Instance.EditorMission);
			BuildCheckpointList();
			if (checkpointList.Count == 0)
			{
				btnTest.interactable = true;
			}
		}

		protected void OnButtonCancel()
		{
			OnBrowseCancelled();
			Dismiss();
		}

		protected void OnButtonTest()
		{
			TestSelectedCheckpoint();
		}

		protected void TestSelectedCheckpoint()
		{
			if (selectedEntry != null)
			{
				if (!selectedEntry.isValid)
				{
					Hide();
					PopupDialog.SpawnPopupDialog(new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), new MultiOptionDialog("TestMissionCheckpoint", Localizer.Format("#autoLOC_8003162"), Localizer.Format("#autoLOC_8003161"), UISkinManager.GetSkin("KSP window 7"), 350f, new DialogGUIButton(Localizer.Format("#autoLOC_226975"), delegate
					{
						Show();
					})), persistAcrossScenes: false, UISkinManager.GetSkin("KSP window 7")).OnDismiss = Show;
				}
				else if (selectedEntry.isCheckpointDirty)
				{
					Hide();
					PopupDialog.SpawnPopupDialog(new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), new MultiOptionDialog("TestMissionCheckpoint", Localizer.Format("#autoLOC_8003164"), Localizer.Format("#autoLOC_8003163"), UISkinManager.GetSkin("KSP window 7"), 350f, new DialogGUIButton(Localizer.Format("#autoLOC_226975"), delegate
					{
						Show();
					})), persistAcrossScenes: false, UISkinManager.GetSkin("KSP window 7")).OnDismiss = Show;
				}
				else
				{
					InjectMissionDataIntoCheckpoint();
					OnFileSelected(selectedEntry.fullFilePath);
					Dismiss();
				}
			}
			else
			{
				OnFileSelected(null);
				Dismiss();
			}
		}

		protected void InjectMissionDataIntoCheckpoint()
		{
			selectedEntry.LoadMission();
			ProtoScenarioModule protoScenarioModule = null;
			int i = 0;
			for (int count = selectedEntry.CheckpointGame.scenarios.Count; i < count; i++)
			{
				if (selectedEntry.CheckpointGame.scenarios[i].moduleName == "MissionSystem")
				{
					protoScenarioModule = selectedEntry.CheckpointGame.scenarios[i];
					break;
				}
			}
			if (protoScenarioModule != null)
			{
				ConfigNode data = protoScenarioModule.GetData();
				if (data != null)
				{
					ConfigNode node = null;
					ConfigNode node2 = null;
					if (data.TryGetNode("MISSIONS", ref node) && node.TryGetNode("MISSION", ref node2))
					{
						node2.ClearData();
						Mission editorMission = MissionEditorLogic.Instance.EditorMission;
						editorMission.Save(node2);
						node2.SetValue("activeNodeID", selectedEntry.CheckpointMission.activeNode.id.ToString());
						node2.SetValue("isStarted", newValue: true);
						selectedEntry.CheckpointMission.situation.SaveVesselsToBuild(node2.GetNode("SITUATION"));
						ConfigNode node3 = new ConfigNode();
						if (node2.TryGetNode("NODES", ref node3))
						{
							node3.ClearData();
							int j = 0;
							for (int count2 = editorMission.nodes.Count; j < count2; j++)
							{
								ConfigNode configNode = node3.AddNode("NODE");
								MENode mENode = editorMission.nodes.At(j);
								if (selectedEntry.CheckpointMission.nodes.ContainsKey(mENode.id))
								{
									MENode mENode2 = selectedEntry.CheckpointMission.nodes[mENode.id];
									if (!mENode2.HasBeenActivated && !mENode2.isStartNode && !mENode2.IsDockedToStartNode)
									{
										mENode.Save(configNode);
										continue;
									}
									mENode2.Save(configNode);
									if (!mENode2.Title.Equals(mENode.Title))
									{
										configNode.SetValue("title", mENode.Title);
									}
								}
								else
								{
									mENode.Save(configNode);
								}
							}
						}
					}
				}
			}
			ConfigNode configNode2 = new ConfigNode();
			selectedEntry.CheckpointGame.Save(configNode2);
			configNode2.Save(selectedEntry.fullFilePath);
		}

		protected void OnButtonReset()
		{
			Hide();
			PopupDialog.SpawnPopupDialog(new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), new MultiOptionDialog("TestMissionOverwrite", Localizer.Format("#autoLOC_8003157"), Localizer.Format("#autoLOC_8003156"), UISkinManager.GetSkin("KSP window 7"), 350f, new DialogGUIButton(Localizer.Format("#autoLOC_8003165"), OnResetConfirm), new DialogGUIButton(Localizer.Format("#autoLOC_226976"), delegate
			{
				Show();
			})), persistAcrossScenes: false, UISkinManager.GetSkin("KSP window 7")).OnDismiss = Show;
		}

		protected void OnResetConfirm()
		{
			int i = 0;
			for (int count = checkpointList.Count; i < count; i++)
			{
				DeleteEntry(checkpointList[i]);
			}
			OnFileSelected(null);
			Dismiss();
		}

		protected void OnButtonRemoveDirty()
		{
			Lock("removeDirtyLock");
			PopupDialog.SpawnPopupDialog(new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), new MultiOptionDialog("TestMissionDelete", Localizer.Format("#autoLOC_8003158"), Localizer.Format("#autoLOC_8003159"), UISkinManager.GetSkin("KSP window 7"), 350f, new DialogGUIButton("<color=orange>" + Localizer.Format("#autoLOC_129950") + "</color>", OnRemoveDirtyConfirm), new DialogGUIButton(Localizer.Format("#autoLOC_226976"), OnRemoveDirtyDismiss)), persistAcrossScenes: false, UISkinManager.GetSkin("KSP window 7")).OnDismiss = OnRemoveDirtyDismiss;
		}

		private void OnRemoveDirtyConfirm()
		{
			int i = 0;
			for (int count = checkpointList.Count; i < count; i++)
			{
				if (checkpointList[i].isCheckpointDirty)
				{
					DeleteEntry(checkpointList[i]);
				}
			}
			BuildCheckpointList();
			selectedEntry = null;
			OnSelectionChanged(null);
			OnRemoveDirtyDismiss();
		}

		private void OnRemoveDirtyDismiss()
		{
			Unlock("removeDirtyLock");
		}

		protected void Show()
		{
			base.gameObject.SetActive(value: true);
		}

		protected void Hide()
		{
			base.gameObject.SetActive(value: false);
		}

		public void Dismiss()
		{
			MissionSystem.RemoveMissionObjects();
			GameEvents.onInputLocksModified.Remove(OnInputLocksModified);
			UnityEngine.Object.Destroy(base.gameObject);
		}

		private void BuildCheckpointList()
		{
			ClearMissionList();
			checkpointList = new List<CheckpointEntry>();
			Mission editorMission = MissionEditorLogic.Instance.EditorMission;
			if (editorMission.historyId == Guid.Empty)
			{
				editorMission.historyId = Guid.NewGuid();
			}
			string[] files = Directory.GetFiles(editorMission.MissionInfo.SaveFolderPath, "*.sfs");
			int i = 0;
			for (int num = files.Length; i < num; i++)
			{
				if (!files[i].EndsWith("persistent.sfs") && Path.GetFileNameWithoutExtension(files[i]).StartsWith("checkpoint_"))
				{
					CheckpointEntry item = mePrefab.Create(files[i], OnEntrySelected, OnDeleteEntry);
					checkpointList.Add(item);
				}
			}
			checkpointList.Sort(MissionCompare);
			int j = 0;
			for (int count = checkpointList.Count; j < count; j++)
			{
				AddMissionEntryWidget(checkpointList[j], listContainer);
			}
			StartCoroutine(ValidateCheckpoints());
		}

		private IEnumerator ValidateCheckpoints()
		{
			int i = 0;
			int cC = checkpointList.Count;
			while (i < cC)
			{
				checkpointList[i].Validate();
				yield return null;
				int num = i + 1;
				i = num;
			}
		}

		private static int MissionCompare(CheckpointEntry a, CheckpointEntry b)
		{
			if (a.CheckpointMeta.UT > b.CheckpointMeta.UT)
			{
				return 1;
			}
			if (a.CheckpointMeta.UT < b.CheckpointMeta.UT)
			{
				return -1;
			}
			return 0;
		}

		protected void AddMissionEntryWidget(CheckpointEntry entry, RectTransform listParent)
		{
			listGroup.RegisterToggle(entry.Toggle);
			entry.Toggle.group = listGroup;
			entry.transform.SetParent(listParent, worldPositionStays: false);
		}

		protected void ClearMissionList()
		{
			if (checkpointList != null)
			{
				int count = checkpointList.Count;
				while (count-- > 0)
				{
					CheckpointEntry checkpointEntry = checkpointList[count];
					listGroup.UnregisterToggle(checkpointEntry.Toggle);
					checkpointEntry.Terminate();
				}
				checkpointList.Clear();
				selectedEntry = null;
			}
			OnSelectionChanged(null);
		}

		protected void ClearSelection()
		{
			selectedEntry = null;
			OnSelectionChanged(null);
			listGroup.SetAllTogglesOff();
		}

		protected void OnSelectionChanged(CheckpointEntry selectedEntry)
		{
			if (selectedEntry != null)
			{
				btnTest.interactable = true;
			}
			else
			{
				btnTest.interactable = false;
			}
		}

		protected void OnEntrySelected(CheckpointEntry entry)
		{
			OnSelectionChanged(entry);
			if (selectedEntry == entry && Mouse.Left.GetDoubleClick(isDelegate: true))
			{
				TestSelectedCheckpoint();
			}
			else
			{
				selectedEntry = entry;
			}
		}

		protected void OnDeleteEntry(CheckpointEntry entry)
		{
			Lock("deleteEntryLock");
			entry.Toggle.isOn = true;
			PopupDialog.SpawnPopupDialog(new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), new MultiOptionDialog("TestMissionDelete", Localizer.Format("#autoLOC_8003086"), Localizer.Format("#autoLOC_8003085"), UISkinManager.GetSkin("KSP window 7"), 350f, new DialogGUIButton("<color=orange>" + Localizer.Format("#autoLOC_129950") + "</color>", OnDeleteConfirm), new DialogGUIButton(Localizer.Format("#autoLOC_464291"), OnDeleteDismiss)), persistAcrossScenes: false, UISkinManager.GetSkin("KSP window 7")).OnDismiss = OnDeleteDismiss;
		}

		private void OnDeleteConfirm()
		{
			DeleteEntry(selectedEntry);
			BuildCheckpointList();
			selectedEntry = null;
			OnSelectionChanged(null);
			OnDeleteDismiss();
			if (checkpointList.Count == 0)
			{
				btnTest.interactable = true;
			}
		}

		private void OnDeleteDismiss()
		{
			Unlock("deleteEntryLock");
		}

		protected void DeleteEntry(CheckpointEntry entry)
		{
			File.Delete(entry.fullFilePath);
			File.Delete(entry.fullFilePath.Replace(".sfs", ".loadmeta"));
		}

		private void OnInputLocksModified(GameEvents.FromToAction<ControlTypes, ControlTypes> ct)
		{
			UpdateUI();
		}

		public void UpdateUI()
		{
			bool flag = InputLockManager.IsUnlocked(ControlTypes.UI_DIALOGS);
			btnCancel.interactable = flag;
			if (checkpointList.Count == 0)
			{
				btnTest.interactable = true;
			}
			else
			{
				btnTest.interactable = selectedEntry != null && flag;
			}
			btnReset.interactable = flag;
			btnRemoveDirty.interactable = flag;
			scrollRect.enabled = flag;
			int i = 0;
			for (int count = checkpointList.Count; i < count; i++)
			{
				checkpointList[i].Toggle.interactable = flag;
				checkpointList[i].btnRemove.interactable = flag;
				checkpointList[i].GetComponent<UIHoverPanel>().hoverEnabled = flag;
				if (flag)
				{
					checkpointList[i].btnRemove.gameObject.SetActive(value: false);
				}
			}
		}

		protected void Lock(string lockID)
		{
			InputLockManager.SetControlLock(ControlTypes.UI_DIALOGS, lockID);
		}

		protected void Unlock(string lockID)
		{
			InputLockManager.RemoveControlLock(lockID);
		}
	}
}
