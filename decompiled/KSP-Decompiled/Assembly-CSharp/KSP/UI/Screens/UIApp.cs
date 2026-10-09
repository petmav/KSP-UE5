using System.Collections;
using UnityEngine;
using UnityEngine.EventSystems;

namespace KSP.UI.Screens
{
	public abstract class UIApp : MonoBehaviour
	{
		[SerializeField]
		private string appName;

		public Texture appLauncherIcon;

		public Animator appLauncherAnim;

		[SerializeField]
		protected int AppStartFrameDelay = 5;

		[SerializeField]
		protected bool enableOnHover = true;

		[SerializeField]
		protected bool enableMutuallyExclusive = true;

		protected Callback defaultCallback = delegate
		{
		};

		public ApplicationLauncherButton appLauncherButton { get; protected set; }

		protected bool hover { get; private set; }

		protected bool pinned { get; private set; }

		protected bool appIsLive { get; private set; }

		public bool IsShowing => appIsLive;

		protected abstract ApplicationLauncher.AppScenes GetAppScenes();

		protected abstract Vector3 GetAppScreenPos(Vector3 defaultAnchorPos);

		protected abstract bool OnAppAboutToStart();

		protected abstract void OnAppInitialized();

		protected abstract void OnAppDestroy();

		protected abstract void DisplayApp();

		protected abstract void HideApp();

		protected virtual void OnAppStarted()
		{
		}

		public virtual void Awake()
		{
			Debug.Log("[UiApp] Awake: " + appName);
			GameEvents.onGUIApplicationLauncherReady.Add(OnAppLauncherReady);
		}

		private void OnDestroy()
		{
			Debug.Log("[UIApp] OnDestroy: " + appName);
			GameEvents.onGUIApplicationLauncherReady.Remove(OnAppLauncherReady);
			OnAppDestroy();
			if (appLauncherButton != null && ApplicationLauncher.Instance != null)
			{
				ApplicationLauncher.Instance.RemoveOnHideCallback(OnAppLauncherHide);
				ApplicationLauncher.Instance.RemoveOnShowCallback(OnAppLauncherShow);
				ApplicationLauncher.Instance.RemoveOnRepositionCallback(Reposition);
				ApplicationLauncher.Instance.RemoveApplication(appLauncherButton);
			}
		}

		private void OnAppLauncherReady()
		{
			StartCoroutine(AddToAppLauncher());
		}

		private IEnumerator AddToAppLauncher()
		{
			for (int i = 0; i < AppStartFrameDelay; i++)
			{
				yield return null;
			}
			yield return null;
			StartCoroutine(CallbackUtil.DelayedCallback(2, delegate
			{
				OnAppStarted();
			}));
			if (!OnAppAboutToStart())
			{
				yield break;
			}
			Debug.Log("[UIApp] Adding " + appName + " to Application Launcher");
			if (appLauncherAnim != null)
			{
				Animator sprite = Object.Instantiate(appLauncherAnim);
				appLauncherButton = ApplicationLauncher.Instance.AddApplication(Show, Hide, enableOnHover ? new Callback(Hover) : defaultCallback, enableOnHover ? new Callback(HoverOut) : defaultCallback, EnablePanel, DisablePanel, sprite);
			}
			else
			{
				if (!(appLauncherIcon != null))
				{
					Debug.LogError("UIApp: No Applauncher button specified, aborting");
					Object.Destroy(base.gameObject);
					yield break;
				}
				appLauncherButton = ApplicationLauncher.Instance.AddApplication(Show, Hide, enableOnHover ? new Callback(Hover) : defaultCallback, enableOnHover ? new Callback(HoverOut) : defaultCallback, EnablePanel, DisablePanel, appLauncherIcon);
			}
			appLauncherButton.VisibleInScenes = GetAppScenes();
			ApplicationLauncher.Instance.AddOnHideCallback(OnAppLauncherHide);
			ApplicationLauncher.Instance.AddOnShowCallback(OnAppLauncherShow);
			ApplicationLauncher.Instance.AddOnRepositionCallback(Reposition);
			if (!enableMutuallyExclusive)
			{
				ApplicationLauncher.Instance.DisableMutuallyExclusive(appLauncherButton);
			}
			base.gameObject.transform.position = GetAppScreenPos(new Vector3(appLauncherButton.GetAnchor().x - 150f + 38f, appLauncherButton.GetAnchor().y, appLauncherButton.GetAnchor().z));
			StartCoroutine(CallbackUtil.DelayedCallback(1, delegate
			{
				OnAppInitialized();
			}));
		}

		public void ForceAddToAppLauncher()
		{
			StartCoroutine(AddToAppLauncher());
		}

		protected virtual void Reposition()
		{
		}

		protected void MouseInput_PointerEnter(PointerEventData eventData)
		{
			if (!hover)
			{
				Hover();
				appLauncherButton.onHoverBtn(appLauncherButton.toggleButton);
				hover = true;
			}
		}

		protected void MouseInput_PointerExit(PointerEventData eventData)
		{
			if (!appLauncherButton.IsHovering)
			{
				HoverOut();
				appLauncherButton.onHoverOutBtn(appLauncherButton.toggleButton);
				hover = false;
			}
		}

		private void Show()
		{
			if (!appIsLive)
			{
				displayApp();
			}
			pinned = true;
		}

		private void Hide()
		{
			if (appIsLive)
			{
				hideApp();
			}
			pinned = false;
		}

		private void Hover()
		{
			if (!appIsLive)
			{
				displayApp();
			}
		}

		private void HoverOut()
		{
			if (appIsLive && !pinned)
			{
				StartCoroutine(HoverOutCoroutine());
			}
		}

		private IEnumerator HoverOutCoroutine()
		{
			yield return null;
			if (!appLauncherButton.IsHovering && appLauncherButton.toggleButton.CurrentState != 0)
			{
				hideApp();
			}
		}

		private void EnablePanel()
		{
			if (appIsLive)
			{
				Show();
			}
		}

		private void DisablePanel()
		{
			Hide();
		}

		private void OnAppLauncherHide()
		{
			if (appIsLive)
			{
				HideApp();
			}
		}

		private void OnAppLauncherShow()
		{
			if (appIsLive)
			{
				DisplayApp();
			}
		}

		private void displayApp()
		{
			appIsLive = true;
			DisplayApp();
		}

		private void hideApp()
		{
			if (!hover)
			{
				appIsLive = false;
				HideApp();
			}
		}
	}
}
