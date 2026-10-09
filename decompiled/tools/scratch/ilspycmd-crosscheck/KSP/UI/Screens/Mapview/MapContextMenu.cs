using System.Collections;
using KSP.Localization;
using KSP.UI.Screens.Mapview.MapContextMenuOptions;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace KSP.UI.Screens.Mapview
{
	public class MapContextMenu
	{
		private string header;

		private PopupDialog dialog;

		private MapContextMenuOption[] options;

		internal readonly IScreenCaster sc;

		private WaitForEndOfFrame updateYield;

		private RectTransform dialogTrf;

		private Callback onDismiss;

		public string Header
		{
			get
			{
				return header;
			}
			set
			{
				header = value;
			}
		}

		public bool Hover => dialog.Hover;

		public MapContextMenu(string header, IScreenCaster sc, Callback onDismiss, MapContextMenuOption[] options)
		{
			this.sc = sc;
			this.header = Localizer.Format("#autoLOC_7001301", header);
			this.onDismiss = onDismiss;
			dialog = PopupDialog.SpawnPopupDialog(Vector2.zero, Vector2.zero, new MultiOptionDialog("MapContextMenu", string.Empty, header, MapView.OrbitIconsTextSkinDef, options), persistAcrossScenes: false, MapView.OrbitIconsTextSkinDef);
			dialog.SetDraggable(draggable: false);
			dialog.onDestroy.AddListener(onDialogDismiss);
			dialog.StartCoroutine(dialogUpdateCoroutine());
			SetupTransform();
		}

		public MapContextMenu(string header, Rect rct, IScreenCaster sc, Callback onDismiss, MapContextMenuOption[] options)
		{
			this.sc = sc;
			this.header = Localizer.Format("#autoLOC_7001301", header);
			this.onDismiss = onDismiss;
			dialog = PopupDialog.SpawnPopupDialog(Vector2.zero, Vector2.zero, new MultiOptionDialog("MapContextMenu", string.Empty, header, MapView.OrbitIconsTextSkinDef, rct, options), persistAcrossScenes: false, MapView.OrbitIconsTextSkinDef);
			dialog.SetDraggable(draggable: false);
			dialog.onDestroy.AddListener(onDialogDismiss);
			dialog.StartCoroutine(dialogUpdateCoroutine());
			SetupTransform();
		}

		public static MapContextMenu Create(string header, IScreenCaster sc, Callback onDismiss, params MapContextMenuOption[] options)
		{
			return new MapContextMenu(header, sc, onDismiss, options);
		}

		public static MapContextMenu Create(string header, Rect rct, IScreenCaster sc, Callback onDismiss, params MapContextMenuOption[] options)
		{
			return new MapContextMenu(header, rct, sc, onDismiss, options);
		}

		private void SetupTransform()
		{
			if (dialog == null)
			{
				return;
			}
			dialogTrf = dialog.GetComponent<RectTransform>();
			if (dialogTrf == null)
			{
				return;
			}
			LayoutRebuilder.ForceRebuildLayoutImmediate(dialogTrf);
			if (sc != null)
			{
				Vector3 vector = CanvasUtil.ScreenToUISpacePos(sc.GetScreenSpacePoint(), MapViewCanvasUtil.MapViewCanvasRect, out var zPositive);
				if (zPositive)
				{
					vector = CanvasUtil.AnchorOffset(vector, dialogTrf, Vector2.down);
					dialogTrf.localPosition = vector;
				}
			}
		}

		public void Dismiss()
		{
			if (dialog != null)
			{
				dialog.Dismiss(KeepMouseState: true);
			}
		}

		protected void onDialogDismiss()
		{
			if (onDismiss != null)
			{
				onDismiss();
				InputLockManager.RemoveControlLock("MapContextMenu_" + GetHashCode());
			}
		}

		private IEnumerator dialogUpdateCoroutine()
		{
			updateYield = new WaitForEndOfFrame();
			InputLockManager.SetControlLock(ControlTypes.ALLBUTCAMERAS, "MapContextMenu_" + GetHashCode());
			bool zPos = true;
			while (dialog != null)
			{
				yield return null;
				DialogUpdate(zPos);
				yield return updateYield;
				zPos = UpdatePosition();
			}
		}

		private void DialogUpdate(bool zPos)
		{
			if (!zPos || GameSettings.PAUSE.GetKeyDown(ignoreInputLock: true))
			{
				Dismiss();
			}
			if (Input.GetMouseButtonDown(0) && !EventSystem.current.IsPointerOverGameObject())
			{
				Dismiss();
			}
		}

		private bool UpdatePosition()
		{
			Vector3 vector = CanvasUtil.ScreenToUISpacePos(sc.GetScreenSpacePoint(), MapViewCanvasUtil.MapViewCanvasRect, out var zPositive);
			if (zPositive)
			{
				vector = CanvasUtil.AnchorOffset(vector, dialogTrf, Vector2.down);
				dialogTrf.localPosition = vector;
			}
			return zPositive;
		}
	}
}
