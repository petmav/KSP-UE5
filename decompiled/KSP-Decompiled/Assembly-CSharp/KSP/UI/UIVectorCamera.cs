using UnityEngine;

namespace KSP.UI
{
	public class UIVectorCamera : UICameraBase
	{
		public static UIVectorCamera Instance;

		public static Camera Camera => Instance.cam;

		private void Awake()
		{
			Instance = this;
		}

		private void OnDestroy()
		{
			if (Instance != null && Instance == this)
			{
				Instance = null;
			}
		}
	}
}
