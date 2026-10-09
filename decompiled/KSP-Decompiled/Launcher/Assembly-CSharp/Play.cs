using System;
using System.Diagnostics;
using System.IO;
using UnityEngine;

public class Play : MonoBehaviour
{
	private bool allowStartGame;

	public Texture2D playOn;

	public Texture2D playOff;

	private string exePath;

	public bool kspExists;

	public AdvancedSettings advSet;

	public UpdateKSP update;

	public Texture2D cursorTexture;

	private void Start()
	{
		string text = "";
		kspExists = false;
		update = GameObject.Find("Update").GetComponent<UpdateKSP>();
		advSet = GameObject.Find("AdvSettings").GetComponent<AdvancedSettings>();
		if (Application.platform == RuntimePlatform.WindowsEditor)
		{
			exePath = Path.Combine(Application.dataPath, "../KSP_win/KSP.exe");
		}
		else if (Application.platform == RuntimePlatform.WindowsPlayer)
		{
			if (advSet.x64Player && SystemInfo.operatingSystem.Contains("64bit") && File.Exists(Path.Combine(Application.dataPath, "../KSP_x64.exe")))
			{
				exePath = Path.Combine(Application.dataPath, "../KSP_x64.exe");
			}
			else
			{
				exePath = Path.Combine(Application.dataPath, "../KSP.exe");
			}
		}
		else if (Application.platform == RuntimePlatform.OSXPlayer)
		{
			exePath = Path.Combine(Application.dataPath, "../../KSP.app/Contents/MacOS/KSP");
		}
		else if (Application.platform == RuntimePlatform.LinuxPlayer)
		{
			Process process = new Process();
			process.EnableRaisingEvents = false;
			process.StartInfo.RedirectStandardOutput = true;
			process.StartInfo.UseShellExecute = false;
			process.StartInfo.FileName = "uname";
			process.StartInfo.Arguments = "-a";
			process.Start();
			process.WaitForExit();
			try
			{
				text = process.StandardOutput.ReadToEnd();
				UnityEngine.Debug.Log("Determining bitness: " + text);
			}
			catch (InvalidOperationException)
			{
				UnityEngine.Debug.Log("Going for default bitness");
				text = "x86_64";
			}
			if (text.Contains("x86_64"))
			{
				exePath = Path.Combine(Application.dataPath, "../KSP.x86_64");
			}
			else
			{
				exePath = Path.Combine(Application.dataPath, "../KSP.x86");
			}
			process.Close();
		}
		UnityEngine.Debug.Log(SystemInfo.operatingSystem);
		UnityEngine.Debug.Log("ExecPath = " + exePath);
		if (File.Exists(exePath))
		{
			kspExists = true;
		}
	}

	private bool shouldAllowPlay()
	{
		if (update.patcherIsDoneDownloading && kspExists)
		{
			return true;
		}
		return false;
	}

	public void AllowStartGame(bool allow)
	{
		allowStartGame = allow;
		if (allowStartGame)
		{
			ChangeTexture(1);
		}
		else
		{
			ChangeTexture(0);
		}
	}

	public void skipLauncher()
	{
		QuitAndStartGame();
	}

	private void Update()
	{
		AllowStartGame(shouldAllowPlay());
	}

	private void QuitAndStartGame()
	{
		ProcessStartInfo processStartInfo = new ProcessStartInfo();
		processStartInfo.Arguments = launchFlags();
		if (Application.platform.ToString().Contains("indows") && !File.Exists(ApplicationRootPath() + "/../../appmanifest_220200.acf"))
		{
			processStartInfo.Verb = "runas";
		}
		processStartInfo.FileName = exePath;
		processStartInfo.UseShellExecute = true;
		processStartInfo.CreateNoWindow = true;
		Process.Start(processStartInfo);
		Application.Quit();
	}

	private string launchFlags()
	{
		string text = "";
		if (advSet.extChar)
		{
			text += "LC_ALL=C ";
		}
		if (advSet.fakeFullS)
		{
			text += "-popupwindow ";
		}
		if (advSet.noLog)
		{
			text += "-nolog ";
		}
		if (advSet.AMD)
		{
			text += "-force-gfx-st ";
		}
		UnityEngine.Debug.Log("Arguments: " + text);
		return text;
	}

	private void ChangeTexture(int state)
	{
		switch (state)
		{
		case 0:
			GetComponent<Renderer>().material.mainTexture = playOff;
			break;
		case 1:
			GetComponent<Renderer>().material.mainTexture = playOn;
			break;
		}
	}

	private void OnMouseEnter()
	{
		if (allowStartGame && Application.platform != RuntimePlatform.LinuxPlayer)
		{
			Cursor.SetCursor(cursorTexture, new Vector2(5f, 0f), CursorMode.Auto);
		}
	}

	private void OnMouseExit()
	{
		if (allowStartGame && Application.platform != RuntimePlatform.LinuxPlayer)
		{
			Cursor.SetCursor(null, Vector2.zero, CursorMode.Auto);
		}
	}

	private void OnMouseDown()
	{
		if (allowStartGame)
		{
			QuitAndStartGame();
		}
	}

	public string ApplicationRootPath()
	{
		if (Application.platform != RuntimePlatform.OSXPlayer)
		{
			return Application.dataPath + "/../";
		}
		return Application.dataPath + "/../../";
	}
}
