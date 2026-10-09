using UnityEngine;

namespace KSP.UI
{
	[RequireComponent(typeof(CanvasGroup))]
	public class CanvasGroupEventLocker : MonoBehaviour
	{
		private CanvasGroup canvasGroup;

		[SerializeField]
		private bool lockWhileKSPediaOpen;

		private void Awake()
		{
			canvasGroup = GetComponent<CanvasGroup>();
			if (lockWhileKSPediaOpen)
			{
				GameEvents.onGUIKSPediaSpawn.Add(Lock);
				GameEvents.onGUIKSPediaDespawn.Add(Unlock);
			}
		}

		private void OnDestroy()
		{
			if (lockWhileKSPediaOpen)
			{
				GameEvents.onGUIKSPediaSpawn.Remove(Lock);
				GameEvents.onGUIKSPediaDespawn.Remove(Unlock);
			}
		}

		private void Lock()
		{
			canvasGroup.blocksRaycasts = false;
		}

		private void Unlock()
		{
			canvasGroup.blocksRaycasts = true;
		}
	}
}
