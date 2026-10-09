using System;
using UnityEngine;

namespace KSP.UI
{
	public class StageManagerParentResizer : MonoBehaviour
	{
		[SerializeField]
		private UIHoverSlidePanel timeWarpSlidePanel;

		[SerializeField]
		private RectTransform resizingPanel;

		private RectTransform slidePanelRT;

		private void Awake()
		{
			if (timeWarpSlidePanel != null)
			{
				slidePanelRT = timeWarpSlidePanel.transform as RectTransform;
				UIHoverSlidePanel uIHoverSlidePanel = timeWarpSlidePanel;
				uIHoverSlidePanel.OnUpdatePosition = (Callback<Vector2>)Delegate.Combine(uIHoverSlidePanel.OnUpdatePosition, new Callback<Vector2>(Resize));
			}
		}

		private void OnDestroy()
		{
			if (timeWarpSlidePanel != null)
			{
				UIHoverSlidePanel uIHoverSlidePanel = timeWarpSlidePanel;
				uIHoverSlidePanel.OnUpdatePosition = (Callback<Vector2>)Delegate.Remove(uIHoverSlidePanel.OnUpdatePosition, new Callback<Vector2>(Resize));
			}
		}

		private void Resize(Vector2 vector)
		{
			resizingPanel.sizeDelta = new Vector2(resizingPanel.sizeDelta.x, vector.y - resizingPanel.anchoredPosition.y - slidePanelRT.sizeDelta.y);
		}
	}
}
