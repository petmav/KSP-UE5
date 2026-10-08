using UnityEngine;
using UnityEngine.SceneManagement;

namespace KSP.UI.Screens.DebugToolbar
{
	public class DebugScreenSpawner : MonoBehaviour
	{
		public static DebugScreenSpawner Instance;

		public DebugScreen screenPrefab;

		private DebugScreen screen;

		private AddDebugScreens debugScreens;

		private void Awake()
		{
			if (Instance != null)
			{
				base.gameObject.DestroyGameObject();
				return;
			}
			Instance = this;
			SceneManager.sceneLoaded += OnSceneLoaded;
		}

		private void Start()
		{
			screen = Object.Instantiate(screenPrefab);
			screen.transform.SetParent(base.transform, worldPositionStays: false);
			debugScreens = screen.GetComponent<AddDebugScreens>();
			screen.Hide();
		}

		private void OnDestroy()
		{
			SceneManager.sceneLoaded -= OnSceneLoaded;
		}

		private void OnSceneLoaded(Scene scene, LoadSceneMode mode)
		{
			InputLockManager.RemoveControlLock("DebugToolbar");
		}

		private void Update()
		{
			if (Input.GetKeyDown(KeyCode.F12) && GameSettings.MODIFIER_KEY.GetKey())
			{
				if (screen.isShown)
				{
					screen.Hide();
				}
				else
				{
					screen.Show();
				}
			}
		}

		public static void ShowDebugScreen()
		{
			if (!(Instance == null) && !Instance.screen.isShown)
			{
				Instance.screen.Show();
			}
		}

		public static void ShowTab(string name)
		{
			if (Instance == null)
			{
				return;
			}
			ShowDebugScreen();
			int index = 0;
			for (int i = 0; i < Instance.debugScreens.screens.Count; i++)
			{
				if (name == Instance.debugScreens.screens[i].name)
				{
					index = i;
					break;
				}
			}
			Instance.screen.SelectItem(Instance.debugScreens.screens[index].text, Instance.debugScreens.screens[index].parentName + "/" + Instance.debugScreens.screens[index].name, Instance.debugScreens.screens[index].screen);
		}
	}
}
