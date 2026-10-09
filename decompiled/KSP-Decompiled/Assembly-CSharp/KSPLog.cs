using System;
using System.IO;
using UnityEngine;

public class KSPLog : MonoBehaviour
{
	public delegate void OnMemoryLogUpdated(string log);

	[SerializeField]
	private bool logToFile = true;

	[SerializeField]
	private string logFilename = "KSP.log";

	[SerializeField]
	private int flushEvery = 20;

	private int flushCount;

	private StreamWriter fileStream;

	private bool localTimeZoneFailed;

	[SerializeField]
	private bool logToMemory = true;

	[SerializeField]
	private int memoryLogSize = 128;

	private string[] memoryLog;

	private int memoryLogIndex;

	private int memoryLogLength;

	private string lastError = "No errors or exceptions have been thrown.";

	private string lastStackTrace = "N/A";

	public OnMemoryLogUpdated onMemoryLogUpdated;

	private static Application.LogCallback logCallback;

	public static KSPLog Instance { get; private set; }

	public int MemoryLogSize
	{
		get
		{
			return memoryLogSize;
		}
		set
		{
			memoryLogSize = value;
			memoryLog = new string[memoryLogSize];
			memoryLogLength = 0;
			memoryLogIndex = 0;
		}
	}

	public string[] MemoryLog => memoryLog;

	public int MemoryLogIndex => memoryLogIndex;

	public int MemoryLogLength => memoryLogLength;

	public string LastError => lastError;

	public string LastStackTrace => lastStackTrace;

	private void Awake()
	{
		if (Instance != null && Instance != this)
		{
			Debug.LogError("KSPLog: Instance already exists");
			UnityEngine.Object.Destroy(this);
		}
		else
		{
			Instance = this;
		}
	}

	private void Start()
	{
		if (base.transform == base.transform.root)
		{
			UnityEngine.Object.DontDestroyOnLoad(base.gameObject);
		}
		Application.logMessageReceivedThreaded += LogCallback;
		if (logToFile)
		{
			AddLogCallback(LogToFile);
			flushCount = 0;
			fileStream = new StreamWriter(logFilename, append: false);
			LogHeader();
		}
		if (logToMemory)
		{
			AddLogCallback(LogToMemory);
			MemoryLogSize = memoryLogSize;
			memoryLog = new string[memoryLogSize];
		}
		if (GameSettings.LOG_ERRORS_TO_SCREEN || GameSettings.LOG_EXCEPTIONS_TO_SCREEN)
		{
			AddLogCallback(LogToScreen);
		}
	}

	private void OnDestroy()
	{
		if (fileStream != null)
		{
			fileStream.Flush();
			fileStream.Close();
		}
		if (memoryLog != null)
		{
			memoryLog = null;
		}
		if (Instance != null && Instance == this)
		{
			Instance = null;
		}
	}

	private void LogHeader()
	{
		if (VersioningBase.Instance != null)
		{
			Debug.Log("******* Log Initiated for " + Versioning.Title + " - " + Versioning.VersionString + " " + Versioning.Language + " *******");
			fileStream.WriteLine(Versioning.Title + " - " + Versioning.VersionString + " " + Versioning.Language);
			fileStream.WriteLine();
			fileStream.WriteLine();
		}
		else
		{
			Debug.Log("******* Log Initiated *******");
		}
		fileStream.WriteLine("OS: " + SystemInfo.operatingSystem);
		fileStream.WriteLine("CPU: " + SystemInfo.processorType + " (" + SystemInfo.processorCount + ")");
		fileStream.WriteLine("RAM: " + SystemInfo.systemMemorySize);
		fileStream.WriteLine("GPU: " + SystemInfo.graphicsDeviceName + " (" + SystemInfo.graphicsMemorySize + "MB)");
		fileStream.WriteLine("SM: " + SystemInfo.graphicsShaderLevel + " (" + SystemInfo.graphicsDeviceVersion + ")");
		LogRenderTextureFormats();
		fileStream.WriteLine();
		fileStream.WriteLine();
		fileStream.WriteLine("Log started: " + KSPUtil.SystemDateTime.DateTimeNow().ToString("ddd, MMM dd, yyyy HH:mm:ss"));
		fileStream.WriteLine();
		fileStream.WriteLine();
		fileStream.Flush();
	}

