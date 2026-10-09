using UnityEngine;

namespace KSP.UI
{
	public class AnchoredDialogHost : MonoBehaviour
	{
		public AnchoredDialog host;

		public Callback OnHostLateUpdate;

		private void LateUpdate()
		{
			if (host != null)
			{
				OnHostLateUpdate();
			}
			else
			{
				Object.Destroy(base.gameObject);
			}
		}
	}
}
