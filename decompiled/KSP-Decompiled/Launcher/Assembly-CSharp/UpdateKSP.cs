using System;
using System.Collections;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Net.Security;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using ICSharpCode.SharpZipLib.Zip;
using UnityEngine;
using UnityEngine.UI;

public class UpdateKSP : MonoBehaviour
{
	private int buildID;

	public bool isSteam;

	public bool isExp;

	public Vector3 startPosition = new Vector3(33.44f, -85.6f, 49.84611f);

	public bool patcherIsDoneDownloading;

	private string gameVersionURL = "https://kerbalspaceprogram.com/files/KSP_LATEST_VERSION.vrs";

	private string testURL;

	public bool test;

	public Texture2D updateOn;

	public Texture2D updateOff;

	public Play startGame;

	public Settings setting;

	public GameObject check;

	public TextMesh checktext;

	public Text versionText;

	private string checkstring;

	private bool needsUpdate;

	private bool allowUpdate;

	private bool isChecking;

	private Process Proc;

	public AdvancedSettings advSet;

	private string patcherHashURL = "https://kerbalspaceprogram.com/kspstore/dp/patcher/index.php";

	private string patcherDLRoot = "https://kerbalspaceprogram.com/kspstore/dp/patcher/releases/{0}/{1}/Patcher";

	public string updateInstructions = "https://www.kerbalspaceprogram.com/instructions.php";

	private bool checkAgainUpdates;

	private int latestBuildID;

	public Texture2D cursorTexture;

	private void Awake()
	{
		UnityEngine.Object.DontDestroyOnLoad(base.gameObject);
		UnityEngine.Object.DontDestroyOnLoad(check.gameObject);
		if (GameObject.FindGameObjectsWithTag("Update").Length > 1)
		{
			UnityEngine.Object.Destroy(base.gameObject);
		}
		base.gameObject.transform.position = startPosition;
		if (!isSteam)
		{
			isSteam = File.Exists(ApplicationRootPath() + "/../../appmanifest_220200.acf");
		}
		advSet = GameObject.Find("AdvSettings").GetComponent<AdvancedSettings>();
		patcherIsDoneDownloading = false;
		if (File.Exists(ApplicationRootPath() + "/buildID.txt"))
		{
			string[] array = File.ReadAllLines(ApplicationRootPath() + "/buildID.txt");
			foreach (string text in array)
			{
				if (text.Contains("="))
				{
					string[] array2 = text.Split('=');
					if (array2.Length == 2)
					{
						buildID = int.Parse(array2[1].Trim());
						MonoBehaviour.print("Local BuildID: " + buildID);
						break;
					}
				}
			}
		}
		else if (File.Exists(ApplicationRootPath() + "/buildID64.txt"))
		{
			string[] array = File.ReadAllLines(ApplicationRootPath() + "/buildID64.txt");
			foreach (string text2 in array)
			{
				if (text2.Contains("="))
				{
					string[] array3 = text2.Split('=');
					if (array3.Length == 2)
					{
						buildID = int.Parse(array3[1].Trim());
						MonoBehaviour.print("Local BuildID: " + buildID);
						break;
					}
				}
			}
		}
		else
		{
			buildID = -1;
		}
	}

	private void OnLevelWasLoaded(int level)
	{
		Vector3 position = startPosition;
		if (level == 1)
		{
			position.z = -60f;
		}
		base.gameObject.transform.position = position;
	}

	private void Start()
	{
		needsUpdate = false;
		StartCoroutine(CheckUpdate());
		if (isSteam)
		{
			patcherIsDoneDownloading = true;
			startGame.AllowStartGame(allow: true);
			setting.allowGoToSettings = true;
			UnityEngine.Debug.Log("Allowed Start game");
			return;
		}
		allowUpdate = false;
		if (!advSet.already)
		{
			isChecking = true;
			check.SetActive(value: true);
			StartCoroutine(CheckPatcher());
		}
	}

	private void Update()
	{
		if (!isChecking)
		{
			check.SetActive(value: false);
		}
		else
		{
			UpdateCheckText();
		}
		if (allowUpdate)
		{
			ChangeTexture(1);
		}
	}