	private void LogRenderTextureFormats()
	{
		RenderTextureFormat[] array = (RenderTextureFormat[])Enum.GetValues(typeof(RenderTextureFormat));
		string text = "";
		int i = 0;
		for (int num = array.Length; i < num; i++)
		{
			if (SystemInfo.SupportsRenderTextureFormat(array[i]))
			{
				if (text != string.Empty)
				{
					text += ", ";
				}
				text += array[i];
			}
		}
		fileStream.WriteLine("RT Formats: " + text);
	}

	private void LogCallback(string logString, string stackTrace, LogType type)
	{
		if (logCallback != null)
		{
			logCallback(logString, stackTrace, type);
		}
	}

	private void LogToFile(string logString, string stackTrace, LogType type)
	{
		string text = TimeString();
		try
		{
			switch (type)
			{
			case LogType.Error:
				fileStream.WriteLine("[ERR " + text + "] " + logString);
				fileStream.WriteLine(ConvertLineEndings(stackTrace, "\t"));
				lastError = "Error: " + logString;
				lastStackTrace = stackTrace;
				break;
			case LogType.Assert:
				fileStream.WriteLine("[ASR " + text + "] " + logString);
				break;
			case LogType.Warning:
				fileStream.WriteLine("[WRN " + text + "] " + logString);
				break;
			case LogType.Log:
				fileStream.WriteLine("[LOG " + text + "] " + logString);
				break;
			case LogType.Exception:
				fileStream.WriteLine("[EXC " + text + "] " + logString);
				fileStream.WriteLine(ConvertLineEndings(stackTrace, "\t"));
				lastError = "Exception: " + logString;
				lastStackTrace = stackTrace;
				break;
			}
			if (GameSettings.LOG_INSTANT_FLUSH || flushCount >= flushEvery)
			{
				fileStream.Flush();
				flushCount = -1;
			}
			flushCount++;
		}
		catch (Exception ex)
		{
			if (!(ex is ObjectDisposedException))
			{
				Debug.LogError(ex.Message);
			}
		}
	}

	private void LogToMemory(string logString, string stackTrace, LogType type)
	{
		if (memoryLog != null)
		{
			string text;
			switch (type)
			{
			case LogType.Error:
				text = (lastError = "Error: " + logString);
				lastStackTrace = stackTrace;
				break;
			case LogType.Assert:
				text = "Assert: " + logString;
				break;
			case LogType.Warning:
				text = "Warning: " + logString;
				break;
			default:
				text = logString;
				break;
			case LogType.Exception:
				text = (lastError = "Exception: " + logString);
				lastStackTrace = stackTrace;
				break;
			}
			memoryLog[memoryLogIndex] = text;
			if (memoryLogLength < memoryLogSize)
			{
				memoryLogLength++;
			}
			memoryLogIndex++;
			if (memoryLogIndex == memoryLogSize)
			{
				memoryLogIndex = 0;
			}
			if (onMemoryLogUpdated != null)
			{
				onMemoryLogUpdated(text);
			}
		}
	}

	private void LogToScreen(string logString, string stackTrace, LogType type)
	{
		if (((type == LogType.Error && GameSettings.LOG_ERRORS_TO_SCREEN) || (type == LogType.Exception && GameSettings.LOG_EXCEPTIONS_TO_SCREEN)) && !(ScreenMessages.Instance == null))
		{
			ScreenMessages.PostScreenMessage(string.Format("<size=-3><color={0}>{1}</color></size>", (type == LogType.Error) ? "orange" : "red", logString), 5f, ScreenMessageStyle.UPPER_RIGHT);
		}
	}

	private static string ConvertLineEndings(string input, string addToStart = "")
	{
		return (addToStart + input).Replace("\n", "\r\n" + addToStart).TrimEnd();
	}

	private string TimeString()
	{
		return KSPUtil.SystemDateTime.DateTimeNow().ToString("HH:mm:ss.fff");
	}

	public static void AddLogCallback(Application.LogCallback method)
	{
		logCallback = (Application.LogCallback)Delegate.Combine(logCallback, method);
	}

	public static void RemoveLogCallback(Application.LogCallback method)
	{
		logCallback = (Application.LogCallback)Delegate.Remove(logCallback, method);
	}

	public void SetScreenLogging(bool logToScreen)
	{
		if (logToScreen)
		{
			AddLogCallback(LogToScreen);
		}
		else
		{
			RemoveLogCallback(LogToScreen);
		}
	}
}
