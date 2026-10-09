using KSP.Localization;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	public class PrereleaseWarningDialog : MonoBehaviour
	{
		[SerializeField]
		private Button btnContinue;

		public TextMeshProUGUI textBox;

		private void Awake()
		{
			if (!Versioning.isPrerelease)
			{
				Object.Destroy(base.gameObject);
				return;
			}
			textBox.text = Localizer.Format("#autoLOC_1900246", Versioning.version_major + "." + Versioning.version_minor + "." + Versioning.Revision, "http://bugs.kerbalspaceprogram.com/projects/prerelease");
			base.transform.SetParent(DialogCanvasUtil.DialogCanvasRect, worldPositionStays: false);
			btnContinue.onClick.AddListener(OnBtnContinue);
		}

		private void OnBtnContinue()
		{
			Object.Destroy(base.gameObject);
		}
	}
}
