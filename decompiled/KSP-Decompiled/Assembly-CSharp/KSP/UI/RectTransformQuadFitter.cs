using System.Collections;
using UnityEngine;

namespace KSP.UI
{
	public class RectTransformQuadFitter : MonoBehaviour
	{
		public RectTransform rectTransform;

		public Transform quadTransform;

		private bool updateDaemonRunning;

		private void Awake()
		{
			StartCoroutine(UpdateDaemon());
		}

		private IEnumerator UpdateDaemon()
		{
			yield return null;
			yield return null;
			if (updateDaemonRunning)
			{
				yield break;
			}
			updateDaemonRunning = true;
			while ((bool)this)
			{
				if (rectTransform.hasChanged)
				{
					rectTransform.hasChanged = false;
					quadTransform.localScale = new Vector3(rectTransform.sizeDelta.x, rectTransform.sizeDelta.y, 1f);
				}
				yield return null;
			}
		}
	}
}
