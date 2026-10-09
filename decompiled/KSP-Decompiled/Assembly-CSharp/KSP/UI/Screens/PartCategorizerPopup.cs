using System.Collections.Generic;
using KSP.Localization;
using RUI.Icons.Selectable;
using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	public class PartCategorizerPopup : MonoBehaviour
	{
		public delegate bool CriteriaAccept(string categoryName, out string reason);

		public delegate bool CriteriaDelete(out string reason);

		public TextMeshProUGUI title;

		public Button btnClose;

		public Button btnDelete;

		public Button btnAccept;

		public InputField inputField;

		public UIList scrollList;

		private Callback onAccept = delegate
		{
		};

		private Callback onDelete = delegate
		{
		};

		public CriteriaAccept onAcceptCriteria = delegate(string categoryName, out string reason)
		{
			reason = "";
			return false;
		};

		public CriteriaDelete onDeleteCriteria = delegate(out string reason)
		{
			reason = "";
			return false;
		};

		private bool _started;

		private PartCategorizer.PopupData currentPopupData;

		private List<PartCategorizerButton> buttonList = new List<PartCategorizerButton>();

		private Icon selectedIcon;

		public bool Showing { get; private set; }

		private void Awake()
		{
			btnClose.onClick.AddListener(MouseinputClose);
			btnAccept.onClick.AddListener(MouseinputAccept);
			btnDelete.onClick.AddListener(MouseinputDelete);
		}

		public void Show(PartCategorizer.PopupData popupData, Callback onAccept, CriteriaAccept onAcceptCriteria, Callback onDelete, CriteriaDelete onDeleteCriteria)
		{
			currentPopupData = popupData;
			Showing = true;
			base.gameObject.SetActive(value: true);
			if (popupData != null)
			{
				title.text = popupData.popupName;
				inputField.text = popupData.categorydisplayName;
			}
			this.onAccept = onAccept;
			this.onAcceptCriteria = onAcceptCriteria;
			if (onDelete == null)
			{
				btnDelete.gameObject.SetActive(value: false);
			}
			else
			{
				btnDelete.gameObject.SetActive(value: true);
				this.onDelete = onDelete;
				this.onDeleteCriteria = onDeleteCriteria;
			}
			if (!_started)
			{
				_started = true;
				CreateIconList();
			}
			if (popupData != null)
			{
				UpdateIconList();
			}
			InputLockManager.SetControlLock(ControlTypes.EDITOR_SOFT_LOCK, "PartCategorizerPopup");
		}

		public void Hide()
		{
			Showing = false;
			base.gameObject.SetActive(value: false);
			InputLockManager.RemoveControlLock("PartCategorizerPopup");
		}

		private void MouseinputClose()
		{
			Hide();
		}

		private void MouseinputAccept()
		{
			string reason = "";
			if (onAcceptCriteria(inputField.text, out reason))
			{
				currentPopupData.categoryName = inputField.text;
				currentPopupData.categorydisplayName = inputField.text;
				currentPopupData.icon = selectedIcon;
				onAccept();
				Hide();
			}
			else
			{
				PopupDialog.SpawnPopupDialog(new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), new MultiOptionDialog("MouseInputWarning", reason, Localizer.Format("#autoLOC_455453"), HighLogic.UISkin, new DialogGUIButton(Localizer.Format("#autoLOC_6004034"), delegate
				{
				})), persistAcrossScenes: false, HighLogic.UISkin);
			}
		}

		private void MouseinputDelete()
		{
			string reason = "";
			if (onDeleteCriteria(out reason))
			{
				AcceptDelete();
				return;
			}
			PopupDialog.SpawnPopupDialog(new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), new MultiOptionDialog("MouseInputDelete", reason, Localizer.Format("#autoLOC_455453"), HighLogic.UISkin, new DialogGUIButton(Localizer.Format("#autoLOC_455454"), delegate
			{
			}), new DialogGUIButton(Localizer.Format("#autoLOC_455455"), AcceptDelete)), persistAcrossScenes: false, HighLogic.UISkin);
		}

		private void AcceptDelete()
		{
			onDelete();
			Hide();
		}

		private void CreateIconList()
		{
			scrollList.Clear(destroyElements: true);
			buttonList.Clear();
			int count = PartCategorizer.Instance.iconLoader.icons.Count;
			for (int i = 0; i < count; i++)
			{
				PartCategorizerButton partCategorizerButton = CreateIcon(PartCategorizer.Instance.iconLoader.icons[i], PartCategorizer.Instance.colorCategory, PartCategorizer.Instance.colorIcons);
				scrollList.AddItem(partCategorizerButton.GetComponent<UIListItem>());
				buttonList.Add(partCategorizerButton);
			}
		}

		private void UpdateIconList()
		{
			PartCategorizerButton selectedButton = null;
			int count = buttonList.Count;
			for (int i = 0; i < count; i++)
			{
				if (i == 0)
				{
					selectedButton = buttonList[i];
				}
				if (currentPopupData.icon.Equals(PartCategorizer.Instance.iconLoader.icons[i]))
				{
					selectedButton = buttonList[i];
					break;
				}
			}
			StartCoroutine(CallbackUtil.DelayedCallback(2, delegate
			{
				selectedButton.activeButton.SetState(UIRadioButton.State.True, UIRadioButton.CallType.APPLICATION, null);
			}));
		}

		private PartCategorizerButton CreateIcon(Icon icon, Color color, Color colorIcon)
		{
			PartCategorizerButton partCategorizerButton = PartCategorizer.InstantiatePartCategorizerButton("new icon", "#autoLOC_6004033", icon, color, colorIcon);
			partCategorizerButton.activeButton.unselectable = false;
			partCategorizerButton.activeButton.onTrueBtn.AddListener(OnIconSelect);
			partCategorizerButton.DisableDividers();
			partCategorizerButton.SetRadioGroup(238);
			return partCategorizerButton;
		}

		private void OnIconSelect(UIRadioButton button, UIRadioButton.CallType callType, PointerEventData data)
		{
			selectedIcon = button.transform.parent.gameObject.GetComponent<PartCategorizerButton>().icon;
		}
	}
}
