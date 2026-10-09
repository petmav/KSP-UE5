using TMPro;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	public class StrategyListItem : MonoBehaviour
	{
		public UIRadioButton toggleButton;

		public UIRadioButtonStateChanger toggleStateChanger;

		public RawImage icon;

		[SerializeField]
		private TextMeshProUGUI btnText;

		private string title;

		private string validColor = "#caff00";

		private string invalidColor = "#bdbdbd";

		private bool _setup;

		public void Initialize(Texture texture, string title)
		{
			this.title = title;
			icon.texture = texture;
		}

		private void updateTitle(string title, string colorHex)
		{
			btnText.text = "<color=" + colorHex + ">" + title + "</color>";
		}

		public UIRadioButton SetupButton(bool acceptable, Administration.StrategyWrapper wrapper, UnityAction<PointerEventData, UIRadioButton.CallType> onTrue, UnityAction<PointerEventData, UIRadioButton.CallType> onFalse)
		{
			_setup = true;
			toggleButton.Data = wrapper;
			toggleButton.onTrue.AddListener(onTrue);
			toggleButton.onFalse.AddListener(onFalse);
			if (acceptable)
			{
				updateTitle(title, validColor);
				toggleStateChanger.SetState("ok");
				return toggleButton;
			}
			updateTitle(title, invalidColor);
			toggleStateChanger.SetState("na");
			return toggleButton;
		}

		public void UpdateButton(bool acceptable)
		{
			if (!_setup)
			{
				Debug.LogError("[AdministrationStrategyListIcon] SetupButton not called");
			}
			if (acceptable)
			{
				updateTitle(title, validColor);
				toggleStateChanger.SetState("ok");
			}
			else
			{
				updateTitle(title, invalidColor);
				toggleStateChanger.SetState("na");
			}
		}
	}
}
