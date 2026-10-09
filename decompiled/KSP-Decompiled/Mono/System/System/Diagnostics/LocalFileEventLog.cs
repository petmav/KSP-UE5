using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Security;
using System.Text;
using System.Threading;

namespace System.Diagnostics;

internal class LocalFileEventLog : EventLogImpl
{
	private const string DateFormat = "yyyyMMddHHmmssfff";

	private static readonly object lockObject = new object();

	private FileSystemWatcher file_watcher;

	private int last_notification_index;

	private bool _notifying;

	private bool RunningOnUnix
	{
		get
		{
			int platform = (int)Environment.OSVersion.Platform;
			if (platform != 4 && platform != 128)
			{
				return platform == 6;
			}
			return true;
		}
	}

	private string EventLogStore
	{
		get
		{
			string environmentVariable = Environment.GetEnvironmentVariable("MONO_EVENTLOG_TYPE");
			if (environmentVariable != null && environmentVariable.Length > "local".Length + 1)
			{
				return environmentVariable.Substring("local".Length + 1);
			}
			if (RunningOnUnix)
			{
				return "/var/lib/mono/eventlog";
			}
			return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData), "mono\\eventlog");
		}
	}

	public override OverflowAction OverflowAction => OverflowAction.DoNotOverwrite;

	public override int MinimumRetentionDays => int.MaxValue;

	public override long MaximumKilobytes
	{
		get
		{
			return long.MaxValue;
		}
		set
		{
			throw new NotSupportedException("This EventLog implementation does not support setting max kilobytes policy");
		}
	}

	public LocalFileEventLog(EventLog coreEventLog)
		: base(coreEventLog)
	{
	}

	public override void BeginInit()
	{
	}

	public override void Clear()
	{
		string path = FindLogStore(base.CoreEventLog.Log);
		if (Directory.Exists(path))
		{
			string[] files = Directory.GetFiles(path, "*.log");
			for (int i = 0; i < files.Length; i++)
			{
				File.Delete(files[i]);
			}
		}
	}

	public override void Close()
	{
		if (file_watcher != null)
		{
			file_watcher.EnableRaisingEvents = false;
			file_watcher = null;
		}
	}

	public override void CreateEventSource(EventSourceCreationData sourceData)
	{
		string text = FindLogStore(sourceData.LogName);
		if (!Directory.Exists(text))
		{
			ValidateCustomerLogName(sourceData.LogName, sourceData.MachineName);
			Directory.CreateDirectory(text);
			Directory.CreateDirectory(Path.Combine(text, sourceData.LogName));
			if (RunningOnUnix)
			{
				ModifyAccessPermissions(text, "777");
				ModifyAccessPermissions(text, "+t");
			}
		}
		Directory.CreateDirectory(Path.Combine(text, sourceData.Source));
	}

	public override void Delete(string logName, string machineName)
	{
		string path = FindLogStore(logName);
		if (!Directory.Exists(path))
		{
			throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "Event Log '{0}' does not exist on computer '{1}'.", logName, machineName));
		}
		Directory.Delete(path, recursive: true);
	}

	public override void DeleteEventSource(string source, string machineName)
	{
		if (!Directory.Exists(EventLogStore))
		{
			throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "The source '{0}' is not registered on computer '{1}'.", source, machineName));
		}
		Directory.Delete(FindSourceDirectory(source) ?? throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "The source '{0}' is not registered on computer '{1}'.", source, machineName)));
	}

	public override void Dispose(bool disposing)
	{
		Close();
	}

	public override void DisableNotification()
	{
		if (file_watcher != null)
		{
			file_watcher.EnableRaisingEvents = false;
		}
	}

	public override void EnableNotification()
	{
		if (file_watcher == null)
		{
			string path = FindLogStore(base.CoreEventLog.Log);
			if (!Directory.Exists(path))
			{
				Directory.CreateDirectory(path);
			}
			file_watcher = new FileSystemWatcher();
			file_watcher.Path = path;
			file_watcher.Created += delegate
			{
				lock (this)
				{
					if (_notifying)
					{
						return;
					}
					_notifying = true;
				}
				Thread.Sleep(100);
				try
				{
					while (GetLatestIndex() > last_notification_index)
					{
						try
						{
							base.CoreEventLog.OnEntryWritten(GetEntry(last_notification_index++));
						}
						catch (Exception)
						{
						}
					}
				}
				finally
				{
					lock (this)
					{
						_notifying = false;
					}
				}
			};
		}
		last_notification_index = GetLatestIndex();
		file_watcher.EnableRaisingEvents = true;
	}

	public override void EndInit()
	{
	}

	public override bool Exists(string logName, string machineName)
	{
		return Directory.Exists(FindLogStore(logName));
	}

	[System.MonoTODO("Use MessageTable from PE for lookup")]
	protected override string FormatMessage(string source, uint eventID, string[] replacementStrings)
	{
		return string.Join(", ", replacementStrings);
	}

	protected override int GetEntryCount()
	{
		string path = FindLogStore(base.CoreEventLog.Log);
		if (!Directory.Exists(path))
		{
			return 0;
		}
		return Directory.GetFiles(path, "*.log").Length;
	}

	protected override EventLogEntry GetEntry(int index)
	{
		string path = Path.Combine(FindLogStore(base.CoreEventLog.Log), (index + 1).ToString(CultureInfo.InvariantCulture) + ".log");
		using TextReader textReader = File.OpenText(path);
		int index2 = int.Parse(Path.GetFileNameWithoutExtension(path), CultureInfo.InvariantCulture);
		uint num = uint.Parse(textReader.ReadLine().Substring(12), CultureInfo.InvariantCulture);
		EventLogEntryType entryType = (EventLogEntryType)Enum.Parse(typeof(EventLogEntryType), textReader.ReadLine().Substring(11));
		string source = textReader.ReadLine().Substring(8);
		string text = textReader.ReadLine().Substring(10);
		short categoryNumber = short.Parse(text, CultureInfo.InvariantCulture);
		string category = "(" + text + ")";
		DateTime timeGenerated = DateTime.ParseExact(textReader.ReadLine().Substring(15), "yyyyMMddHHmmssfff", CultureInfo.InvariantCulture);
		DateTime lastWriteTime = File.GetLastWriteTime(path);
		int num2 = int.Parse(textReader.ReadLine().Substring(20));
		List<string> list = new List<string>();
		StringBuilder stringBuilder = new StringBuilder();
		while (list.Count < num2)
		{
			char c = (char)textReader.Read();
			if (c == '\0')
			{
				list.Add(stringBuilder.ToString());
				stringBuilder.Length = 0;
			}
			else
			{
				stringBuilder.Append(c);
			}
		}
		string[] replacementStrings = list.ToArray();
		string message = FormatMessage(source, num, replacementStrings);
		int eventID = EventLog.GetEventID(num);
		byte[] data = Convert.FromBase64String(textReader.ReadToEnd());
		return new EventLogEntry(category, categoryNumber, index2, eventID, source, message, null, Environment.MachineName, entryType, timeGenerated, lastWriteTime, data, replacementStrings, num);
	}

	[System.MonoTODO]
	protected override string GetLogDisplayName()
	{
		return base.CoreEventLog.Log;
	}

	protected override string[] GetLogNames(string machineName)
	{
		if (!Directory.Exists(EventLogStore))
		{
			return new string[0];
		}
		string[] directories = Directory.GetDirectories(EventLogStore, "*");
		string[] array = new string[directories.Length];
		for (int i = 0; i < directories.Length; i++)
		{
			array[i] = Path.GetFileName(directories[i]);
		}
		return array;
	}

	public override string LogNameFromSourceName(string source, string machineName)
	{
		if (!Directory.Exists(EventLogStore))
		{
			return string.Empty;
		}
		string text = FindSourceDirectory(source);
		if (text == null)
		{
			return string.Empty;
		}
		return new DirectoryInfo(text).Parent.Name;
	}

	public override bool SourceExists(string source, string machineName)
	{
		if (!Directory.Exists(EventLogStore))
		{
			return false;
		}
		return FindSourceDirectory(source) != null;
	}

	public override void WriteEntry(string[] replacementStrings, EventLogEntryType type, uint instanceID, short category, byte[] rawData)
	{
		lock (lockObject)
		{
			string path = Path.Combine(FindLogStore(base.CoreEventLog.Log), (GetLatestIndex() + 1).ToString(CultureInfo.InvariantCulture) + ".log");
			try
			{
				using TextWriter textWriter = File.CreateText(path);
				textWriter.WriteLine("InstanceID: {0}", instanceID.ToString(CultureInfo.InvariantCulture));
				textWriter.WriteLine("EntryType: {0}", (int)type);
				textWriter.WriteLine("Source: {0}", base.CoreEventLog.Source);
				textWriter.WriteLine("Category: {0}", category.ToString(CultureInfo.InvariantCulture));
				textWriter.WriteLine("TimeGenerated: {0}", DateTime.Now.ToString("yyyyMMddHHmmssfff", CultureInfo.InvariantCulture));
				textWriter.WriteLine("ReplacementStrings: {0}", replacementStrings.Length.ToString(CultureInfo.InvariantCulture));
				StringBuilder stringBuilder = new StringBuilder();
				foreach (string value in replacementStrings)
				{
					stringBuilder.Append(value);
					stringBuilder.Append('\0');
				}
				textWriter.Write(stringBuilder.ToString());
				textWriter.Write(Convert.ToBase64String(rawData));
			}
			catch (IOException)
			{
				File.Delete(path);
			}
		}
	}

	private string FindSourceDirectory(string source)
	{
		string result = null;
		string[] directories = Directory.GetDirectories(EventLogStore, "*");
		for (int i = 0; i < directories.Length; i++)
		{
			string[] directories2 = Directory.GetDirectories(directories[i], "*");
			for (int j = 0; j < directories2.Length; j++)
			{
				if (string.Compare(Path.GetFileName(directories2[j]), source, ignoreCase: true, CultureInfo.InvariantCulture) == 0)
				{
					result = directories2[j];
					break;
				}
			}
		}
		return result;
	}

	private string FindLogStore(string logName)
	{
		if (!Directory.Exists(EventLogStore))
		{
			return Path.Combine(EventLogStore, logName);
		}
		string[] directories = Directory.GetDirectories(EventLogStore, "*");
		for (int i = 0; i < directories.Length; i++)
		{
			if (string.Compare(Path.GetFileName(directories[i]), logName, ignoreCase: true, CultureInfo.InvariantCulture) == 0)
			{
				return directories[i];
			}
		}
		return Path.Combine(EventLogStore, logName);
	}

	private int GetLatestIndex()
	{
		int num = 0;
		string[] files = Directory.GetFiles(FindLogStore(base.CoreEventLog.Log), "*.log");
		for (int i = 0; i < files.Length; i++)
		{
			try
			{
				int num2 = int.Parse(Path.GetFileNameWithoutExtension(files[i]), CultureInfo.InvariantCulture);
				if (num2 > num)
				{
					num = num2;
				}
			}
			catch
			{
			}
		}
		return num;
	}

	private static void ModifyAccessPermissions(string path, string permissions)
	{
		ProcessStartInfo processStartInfo = new ProcessStartInfo();
		processStartInfo.FileName = "chmod";
		processStartInfo.RedirectStandardOutput = true;
		processStartInfo.RedirectStandardError = true;
		processStartInfo.UseShellExecute = false;
		processStartInfo.Arguments = $"{permissions} \"{path}\"";
		Process process = null;
		try
		{
			process = Process.Start(processStartInfo);
		}
		catch (Exception inner)
		{
			throw new SecurityException("Access permissions could not be modified.", inner);
		}
		process.WaitForExit();
		if (process.ExitCode != 0)
		{
			process.Close();
			throw new SecurityException("Access permissions could not be modified.");
		}
		process.Close();
	}

	public override void ModifyOverflowPolicy(OverflowAction action, int retentionDays)
	{
		throw new NotSupportedException("This EventLog implementation does not support modifying overflow policy");
	}

	public override void RegisterDisplayName(string resourceFile, long resourceId)
	{
		throw new NotSupportedException("This EventLog implementation does not support registering display name");
	}
}
