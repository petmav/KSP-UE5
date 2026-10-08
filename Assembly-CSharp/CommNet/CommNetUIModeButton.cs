using KSP.Localization;
using KSP.UI;
using KSP.UI.TooltipTypes;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace CommNet
{
	public class CommNetUIModeButton : MonoBehaviour, IPointerClickHandler, IEventSystemHandler
	{
		public Button button;

		public UIStateImage stateImage;

		public TooltipController_Text tooltip;

		protected virtual void Awake()
		{
			base.gameObject.SetActive(value: false);
			GameEvents.CommNet.OnNetworkInitialized.Add(OnNetworkInitialized);
		}

		protected virtual void Start()
		{
			UpdateUI();
		}

		protected virtual void Update()
		{
			UpdateUI();
		}

		protected virtual void OnEnable()
		{
			UpdateUI();
		}

		protected virtual void OnDestroy()
		{
			GameEvents.CommNet.OnNetworkInitialized.Remove(OnNetworkInitialized);
		}

		protected virtual void OnNetworkInitialized()
		{
			base.gameObject.SetActive(value: true);
			UpdateUI();
		}

		public void OnPointerClick(PointerEventData eventData)
		{
			switch (eventData.button)
			{
			case PointerEventData.InputButton.Left:
				CommNetUI.Instance.NextMode();
				UpdateUI();
				break;
			case PointerEventData.InputButton.Right:
				CommNetUI.Instance.PreviousMode();
				UpdateUI();
				break;
			case PointerEventData.InputButton.Middle:
				CommNetUI.Instance.ResetMode();
				UpdateUI();
				break;
			}
		}

		public virtual void UpdateUI()
		{
			string text = Localizer.Format("#autoLOC_6002257") + ": " + CommNetUI.Mode.displayDescription();
			if (tooltip.textString != text)
			{
				tooltip.SetText(text);
			}
			stateImage.SetState((int)CommNetUI.Mode);
		}
	}
}
