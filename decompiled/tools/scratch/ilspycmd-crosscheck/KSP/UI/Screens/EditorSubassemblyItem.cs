using System.Collections;
using System.IO;
using KSP.Localization;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	public class EditorSubassemblyItem : MonoBehaviour
	{
		public TextMeshProUGUI title;

		public TextMeshProUGUI description;

		public TextMeshProUGUI partcount;

		public Button btnSpawnSubassembly;

		public Button btnDelete;

		private EditorPartList partList;

		private ShipTemplate subassembly;

		private void Awake()
		{
			btnDelete.onClick.AddListener(MouseInput_Delete);
			btnSpawnSubassembly.onClick.AddListener(MouseInput_Spawn);
		}

		public void Create(EditorPartList partList, ShipTemplate sa)
		{
			this.partList = partList;
			subassembly = sa;
			title.text = sa.shipName;
			description.text = sa.shipDescription;
			partcount.text = sa.partCount.ToString();
		}

		public void MouseInput_Spawn()
		{
			if (EditorLogic.SelectedPart == null)
			{
				partList.TapIcon(subassembly);
				return;
			}
			if (!EditorLogic.SelectedPart.isAttachable)
			{
				Debug.Log("EditorSubAssembly: Cannot save a part which is not attachable");
				return;
			}
			InputLockManager.SetControlLock(ControlTypes.EDITOR_SOFT_LOCK, "editorSubassemblyItem");
			Part subassemblyRootpart = EditorLogic.SelectedPart;
			PopupDialog.SpawnPopupDialog(new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), new MultiOptionDialog("ConfirmOverwrite", Localizer.Format("#autoLOC_129949"), Localizer.Format("#autoLOC_6004041"), HighLogic.UISkin, new DialogGUIButton(Localizer.Format("#autoLOC_129950"), delegate
			{
				SubassemblyDropZone.Instance.SaveSubassembly(subassembly.shipName, subassembly.shipDescription, subassemblyRootpart);
				OnDismiss();
			}), new DialogGUIButton(Localizer.Format("#autoLOC_129951"), OnDismiss)), persistAcrossScenes: false, HighLogic.UISkin).OnDismiss = OnDismiss;
		}

		public void MouseInput_Delete()
		{
			InputLockManager.SetControlLock(ControlTypes.EDITOR_SOFT_LOCK, "editorSubassemblyItem");
			PopupDialog.SpawnPopupDialog(new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), new MultiOptionDialog("ConfirmDelete", Localizer.Format("#autoLOC_129960"), Localizer.Format("#autoLOC_7000025"), HighLogic.UISkin, new DialogGUIButton(Localizer.Format("#autoLOC_129950"), OnDeleteConfirm), new DialogGUIButton(Localizer.Format("#autoLOC_129951"), OnDismiss)), persistAcrossScenes: false, HighLogic.UISkin).OnDismiss = OnDismiss;
		}

		private void OnDeleteConfirm()
		{
			PartCategorizer.Instance.RemoveSubassemblyFromSelectedCategory(subassembly.shipName, refresh: false);
			DeleteSubassembly();
		}

		private void OnDismiss()
		{
			InputLockManager.RemoveControlLock("editorSubassemblyItem");
			StartCoroutine(OnDismissCoroutine());
		}

		private void DeleteSubassembly()
		{
			File.Delete(subassembly.filename);
			OnDismiss();
		}

		private IEnumerator OnDismissCoroutine()
		{
			yield return null;
			yield return null;
			EditorPartList.Instance.RefreshSubassemblies();
		}
	}
}
