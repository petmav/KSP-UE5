using KSP.UI.Screens;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.EventSystems;

namespace KSP.UI
{
	public class SimpleAppFrame : MonoBehaviour
	{
		public PointerEnterExitHandler hoverController;

		private ApplicationLauncherButton appLauncherButton;

		private RectTransform rectTransform;

		private void Awake()
		{
			rectTransform = base.transform as RectTransform;
		}

		public void Setup(ApplicationLauncherButton appLauncherButton, string appName)
		{
			this.appLauncherButton = appLauncherButton;
			ApplicationLauncher.Instance.AddOnRepositionCallback(Reposition);
			Reposition();
		}

		public void AddGlobalInputDelegate(UnityAction<PointerEventData> pointerEnter, UnityAction<PointerEventData> pointerExit)
		{
			hoverController.onPointerEnter.AddListener(pointerEnter);
			hoverController.onPointerExit.AddListener(pointerExit);
		}

		public void Reposition()
		{
			if (ApplicationLauncher.Instance.IsPositionedAtTop)
			{
				base.gameObject.transform.SetParent(ApplicationLauncher.Instance.appSpace, worldPositionStays: false);
				rectTransform.anchoredPosition = appLauncherButton.GetAnchorTopRight();
			}
			else
			{
				base.gameObject.transform.SetParent(ApplicationLauncher.Instance.appSpace, worldPositionStays: false);
			}
		}

		private void OnDestroy()
		{
			if ((bool)ApplicationLauncher.Instance)
			{
				ApplicationLauncher.Instance.RemoveOnRepositionCallback(Reposition);
			}
		}
	}
}
