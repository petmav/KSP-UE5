using TMPro;
using UnityEngine;

namespace KSP.UI.Screens.DebugToolbar
{
	public class DebugScreenInputLocks : MonoBehaviour
	{
		public TextMeshProUGUI currentLocks;

		public TextMeshProUGUI bitMask;

		public static DebugScreenInputLocks Instance { get; private set; }

		private void Awake()
		{
			Instance = this;
		}

		private void OnDestroy()
		{
			Instance = null;
		}
	}
}
