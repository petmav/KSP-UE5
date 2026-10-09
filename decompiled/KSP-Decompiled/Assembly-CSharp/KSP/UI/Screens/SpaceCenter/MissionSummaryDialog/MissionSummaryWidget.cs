using TMPro;
using UnityEngine;

namespace KSP.UI.Screens.SpaceCenter.MissionSummaryDialog
{
	public class MissionSummaryWidget : MonoBehaviour
	{
		private RectTransform rTrf;

		protected MissionRecoveryDialog host;

		[SerializeField]
		protected TextMeshProUGUI header;

		public RectTransform RTrf => rTrf;

		protected void Init(MissionRecoveryDialog host)
		{
			this.host = host;
			rTrf = GetComponent<RectTransform>();
		}
	}
}
