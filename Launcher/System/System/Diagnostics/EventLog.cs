using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Security.Permissions;
using Unity;

namespace System.Diagnostics;

[MonitoringDescription("EventLogDesc")]
[InstallerType("System.Diagnostics.EventLogInstaller, System.Configuration.Install, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a")]
[DefaultEvent("EntryWritten")]
public class EventLog : Component, ISupportInitialize
{
	public bool EnableRaisingEvents
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public EventLogEntryCollection Entries
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string Log
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public string LogDisplayName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string MachineName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public long MaximumKilobytes
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(long);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int MinimumRetentionDays
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public OverflowAction OverflowAction
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(OverflowAction);
		}
	}

	public string Source
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public ISynchronizeInvoke SynchronizingObject
	{
		[HostProtection(SecurityAction.LinkDemand, Synchronization = true)]
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public event EntryWrittenEventHandler EntryWritten
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public EventLog()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLog(string logName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLog(string logName, string machineName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLog(string logName, string machineName, string source)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void BeginInit()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Clear()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Close()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void CreateEventSource(EventSourceCreationData sourceData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void CreateEventSource(string source, string logName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[Obsolete("This method has been deprecated.  Please use System.Diagnostics.EventLog.CreateEventSource(EventSourceCreationData sourceData) instead.  http://go.microsoft.com/fwlink/?linkid=14202")]
	public static void CreateEventSource(string source, string logName, string machineName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void Delete(string logName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void Delete(string logName, string machineName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void DeleteEventSource(string source)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void DeleteEventSource(string source, string machineName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EndInit()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static bool Exists(string logName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool Exists(string logName, string machineName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static EventLog[] GetEventLogs()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static EventLog[] GetEventLogs(string machineName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static string LogNameFromSourceName(string source, string machineName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[ComVisible(false)]
	public void ModifyOverflowPolicy(OverflowAction action, int retentionDays)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[ComVisible(false)]
	public void RegisterDisplayName(string resourceFile, long resourceId)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static bool SourceExists(string source)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool SourceExists(string source, string machineName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void WriteEntry(string message)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void WriteEntry(string message, EventLogEntryType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void WriteEntry(string message, EventLogEntryType type, int eventID)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void WriteEntry(string message, EventLogEntryType type, int eventID, short category)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void WriteEntry(string message, EventLogEntryType type, int eventID, short category, byte[] rawData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void WriteEntry(string source, string message)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void WriteEntry(string source, string message, EventLogEntryType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void WriteEntry(string source, string message, EventLogEntryType type, int eventID)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void WriteEntry(string source, string message, EventLogEntryType type, int eventID, short category)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void WriteEntry(string source, string message, EventLogEntryType type, int eventID, short category, byte[] rawData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[ComVisible(false)]
	public void WriteEvent(EventInstance instance, byte[] data, object[] values)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[ComVisible(false)]
	public void WriteEvent(EventInstance instance, object[] values)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void WriteEvent(string source, EventInstance instance, byte[] data, object[] values)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void WriteEvent(string source, EventInstance instance, object[] values)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
