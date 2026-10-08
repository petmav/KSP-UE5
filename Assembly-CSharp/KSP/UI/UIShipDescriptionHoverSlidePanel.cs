using TMPro;
using UnityEngine.EventSystems;

namespace KSP.UI
{
	public class UIShipDescriptionHoverSlidePanel : UIHoverSlidePanel
	{
		public XSelectable inputFieldSelectable;

		public TMP_InputField shipTextField;

		public TMP_InputField descriptionTextField;

		private bool nameFieldSelected;

		private bool descriptionFieldHover;

		protected new void Start()
		{
			base.Start();
			inputFieldSelectable.onPointerEnter += InputFieldSelectable_onPointerEnter;
			inputFieldSelectable.onPointerExit += InputFieldSelectable_onPointerExit;
			shipTextField.onEndEdit.AddListener(OnEndEdit);
			descriptionTextField.onEndEdit.AddListener(OnEndEdit);
		}

		private void InputFieldSelectable_onPointerExit(XSelectable arg1, PointerEventData arg2)
		{
			nameFieldSelected = false;
			UpdateHoverState();
		}

		private void InputFieldSelectable_onPointerEnter(XSelectable arg1, PointerEventData arg2)
		{
			if (EditorLogic.SelectedPart == null)
			{
				shipTextField.interactable = true;
				descriptionTextField.interactable = true;
				nameFieldSelected = true;
				UpdateHoverState();
			}
			else
			{
				shipTextField.interactable = false;
				descriptionTextField.interactable = false;
			}
		}

		public override void OnPointerEnter(PointerEventData eventData)
		{
			descriptionFieldHover = true;
			UpdateHoverState();
		}

		public override void OnPointerExit(PointerEventData eventData)
		{
			descriptionFieldHover = false;
			UpdateHoverState();
		}

		public void OnEndEdit(string id)
		{
			UpdateHoverState();
		}

		protected void UpdateHoverState()
		{
			if (EditorLogic.SelectedPart != null)
			{
				shipTextField.interactable = false;
				descriptionTextField.interactable = false;
			}
			else
			{
				shipTextField.interactable = true;
				descriptionTextField.interactable = true;
			}
			pointOver = descriptionFieldHover || nameFieldSelected || shipTextField.isFocused || descriptionTextField.isFocused;
			if (pointOver)
			{
				if (coroutine == null && EditorLogic.SelectedPart == null)
				{
					coroutine = StartCoroutine(MoveToState(0f, newState: true));
				}
			}
			else if (coroutine == null)
			{
				coroutine = StartCoroutine(MoveToState(0.7f, newState: false));
			}
		}
	}
}
