using System.Globalization;

namespace System.Diagnostics;

internal abstract class EventLogImpl
{
	private readonly EventLog _coreEventLog;

	protected EventLog CoreEventLog => _coreEventLog;

	public int EntryCount
	{
		get
		{
			if (_coreEventLog.Log == null || _coreEventLog.Log.Length == 0)
			{
				throw new ArgumentException("Log property is not set.");
			}
			if (!EventLog.Exists(_coreEventLog.Log, _coreEventLog.MachineName))
			{
				throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "The event log '{0}' on  computer '{1}' does not exist.", _coreEventLog.Log, _coreEventLog.MachineName));
			}
			return GetEntryCount();
		}
	}

	public EventLogEntry this[int index]
	{
		get
		{
			if (_coreEventLog.Log == null || _coreEventLog.Log.Length == 0)
			{
				throw new ArgumentException("Log property is not set.");
			}
			if (!EventLog.Exists(_coreEventLog.Log, _coreEventLog.MachineName))
			{
				throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "The event log '{0}' on  computer '{1}' does not exist.", _coreEventLog.Log, _coreEventLog.MachineName));
			}
			if (index < 0 || index >= EntryCount)
			{
				throw new ArgumentException("Index out of range");
			}
			return GetEntry(index);
		}
	}

	public string LogDisplayName
	{
		get
		{
			if (_coreEventLog.Log != null && _coreEventLog.Log.Length == 0)
			{
				throw new InvalidOperationException("Event log names must consist of printable characters and cannot contain \\, *, ?, or spaces.");
			}
			if (_coreEventLog.Log != null)
			{
				if (_coreEventLog.Log.Length == 0)
				{
					return string.Empty;
				}
				if (!EventLog.Exists(_coreEventLog.Log, _coreEventLog.MachineName))
				{
					throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "Cannot find Log {0} on computer {1}.", _coreEventLog.Log, _coreEventLog.MachineName));
				}
			}
			return GetLogDisplayName();
		}
	}

	public abstract OverflowAction OverflowAction { get; }

	public abstract int MinimumRetentionDays { get; }

	public abstract long MaximumKilobytes { get; set; }

	protected EventLogImpl(EventLog coreEventLog)
	{
		_coreEventLog = coreEventLog;
	}

	public EventLogEntry[] GetEntries()
	{
		string log = CoreEventLog.Log;
		if (log == null || log.Length == 0)
		{
			throw new ArgumentException("Log property value has not been specified.");
		}
		if (!EventLog.Exists(log))
		{
			throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "The event log '{0}' on  computer '{1}' does not exist.", log, _coreEventLog.MachineName));
		}
		int entryCount = GetEntryCount();
		EventLogEntry[] array = new EventLogEntry[entryCount];
		for (int i = 0; i < entryCount; i++)
		{
			array[i] = GetEntry(i);
		}
		return array;
	}

	public abstract void DisableNotification();

	public abstract void EnableNotification();

	public abstract void BeginInit();

	public abstract void Clear();

	public abstract void Close();

	public abstract void CreateEventSource(EventSourceCreationData sourceData);

	public abstract void Delete(string logName, string machineName);

	public abstract void DeleteEventSource(string source, string machineName);

	public abstract void Dispose(bool disposing);

	public abstract void EndInit();

	public abstract bool Exists(string logName, string machineName);

	protected abstract int GetEntryCount();

	protected abstract EventLogEntry GetEntry(int index);

	public EventLog[] GetEventLogs(string machineName)
	{
		string[] logNames = GetLogNames(machineName);
		EventLog[] array = new EventLog[logNames.Length];
		for (int i = 0; i < logNames.Length; i++)
		{
			EventLog eventLog = new EventLog(logNames[i], machineName);
			array[i] = eventLog;
		}
		return array;
	}

	protected abstract string GetLogDisplayName();

	public abstract string LogNameFromSourceName(string source, string machineName);

	public abstract bool SourceExists(string source, string machineName);

	public abstract void WriteEntry(string[] replacementStrings, EventLogEntryType type, uint instanceID, short category, byte[] rawData);

	protected abstract string FormatMessage(string source, uint messageID, string[] replacementStrings);

	protected abstract string[] GetLogNames(string machineName);

	protected void ValidateCustomerLogName(string logName, string machineName)
	{
		if (logName.Length >= 8)
		{
			string text = logName.Substring(0, 8);
			if (string.Compare(text, "AppEvent", ignoreCase: true) == 0 || string.Compare(text, "SysEvent", ignoreCase: true) == 0 || string.Compare(text, "SecEvent", ignoreCase: true) == 0)
			{
				throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "The log name: '{0}' is invalid for customer log creation.", logName));
			}
			string[] logNames = GetLogNames(machineName);
			foreach (string text2 in logNames)
			{
				if (text2.Length >= 8 && string.Compare(text2, 0, text, 0, 8, ignoreCase: true) == 0)
				{
					throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "Only the first eight characters of a custom log name are significant, and there is already another log on the system using the first eight characters of the name given. Name given: '{0}', name of existing log: '{1}'.", logName, text2));
				}
			}
		}
		if (SourceExists(logName, machineName))
		{
			if (machineName == ".")
			{
				throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "Log {0} has already been registered as a source on the local computer.", logName));
			}
			throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "Log {0} has already been registered as a source on the computer {1}.", logName, machineName));
		}
	}

	public abstract void ModifyOverflowPolicy(OverflowAction action, int retentionDays);

	public abstract void RegisterDisplayName(string resourceFile, long resourceId);
}
