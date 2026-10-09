using System.Collections;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.DebugToolbar
{
	public class ScreenDatabaseList : MonoBehaviour
	{
		public TextMeshQueue textMeshQueue;

		public ScrollRect scrollRect;

		private void Awake()
		{
			LoadDatabaseListItems();
		}

		private void OnEnable()
		{
			StartCoroutine(ScrollUp());
		}

		private IEnumerator ScrollUp()
		{
			yield return null;
			if (scrollRect != null)
			{
				scrollRect.verticalNormalizedPosition = 1f;
			}
		}

		protected virtual void LoadDatabaseListItems()
		{
			for (int i = 0; i <= 100; i++)
			{
				textMeshQueue.AddLine("DatabaseList is not properly set up.");
			}
		}
	}
}