	private void FixedUpdate()
	{
		if (checkAgainUpdates)
		{
			checkAgainUpdates = false;
			StartCoroutine(CheckPatcher());
		}
	}

	private IEnumerator CheckUpdate()
	{
		checkstring = "Checking for Updates...";
		testURL = ApplicationRootPath() + "KSP_LATEST_VERSION.vrs";
		if (File.Exists(testURL))
		{
			test = true;
		}
		WWW post = new WWW(test ? ("file://" + testURL) : gameVersionURL);
		yield return post;
		if (post.error != null)
		{
			MonoBehaviour.print("Error connecting to KSP home. Unable to fetch version info. \nmsg: " + post.error);
			UnityEngine.Object.Destroy(base.gameObject.GetComponent<UpdateKSP>());
			yield break;
		}
		string[] array = post.text.Split('\n');
		foreach (string text in array)
		{
			if (text.Contains("="))
			{
				string text2 = text.Split('=')[0].Trim();
				string s = text.Split('=')[1].Trim();
				if (text2 == "BUILD")
				{
					latestBuildID = int.Parse(s);
					MonoBehaviour.print("Last BuildID: " + latestBuildID);
					break;
				}
			}
		}
		versionText.text = "Installed version: " + buildID + "\nMost recent version: " + latestBuildID;
		if (buildID < latestBuildID)
		{
			allowUpdate = true;
		}
	}

	private IEnumerator CheckPatcher()
	{
		bool onAMac = Application.platform.ToString().Contains("OSX");
		bool onLinux = Application.platform == RuntimePlatform.LinuxPlayer;
		string patcherPath = ((!onLinux) ? (ApplicationRootPath() + "Patcher.zip") : (ApplicationRootPath() + "Patcher.tar.bz"));
		bool dlPatcher = !File.Exists(patcherPath);
		if (!dlPatcher)
		{
			patcherIsDoneDownloading = true;
			startGame.AllowStartGame(allow: true);
			setting.allowGoToSettings = true;
			WWWForm wWWForm = new WWWForm();
			wWWForm.AddField("act", "patchermd5");
			wWWForm.AddField("experimental", "0");
			wWWForm.AddField("platID", GetOSCode());
			string text = patcherHashURL;
			MonoBehaviour.print("Checking patcher at " + text);
			WWW patcherChecker = new WWW(text, wWWForm);
			yield return patcherChecker;
			if (patcherChecker.error != null)
			{
				MonoBehaviour.print("Connecting to patcher updater: " + patcherChecker.error);
				isChecking = false;
				allowUpdate = true;
				yield break;
			}
			FastZip fastZip = new FastZip();
			try
			{
				fastZip.ExtractZip(patcherPath, ApplicationRootPath(), null);
			}
			catch (Exception ex)
			{
				UnityEngine.Debug.Log("Something went wrong unpacking the file, reason: " + ex.ToString());
			}
			string text2 = ((onAMac || onLinux) ? "Patcher" : "Patcher.exe");
			string text3 = patcherChecker.text;
			string text4 = FileMD5String(text2);
			if (text3.Contains("<"))
			{
				MonoBehaviour.print("Server returned an error: " + text3);
			}
			else
			{
				MonoBehaviour.print("[Updater] Server reports that Patcher has MD5 of " + text3);
				MonoBehaviour.print("[Updater] Our own copy of " + text2 + " has MD5 of " + text4);
				if (!text3.Equals(text4))
				{
					MonoBehaviour.print("[Updater]  Well, they don't match.  Redownloading...");
					dlPatcher = true;
				}
				else
				{
					MonoBehaviour.print("ALLOW UPDATE? " + needsUpdate);
					if (needsUpdate || !startGame.kspExists)
					{
						allowUpdate = true;
					}
					isChecking = false;
				}
			}
		}
		else
		{
			MonoBehaviour.print("[Updater] Patcher does NOT exist! Redownloading.");
			dlPatcher = true;
		}
		if (dlPatcher)
		{
			DownloadPatcher();
		}
		else if (!startGame.kspExists)
		{
			allowUpdate = true;
		}
	}

