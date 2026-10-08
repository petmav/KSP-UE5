using System.Collections;
using UnityEngine;

public class StatsCallHome : MonoBehaviour
{
	public static string LAUNCHER_ID = "0";

	public static string LAUNCHER_PLATFORM = "Editor";

	private string serverURL = "https://kerbalspaceprogram.com/kspstore/gamescripts/launcherch.php";

	private string _value;

	private bool quit;

	private void Start()
	{
		GetLauncherInfo();
	}

	private void GetLauncherInfo()
	{
		if (LAUNCHER_ID == "0")
		{
			CreateGameID();
		}
		switch (Application.platform)
		{
		case RuntimePlatform.WindowsPlayer:
			LAUNCHER_PLATFORM = "Windows";
			break;
		case RuntimePlatform.OSXPlayer:
			LAUNCHER_PLATFORM = "OSX";
			break;
		case RuntimePlatform.LinuxPlayer:
			LAUNCHER_PLATFORM = "Linux";
			break;
		}
	}

	public void SendInfo(string value)
	{
		_value = value;
		StartCoroutine(SendStatsInfo());
	}

	private IEnumerator SendStatsInfo()
	{
		WWWForm wWWForm = new WWWForm();
		if (LAUNCHER_ID != "0")
		{
			wWWForm.AddField("id", LAUNCHER_ID);
		}
		wWWForm.AddField("platform", LAUNCHER_PLATFORM);
		wWWForm.AddField("value", _value);
		WWW conection = new WWW(serverURL, wWWForm);
		yield return conection;
		if (conection.error != null)
		{
			MonoBehaviour.print(conection.error);
		}
		else if (conection.text.Contains("OBTAINED"))
		{
			MonoBehaviour.print("Stats sent. Value: " + _value);
			if (quit)
			{
				Application.Quit();
			}
		}
		else
		{
			MonoBehaviour.print(conection.text);
		}
	}

	private void OnApplicationQuit()
	{
		if (!quit)
		{
			Application.CancelQuit();
		}
		if (GameObject.Find("AdvSettings").GetComponent<AdvancedSettings>().dontShowLauncher)
		{
			SendInfo("DISABLED");
		}
		else
		{
			SendInfo("ENABLED");
		}
		quit = true;
	}

	private void CreateGameID()
	{
		LAUNCHER_ID = SystemInfo.deviceUniqueIdentifier;
	}
}
