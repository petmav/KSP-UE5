using KSP.Localization;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.Flight.Dialogs
{
	public class FlagPlaqueDialog : MonoBehaviour
	{
		[SerializeField]
		private TextMeshProUGUI textHeader;

		[SerializeField]
		private TextMeshProUGUI textContent;

		[SerializeField]
		private Button buttonClose;

		private Callback onDismiss;

		private string plaqueText;

		private string siteName;

		public static FlagPlaqueDialog Spawn(string siteName, string plaqueText, Callback onDismiss)
		{
			FlagPlaqueDialog component = Object.Instantiate(AssetBase.GetPrefab("FlagPlaqueDialog")).GetComponent<FlagPlaqueDialog>();
			component.transform.SetParent(DialogCanvasUtil.DialogCanvasRect, worldPositionStays: false);
			component.siteName = siteName;
			component.plaqueText = plaqueText;
			component.onDismiss = onDismiss;
			return component;
		}

		public void Terminate()
		{
			Object.Destroy(base.gameObject);
		}

		protected void Start()
		{
			textHeader.text = Localizer.Format("#autoLOC_457848", siteName);
			textContent.text = plaqueText;
			buttonClose.onClick.AddListener(onBtnClose);
		}

		private void onBtnClose()
		{
			if (onDismiss != null)
			{
				onDismiss();
			}
			Terminate();
		}
	}
}