	private void DownloadPatcher()
	{
		checkstring = "Downloading Patcher...";
		startGame.AllowStartGame(allow: false);
		setting.allowGoToSettings = false;
		try
		{
			ServicePointManager.ServerCertificateValidationCallback = (object _003Cp0_003E, X509Certificate _003Cp1_003E, X509Chain _003Cp2_003E, SslPolicyErrors _003Cp3_003E) => true;
		}
		catch (Exception message)
		{
			MonoBehaviour.print(message);
		}
		WebClient webClient = new WebClient();
		webClient.DownloadFileCompleted += PatcherDownloaded;
		webClient.DownloadProgressChanged += DisplayDownloadProgress;
		string text = ApplicationRootPath() + "Patcher";
		string text2 = ((!(GetOSCode() == "linux")) ? ".zip" : ".tar.gz");
		string text3 = ((!isExp) ? (string.Format(patcherDLRoot, "production", GetOSCode()) + text2) : (string.Format(patcherDLRoot, "experimentals", GetOSCode()) + text2));
		text += text2;
		MonoBehaviour.print(text3 + " -> " + text);
		webClient.DownloadFileAsync(new Uri(text3), text);
	}

	private IEnumerator StartPatcher()
	{
		bool flag = false;
		startGame.AllowStartGame(allow: false);
		setting.allowGoToSettings = false;
		patcherIsDoneDownloading = false;
		bool flag2 = Application.platform.ToString().Contains("OSX");
		bool flag3 = Application.platform == RuntimePlatform.LinuxPlayer;
		if (!File.Exists(ApplicationRootPath() + "Patcher"))
		{
			FastZip fastZip = new FastZip();
			try
			{
				fastZip.ExtractZip(ApplicationRootPath() + "Patcher.zip", ApplicationRootPath(), null);
			}
			catch (Exception ex)
			{
				UnityEngine.Debug.Log("Something went wrong unpacking the file, reason: " + ex.ToString());
			}
		}
		if (flag2 || flag3)
		{
			Process process = new Process();
			process.StartInfo.FileName = "chmod";
			process.StartInfo.Arguments = "775 " + ApplicationRootPath() + "Patcher";
			process.StartInfo.Verb = "runas";
			process.Start();
		}
		if (flag2)
		{
			InstallDependencies();
		}
		try
		{
			Proc = new Process();
			if (flag2 || flag3)
			{
				Proc.StartInfo.FileName = "Patcher";
			}
			else
			{
				Proc.StartInfo.FileName = "Patcher.exe";
			}
			Proc.Start();
		}
		catch (Exception ex2)
		{
			MonoBehaviour.print("Could not StartPatcher. The patch client could not be found or has no execution permission.\n\n" + ex2);
			startGame.AllowStartGame(allow: true);
			setting.allowGoToSettings = true;
			patcherIsDoneDownloading = true;
			checkAgainUpdates = true;
			flag = true;
		}
		if (!flag)
		{
			while (!Proc.HasExited)
			{
				yield return new WaitForSeconds(2f);
			}
		}
		MonoBehaviour.print("Patcher exited!");
		startGame.AllowStartGame(allow: true);
		setting.allowGoToSettings = true;
		patcherIsDoneDownloading = true;
		checkAgainUpdates = true;
	}

