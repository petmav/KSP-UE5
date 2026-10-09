using UnityEngine;
using UnityEngine.UI;

namespace KSPAssets.KSPedia
{
	[RequireComponent(typeof(Button))]
	public class DatabaseLinkButton : MonoBehaviour
	{
		public string linkUrl = "";

		private Button btn;

		private void Awake()
		{
			btn = GetComponent<Button>();
			btn.onClick.AddListener(OnClick);
		}

		protected virtual void OnClick()
		{
			KSPediaController.Instance.ShowScreen(linkUrl);
		}
	}
}
