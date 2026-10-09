using System;
using System.Collections.Generic;
using System.IO;
using UnityEngine;

public class AdvancedSettings : MonoBehaviour
{
	public bool AMD;

	public bool dontShowLauncher;

	public bool extChar;

	public bool noLog;

	public bool fakeFullS;

	public bool already;

	public bool x64Player;

	public Play plyObj;

	private string path;

	private Dictionary<string, string> advancedDict;

	private void Awake()
	{
		UnityEngine.Object.DontDestroyOnLoad(base.gameObject);
		already = false;
		if (GameObject.FindGameObjectsWithTag("SettingsObj").Length > 1)
		{
			already = true;
			UnityEngine.Object.Destroy(base.gameObject);
		}
		plyObj = GameObject.Find("Play").GetComponent<Play>();
		AMD = false;
		dontShowLauncher = false;
		extChar = false;
		noLog = false;
		fakeFullS = false;
		if (SystemInfo.operatingSystem.Contains("64bit"))
		{
			x64Player = true;
		}
		else
		{
			x64Player = false;
		}
		advancedDict = new Dictionary<string, string>();
	}

	private void Start()
	{
		path = ((Application.platform == RuntimePlatform.WindowsEditor) ? Path.Combine(Application.dataPath, "../Settings_Data/E5150.cfg.bak") : Path.Combine(Application.dataPath, "E5150.cfg.bak"));
		Debug.Log("E5150 path: " + path);
		if (!File.Exists(path))
		{
			advancedDict.Add("DONT_SHOW_LAUNCHER ", "false");
			advancedDict.Add("AMD_MODE ", "false");
			advancedDict.Add("NO_LOG_MODE ", "false");
			advancedDict.Add("FRAMELESS_WINDOW ", "false");
			advancedDict.Add("EXT_CHAR_MODE ", "false");
			if (SystemInfo.operatingSystem.Contains("64bit"))
			{
				advancedDict.Add("RUN_X64_PLAYER ", "true");
			}
			else
			{
				advancedDict.Add("RUN_X64_PLAYER ", "false");
			}
			saveAdvancedSettings();
		}
		else
		{
			loadAdvancedSettings();
		}
	}

	private void loadAdvancedSettings()
	{
		Debug.Log("Loading Settings...");
		if (File.Exists(path))
		{
			string[] array = File.ReadAllLines(path);
			for (int i = 0; i < array.Length; i++)
			{
				string key = array[i].Substring(0, array[i].IndexOf("="));
				string value = array[i].Substring(array[i].IndexOf("=") + 1, array[i].Length - array[i].IndexOf("=") - 1);
				advancedDict.Add(key, value);
			}
		}
		if (advancedDict.ContainsKey("DONT_SHOW_LAUNCHER "))
		{
			dontShowLauncher = Convert.ToBoolean(advancedDict["DONT_SHOW_LAUNCHER "]);
		}
		if (advancedDict.ContainsKey("AMD_MODE "))
		{
			AMD = Convert.ToBoolean(advancedDict["AMD_MODE "]);
		}
		if (advancedDict.ContainsKey("NO_LOG_MODE "))
		{
			noLog = Convert.ToBoolean(advancedDict["NO_LOG_MODE "]);
		}
		if (advancedDict.ContainsKey("FRAMELESS_WINDOW "))
		{
			fakeFullS = Convert.ToBoolean(advancedDict["FRAMELESS_WINDOW "]);
		}
		if (advancedDict.ContainsKey("EXT_CHAR_MODE "))
		{
			extChar = Convert.ToBoolean(advancedDict["EXT_CHAR_MODE "]);
		}
		if (advancedDict.ContainsKey("RUN_X64_PLAYER "))
		{
			x64Player = Convert.ToBoolean(advancedDict["RUN_X64_PLAYER "]);
		}
		if (dontShowLauncher && !already)
		{
			plyObj.skipLauncher();
		}
	}

	public void saveAdvancedSettings()
	{
		advancedDict["DONT_SHOW_LAUNCHER "] = " " + dontShowLauncher;
		advancedDict["AMD_MODE "] = " " + AMD;
		advancedDict["NO_LOG_MODE "] = " " + noLog;
		advancedDict["FRAMELESS_WINDOW "] = " " + fakeFullS;
		advancedDict["EXT_CHAR_MODE "] = " " + extChar;
		advancedDict["RUN_X64_PLAYER "] = " " + x64Player;
		int num = 0;
		string[] array = new string[advancedDict.Count];
		foreach (KeyValuePair<string, string> item in advancedDict)
		{
			array[num] = item.Key + "=" + item.Value;
			Debug.Log("AdvSettings: " + array[num]);
			num++;
		}
		File.WriteAllLines(path, array);
		already = true;
	}

	public void setAMDState(string state)
	{
		AMD = Convert.ToBoolean(state);
	}

	public void setLauncherState(string state)
	{
		dontShowLauncher = Convert.ToBoolean(state);
	}

	public void setCharState(string state)
	{
		extChar = Convert.ToBoolean(state);
	}

	public void setFullSState(string state)
	{
		fakeFullS = Convert.ToBoolean(state);
	}

	public void setunLogState(string state)
	{
		noLog = Convert.ToBoolean(state);
	}

	public void setx64tate(string state)
	{
		x64Player = Convert.ToBoolean(state);
	}
}