	public void InstallDependencies()
	{
		UnityEngine.Debug.Log("Installing dependencies");
		bool flag = false;
		bool flag2 = false;
		Process process = new Process();
		process.StartInfo.FileName = "sh";
		process.StartInfo.Arguments = "/usr/local/bin/brew list";
		process.StartInfo.UseShellExecute = false;
		process.StartInfo.Verb = "";
		process.StartInfo.RedirectStandardOutput = true;
		process.StartInfo.RedirectStandardError = true;
		process.Start();
		string text = process.StandardOutput.ReadToEnd();
		string text2 = process.StandardError.ReadToEnd();
		process.WaitForExit();
		UnityEngine.Debug.Log(text);
		UnityEngine.Debug.Log(text2);
		if (text.Contains("pyqt"))
		{
			flag = true;
			flag2 = true;
		}
		else if (text.Contains("not found") || text2.Contains("not found") || text.Contains("such file") || text2.Contains("such file"))
		{
			flag = false;
			flag2 = false;
		}
		else
		{
			flag = true;
			flag2 = false;
		}
		if (!flag)
		{
			string text3 = "\"ruby -e \\\"$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)\\\"";
			UnityEngine.Debug.Log("Attempting to instal HOMEBREW " + text3);
			Process process2 = new Process();
			process2.StartInfo.FileName = "osascript";
			process2.StartInfo.Arguments = "-e 'do shell script " + text3 + "\"'";
			process2.StartInfo.Verb = "runas";
			process2.StartInfo.CreateNoWindow = false;
			process2.Start();
			process2.WaitForExit();
		}
		if (!flag2)
		{
			UnityEngine.Debug.Log("Attempting to instal PYQT");
			process.StartInfo.FileName = "sh";
			process.StartInfo.Arguments = "/usr/local/bin/brew install pyqt";
			process.StartInfo.Verb = "runas";
			process.StartInfo.UseShellExecute = false;
			process.StartInfo.RedirectStandardOutput = true;
			process.StartInfo.RedirectStandardError = true;
			process.Start();
			string message = process.StandardOutput.ReadToEnd();
			string message2 = process.StandardError.ReadToEnd();
			process.WaitForExit();
			UnityEngine.Debug.Log(message);
			UnityEngine.Debug.Log(message2);
		}
	}

	private string GetOSCode()
	{
		if (Application.platform.ToString().Contains("Windows"))
		{
			return "win";
		}
		if (Application.platform.ToString().Contains("OSX"))
		{
			return "osx";
		}
		return "linux";
	}

	public static byte[] FileMD5Bytes(string file)
	{
		return new MD5CryptoServiceProvider().ComputeHash(File.ReadAllBytes(file));
	}

	public static string FileMD5String(string file)
	{
		return BytesToHex(FileMD5Bytes(file));
	}

	public static string BytesToHex(byte[] b)
	{
		string text = string.Empty;
		for (int i = 0; i < b.Length; i++)
		{
			byte b2 = b[i];
			text = ((b2 >= 16) ? (text + b2.ToString("x")) : (text + "0" + b2.ToString("x")));
		}
		return text;
	}

	private void PatcherDownloaded(object sender, AsyncCompletedEventArgs e)
	{
		if (e.Error != null)
		{
			MonoBehaviour.print(e.Error);
		}
		else
		{
			MonoBehaviour.print("Downloaded fresh patcher!");
			patcherIsDoneDownloading = true;
		}
		isChecking = false;
		setting.allowGoToSettings = true;
		if (needsUpdate || !startGame.kspExists)
		{
			allowUpdate = true;
		}
	}

	private void DisplayDownloadProgress(object sender, DownloadProgressChangedEventArgs e)
	{
		checkstring = "Downloading Patcher...  " + e.ProgressPercentage + "%";
	}

	private void OnMouseEnter()
	{
		if (allowUpdate && Application.platform != RuntimePlatform.LinuxPlayer)
		{
			Cursor.SetCursor(cursorTexture, new Vector2(5f, 0f), CursorMode.Auto);
		}
	}

	private void OnMouseExit()
	{
		if (allowUpdate && Application.platform != RuntimePlatform.LinuxPlayer)
		{
			Cursor.SetCursor(null, Vector2.zero, CursorMode.Auto);
		}
	}

	private void OnMouseDown()
	{
		if (allowUpdate)
		{
			Application.OpenURL(updateInstructions);
		}
	}

	private void ChangeTexture(int state)
	{
		switch (state)
		{
		case 0:
			GetComponent<Renderer>().material.mainTexture = updateOff;
			break;
		case 1:
			GetComponent<Renderer>().material.mainTexture = updateOn;
			break;
		}
	}

	private void UpdateCheckText()
	{
		checktext.text = checkstring;
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
