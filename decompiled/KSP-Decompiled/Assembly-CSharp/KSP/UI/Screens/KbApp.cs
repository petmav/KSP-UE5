using System;
using UnityEngine;

namespace KSP.UI.Screens
{
	public abstract class KbApp : MonoBehaviour
	{
		public Texture appIcon;

		public string appName;

		public string appTitle;

		public KnowledgeBase.KbTargetType targetType;

		[NonSerialized]
		public ApplicationLauncherButton appLauncherButton;

		public KbAppFrame appFramePrefab;

		[NonSerialized]
		public KbAppFrame appFrame;

		public Color headerColor;

		protected bool pinned { get; private set; }

		public bool appIsLive { get; private set; }

		protected abstract void DisplayApp();

		protected abstract void HideApp();

		public abstract void ActivateApp(MapObject target);

		public virtual void Awake()
		{
		}

		public virtual void Start()
		{
			if (appFrame != null)
			{
				appFrame.header.color = headerColor;
			}
		}

		public virtual void OnDestroy()
		{
			Debug.Log("KbApp.OnDestroy " + appName);
			if (appFrame != null)
			{
				appFrame.gameObject.DestroyGameObject();
			}
			if (ApplicationLauncher.Instance != null)
			{
				ApplicationLauncher.Instance.DisableMutuallyExclusive(appLauncherButton);
			}
		}

		public virtual void Setup()
		{
			appLauncherButton = UnityEngine.Object.Instantiate(KnowledgeBase.Instance.appLauncherButtonPrefab);
			appLauncherButton.Setup(Show, Hide, Hover, HoverOut, EnablePanel, DisablePanel, appIcon);
			if (appFramePrefab != null)
			{
				appFrame = UnityEngine.Object.Instantiate(appFramePrefab);
				appFrame.Setup(appLauncherButton, appName, appTitle);
			}
			ApplicationLauncher.Instance.EnableMutuallyExclusive(appLauncherButton);
		}

		public void Restore()
		{
			if (appLauncherButton.toggleButton.CurrentState == UIRadioButton.State.True)
			{
				displayApp();
			}
		}

		public void Show()
		{
			if (!appIsLive && ApplicationLauncher.Instance.DetermineVisibility(appLauncherButton))
			{
				displayApp();
			}
			pinned = true;
		}

		public void Hide()
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
				hideApp();
			}
		}

		private void EnablePanel()
		{
			Show();
		}

		private void DisablePanel()
		{
			Hide();
		}

		private void displayApp()
		{
			appIsLive = true;
			if (appFrame != null)
			{
				appFrame.gameObject.SetActive(value: true);
				appFrame.Reposition();
			}
			DisplayApp();
		}

		private void hideApp()
		{
			appIsLive = false;
			if (appFrame != null)
			{
				appFrame.gameObject.SetActive(value: false);
			}
			HideApp();
		}
	}
}
