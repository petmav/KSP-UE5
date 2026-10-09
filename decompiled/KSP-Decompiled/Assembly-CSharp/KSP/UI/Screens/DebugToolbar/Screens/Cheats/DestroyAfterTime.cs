using UnityEngine;

namespace KSP.UI.Screens.DebugToolbar.Screens.Cheats
{
	public class DestroyAfterTime : MonoBehaviour
	{
		public float time = 1f;

		private float startTime;

		private void Start()
		{
			startTime = Time.realtimeSinceStartup;
		}

		private void Update()
		{
			if (Time.realtimeSinceStartup > startTime + time && base.gameObject != null)
			{
				Object.Destroy(base.gameObject);
			}
		}
	}
}
