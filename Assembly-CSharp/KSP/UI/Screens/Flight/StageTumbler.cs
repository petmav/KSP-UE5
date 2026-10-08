using UnityEngine;

namespace KSP.UI.Screens.Flight
{
	public class StageTumbler : MonoBehaviour
	{
		public Tumbler tumbler;

		private void Awake()
		{
			SetTumblerPosition();
		}

		private void SetTumblerPosition()
		{
			base.transform.localPosition = new Vector3(base.transform.localPosition.x, base.transform.localPosition.y, -1500f + 1222f * GameSettings.UI_SCALE - 320f * GameSettings.UI_SCALE * GameSettings.UI_SCALE);
		}

		private void Reset()
		{
			tumbler = GetComponent<Tumbler>();
		}

		private void LateUpdate()
		{
			if (FlightGlobals.ready && tumbler != null)
			{
				tumbler.SetValue(Mathf.Clamp(StageManager.CurrentStage, 0, StageManager.LastStage));
			}
		}
	}
}
