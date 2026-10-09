using UnityEngine;

namespace KSP.UI
{
	public class UIDialogCamera : UICameraBase
	{
		public static UIDialogCamera Instance;

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
