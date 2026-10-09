using System.Collections.Generic;
using System.ComponentModel;
using System.Globalization;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using Microsoft.Win32;

namespace System.Diagnostics;

internal class Win32EventLog : EventLogImpl
{
	private class PInvoke
	{
		public const int ERROR_INSUFFICIENT_BUFFER = 122;

		public const int ERROR_EVENTLOG_FILE_CHANGED = 1503;

		[DllImport("advapi32.dll", SetLastError = true)]
		public static extern int ClearEventLog(IntPtr hEventLog, string lpBackupFileName);

		[DllImport("advapi32.dll", SetLastError = true)]
		public static extern int CloseEventLog(IntPtr hEventLog);

		[DllImport("advapi32.dll", SetLastError = true)]
		public static extern int DeregisterEventSource(IntPtr hEventLog);

		[DllImport("kernel32", CharSet = CharSet.Auto, SetLastError = true)]
		public static extern int FormatMessage(FormatMessageFlags dwFlags, IntPtr lpSource, uint dwMessageId, int dwLanguageId, ref IntPtr lpBuffer, int nSize, IntPtr[] arguments);

		[DllImport("kernel32", SetLastError = true)]
		public static extern bool FreeLibrary(IntPtr hModule);

		[DllImport("advapi32.dll", SetLastError = true)]
		public static extern int GetNumberOfEventLogRecords(IntPtr hEventLog, ref int NumberOfRecords);

		[DllImport("advapi32.dll", SetLastError = true)]
		public static extern int GetOldestEventLogRecord(IntPtr hEventLog, ref int OldestRecord);

		[DllImport("kernel32", SetLastError = true)]
		public static extern IntPtr LoadLibraryEx(string lpFileName, IntPtr hFile, LoadFlags dwFlags);

		[DllImport("kernel32", SetLastError = true)]
		public static extern IntPtr LocalFree(IntPtr hMem);

		[DllImport("advapi32.dll", SetLastError = true)]
		public static extern bool LookupAccountSid(string lpSystemName, [MarshalAs(UnmanagedType.LPArray)] byte[] Sid, StringBuilder lpName, ref uint cchName, StringBuilder ReferencedDomainName, ref uint cchReferencedDomainName, out SidNameUse peUse);

		[DllImport("advapi32.dll", SetLastError = true)]
		public static extern int NotifyChangeEventLog(IntPtr hEventLog, IntPtr hEvent);

		[DllImport("advapi32.dll", SetLastError = true)]
		public static extern IntPtr OpenEventLog(string machineName, string logName);

		[DllImport("advapi32.dll", SetLastError = true)]
		public static extern IntPtr RegisterEventSource(string machineName, string sourceName);

		[DllImport("advapi32.dll", SetLastError = true)]
		public static extern int ReportEvent(IntPtr hHandle, ushort wType, ushort wCategory, uint dwEventID, IntPtr sid, ushort wNumStrings, uint dwDataSize, string[] lpStrings, byte[] lpRawData);

		[DllImport("advapi32.dll", SetLastError = true)]
		public static extern int ReadEventLog(IntPtr hEventLog, ReadFlags dwReadFlags, int dwRecordOffset, byte[] buffer, int nNumberOfBytesToRead, ref int pnBytesRead, ref int pnMinNumberOfBytesNeeded);
	}

	private enum ReadFlags
	{
		Sequential = 1,
		Seek = 2,
		ForwardsRead = 4,
		BackwardsRead = 8
	}

	private enum LoadFlags : uint
	{
		LibraryAsDataFile = 2u
	}

	[Flags]
	private enum FormatMessageFlags
	{
		AllocateBuffer = 0x100,
		IgnoreInserts = 0x200,
		FromHModule = 0x800,
		FromSystem = 0x1000,
		ArgumentArray = 0x2000
	}

	private enum SidNameUse
	{
		User = 1,
		Group,
		Domain,
		lias,
		WellKnownGroup,
		DeletedAccount,
		Invalid,
		Unknown,
		Computer
	}

	private const int MESSAGE_NOT_FOUND = 317;

	private ManualResetEvent _notifyResetEvent;

	private IntPtr _readHandle;

	private Thread _notifyThread;

	private int _lastEntryWritten;

	private object _eventLock = new object();

	private int OldestEventLogEntry
	{
		get
		{
			int OldestRecord = 0;
			if (PInvoke.GetOldestEventLogRecord(ReadHandle, ref OldestRecord) != 1)
			{
				throw new Win32Exception(Marshal.GetLastWin32Error());
			}
			return OldestRecord;
		}
	}

	private IntPtr ReadHandle
	{
		get
		{
			if (_readHandle != IntPtr.Zero)
			{
				return _readHandle;
			}
			string logName = base.CoreEventLog.GetLogName();
			_readHandle = PInvoke.OpenEventLog(base.CoreEventLog.MachineName, logName);
			if (_readHandle == IntPtr.Zero)
			{
				throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "Event Log '{0}' on computer '{1}' cannot be opened.", logName, base.CoreEventLog.MachineName), new Win32Exception());
			}
			return _readHandle;
		}
	}

	public override OverflowAction OverflowAction
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	public override int MinimumRetentionDays
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	public override long MaximumKilobytes
	{
		get
		{
			throw new NotImplementedException();
		}
		set
		{
			throw new NotImplementedException();
		}
	}

	public Win32EventLog(EventLog coreEventLog)
		: base(coreEventLog)
	{
	}

	public override void BeginInit()
	{
	}

	public override void Clear()
	{
		if (PInvoke.ClearEventLog(ReadHandle, null) != 1)
		{
			throw new Win32Exception(Marshal.GetLastWin32Error());
		}
	}

	public override void Close()
	{
		lock (_eventLock)
		{
			if (_readHandle != IntPtr.Zero)
			{
				CloseEventLog(_readHandle);
				_readHandle = IntPtr.Zero;
			}
		}
	}

	public override void CreateEventSource(EventSourceCreationData sourceData)
	{
		using RegistryKey registryKey = GetEventLogKey(sourceData.MachineName, writable: true);
		if (registryKey == null)
		{
			throw new InvalidOperationException("EventLog registry key is missing.");
		}
		bool flag = false;
		RegistryKey registryKey2 = null;
		try
		{
			registryKey2 = registryKey.OpenSubKey(sourceData.LogName, writable: true);
			if (registryKey2 == null)
			{
				ValidateCustomerLogName(sourceData.LogName, sourceData.MachineName);
				registryKey2 = registryKey.CreateSubKey(sourceData.LogName);
				registryKey2.SetValue("Sources", new string[2] { sourceData.LogName, sourceData.Source });
				UpdateLogRegistry(registryKey2);
				using (RegistryKey sourceKey = registryKey2.CreateSubKey(sourceData.LogName))
				{
					UpdateSourceRegistry(sourceKey, sourceData);
				}
				flag = true;
			}
			if (!(sourceData.LogName != sourceData.Source))
			{
				return;
			}
			if (!flag)
			{
				string[] array = (string[])registryKey2.GetValue("Sources");
				if (array == null)
				{
					registryKey2.SetValue("Sources", new string[2] { sourceData.LogName, sourceData.Source });
				}
				else
				{
					bool flag2 = false;
					for (int i = 0; i < array.Length; i++)
					{
						if (array[i] == sourceData.Source)
						{
							flag2 = true;
							break;
						}
					}
					if (!flag2)
					{
						string[] array2 = new string[array.Length + 1];
						Array.Copy(array, 0, array2, 0, array.Length);
						array2[array.Length] = sourceData.Source;
						registryKey2.SetValue("Sources", array2);
					}
				}
			}
			using RegistryKey sourceKey2 = registryKey2.CreateSubKey(sourceData.Source);
			UpdateSourceRegistry(sourceKey2, sourceData);
		}
		finally
		{
			registryKey2?.Close();
		}
	}

	public override void Delete(string logName, string machineName)
	{
		using RegistryKey registryKey = GetEventLogKey(machineName, writable: true);
		if (registryKey == null)
		{
			throw new InvalidOperationException("The event log key does not exist.");
		}
		using (RegistryKey registryKey2 = registryKey.OpenSubKey(logName, writable: false))
		{
			if (registryKey2 == null)
			{
				throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "Event Log '{0}' does not exist on computer '{1}'.", logName, machineName));
			}
			base.CoreEventLog.Clear();
			string text = (string)registryKey2.GetValue("File");
			if (text != null)
			{
				try
				{
					File.Delete(text);
				}
				catch (Exception)
				{
				}
			}
		}
		registryKey.DeleteSubKeyTree(logName);
	}

	public override void DeleteEventSource(string source, string machineName)
	{
		using RegistryKey registryKey = FindLogKeyBySource(source, machineName, writable: true);
		if (registryKey == null)
		{
			throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "The source '{0}' is not registered on computer '{1}'.", source, machineName));
		}
		registryKey.DeleteSubKeyTree(source);
		string[] array = (string[])registryKey.GetValue("Sources");
		if (array == null)
		{
			return;
		}
		List<string> list = new List<string>();
		for (int i = 0; i < array.Length; i++)
		{
			if (array[i] != source)
			{
				list.Add(array[i]);
			}
		}
		string[] value = list.ToArray();
		registryKey.SetValue("Sources", value);
	}

	public override void Dispose(bool disposing)
	{
		Close();
	}

	public override void EndInit()
	{
	}

	public override bool Exists(string logName, string machineName)
	{
		using RegistryKey registryKey = FindLogKeyByName(logName, machineName, writable: false);
		return registryKey != null;
	}

	[System.MonoTODO]
	protected override string FormatMessage(string source, uint messageID, string[] replacementStrings)
	{
		string text = null;
		string[] messageResourceDlls = GetMessageResourceDlls(source, "EventMessageFile");
		for (int i = 0; i < messageResourceDlls.Length; i++)
		{
			text = FetchMessage(messageResourceDlls[i], messageID, replacementStrings);
			if (text != null)
			{
				break;
			}
		}
		if (text == null)
		{
			return string.Join(", ", replacementStrings);
		}
		return text;
	}

	private string FormatCategory(string source, int category)
	{
		string text = null;
		string[] messageResourceDlls = GetMessageResourceDlls(source, "CategoryMessageFile");
		for (int i = 0; i < messageResourceDlls.Length; i++)
		{
			text = FetchMessage(messageResourceDlls[i], (uint)category, new string[0]);
			if (text != null)
			{
				break;
			}
		}
		if (text == null)
		{
			return "(" + category.ToString(CultureInfo.InvariantCulture) + ")";
		}
		return text;
	}

	protected override int GetEntryCount()
	{
		int NumberOfRecords = 0;
		if (PInvoke.GetNumberOfEventLogRecords(ReadHandle, ref NumberOfRecords) != 1)
		{
			throw new Win32Exception(Marshal.GetLastWin32Error());
		}
		return NumberOfRecords;
	}

	protected override EventLogEntry GetEntry(int index)
	{
		index += OldestEventLogEntry;
		int bytesRead = 0;
		int minBufferNeeded = 0;
		byte[] buffer = new byte[524287];
		ReadEventLog(index, buffer, ref bytesRead, ref minBufferNeeded);
		MemoryStream memoryStream = new MemoryStream(buffer);
		BinaryReader binaryReader = new BinaryReader(memoryStream);
		binaryReader.ReadBytes(8);
		int index2 = binaryReader.ReadInt32();
		int num = binaryReader.ReadInt32();
		int num2 = binaryReader.ReadInt32();
		uint num3 = binaryReader.ReadUInt32();
		int eventID = EventLog.GetEventID(num3);
		short entryType = binaryReader.ReadInt16();
		short num4 = binaryReader.ReadInt16();
		short num5 = binaryReader.ReadInt16();
		binaryReader.ReadInt16();
		binaryReader.ReadInt32();
		int num6 = binaryReader.ReadInt32();
		int num7 = binaryReader.ReadInt32();
		int num8 = binaryReader.ReadInt32();
		int num9 = binaryReader.ReadInt32();
		int num10 = binaryReader.ReadInt32();
		DateTime timeGenerated = new DateTime(1970, 1, 1).AddSeconds(num);
		DateTime timeWritten = new DateTime(1970, 1, 1).AddSeconds(num2);
		StringBuilder stringBuilder = new StringBuilder();
		while (binaryReader.PeekChar() != 0)
		{
			stringBuilder.Append(binaryReader.ReadChar());
		}
		binaryReader.ReadChar();
		string source = stringBuilder.ToString();
		stringBuilder.Length = 0;
		while (binaryReader.PeekChar() != 0)
		{
			stringBuilder.Append(binaryReader.ReadChar());
		}
		binaryReader.ReadChar();
		string machineName = stringBuilder.ToString();
		stringBuilder.Length = 0;
		while (binaryReader.PeekChar() != 0)
		{
			stringBuilder.Append(binaryReader.ReadChar());
		}
		binaryReader.ReadChar();
		string userName = null;
		if (num7 != 0)
		{
			memoryStream.Position = num8;
			byte[] sid = binaryReader.ReadBytes(num7);
			userName = LookupAccountSid(machineName, sid);
		}
		memoryStream.Position = num6;
		string[] array = new string[num4];
		for (int i = 0; i < num4; i++)
		{
			stringBuilder.Length = 0;
			while (binaryReader.PeekChar() != 0)
			{
				stringBuilder.Append(binaryReader.ReadChar());
			}
			binaryReader.ReadChar();
			array[i] = stringBuilder.ToString();
		}
		byte[] array2 = new byte[num9];
		memoryStream.Position = num10;
		binaryReader.Read(array2, 0, num9);
		string message = FormatMessage(source, num3, array);
		return new EventLogEntry(FormatCategory(source, num5), num5, index2, eventID, source, message, userName, machineName, (EventLogEntryType)entryType, timeGenerated, timeWritten, array2, array, num3);
	}

	[System.MonoTODO]
	protected override string GetLogDisplayName()
	{
		return base.CoreEventLog.Log;
	}

	protected override string[] GetLogNames(string machineName)
	{
		using RegistryKey registryKey = GetEventLogKey(machineName, writable: true);
		if (registryKey == null)
		{
			return new string[0];
		}
		return registryKey.GetSubKeyNames();
	}

	public override string LogNameFromSourceName(string source, string machineName)
	{
		using RegistryKey registryKey = FindLogKeyBySource(source, machineName, writable: false);
		if (registryKey == null)
		{
			return string.Empty;
		}
		return GetLogName(registryKey);
	}

	public override bool SourceExists(string source, string machineName)
	{
		RegistryKey registryKey = FindLogKeyBySource(source, machineName, writable: false);
		if (registryKey != null)
		{
			registryKey.Close();
			return true;
		}
		return false;
	}

	public override void WriteEntry(string[] replacementStrings, EventLogEntryType type, uint instanceID, short category, byte[] rawData)
	{
		IntPtr intPtr = RegisterEventSource();
		try
		{
			if (PInvoke.ReportEvent(intPtr, (ushort)type, (ushort)category, instanceID, IntPtr.Zero, (ushort)replacementStrings.Length, (uint)rawData.Length, replacementStrings, rawData) != 1)
			{
				throw new Win32Exception(Marshal.GetLastWin32Error());
			}
		}
		finally
		{
			DeregisterEventSource(intPtr);
		}
	}

	private static void UpdateLogRegistry(RegistryKey logKey)
	{
		if (logKey.GetValue("File") == null)
		{
			string logName = GetLogName(logKey);
			string path = ((logName.Length <= 8) ? (logName + ".evt") : (logName.Substring(0, 8) + ".evt"));
			string path2 = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "config");
			logKey.SetValue("File", Path.Combine(path2, path));
		}
	}

	private static void UpdateSourceRegistry(RegistryKey sourceKey, EventSourceCreationData data)
	{
		if (data.CategoryCount > 0)
		{
			sourceKey.SetValue("CategoryCount", data.CategoryCount);
		}
		if (data.CategoryResourceFile != null && data.CategoryResourceFile.Length > 0)
		{
			sourceKey.SetValue("CategoryMessageFile", data.CategoryResourceFile);
		}
		if (data.MessageResourceFile != null && data.MessageResourceFile.Length > 0)
		{
			sourceKey.SetValue("EventMessageFile", data.MessageResourceFile);
		}
		if (data.ParameterResourceFile != null && data.ParameterResourceFile.Length > 0)
		{
			sourceKey.SetValue("ParameterMessageFile", data.ParameterResourceFile);
		}
	}

	private static string GetLogName(RegistryKey logKey)
	{
		string name = logKey.Name;
		return name.Substring(name.LastIndexOf("\\") + 1);
	}

	private void ReadEventLog(int index, byte[] buffer, ref int bytesRead, ref int minBufferNeeded)
	{
		for (int i = 0; i < 3; i++)
		{
			if (PInvoke.ReadEventLog(ReadHandle, (ReadFlags)6, index, buffer, buffer.Length, ref bytesRead, ref minBufferNeeded) != 1)
			{
				int lastWin32Error = Marshal.GetLastWin32Error();
				if (i >= 2)
				{
					throw new Win32Exception(lastWin32Error);
				}
				base.CoreEventLog.Reset();
			}
		}
	}

	[System.MonoTODO("Support remote machines")]
	private static RegistryKey GetEventLogKey(string machineName, bool writable)
	{
		return Registry.LocalMachine.OpenSubKey("SYSTEM\\CurrentControlSet\\Services\\EventLog", writable);
	}

	private static RegistryKey FindSourceKeyByName(string source, string machineName, bool writable)
	{
		if (source == null || source.Length == 0)
		{
			return null;
		}
		RegistryKey registryKey = null;
		try
		{
			registryKey = GetEventLogKey(machineName, writable);
			if (registryKey == null)
			{
				return null;
			}
			string[] subKeyNames = registryKey.GetSubKeyNames();
			for (int i = 0; i < subKeyNames.Length; i++)
			{
				using RegistryKey registryKey2 = registryKey.OpenSubKey(subKeyNames[i], writable);
				if (registryKey2 == null)
				{
					break;
				}
				RegistryKey registryKey3 = registryKey2.OpenSubKey(source, writable);
				if (registryKey3 != null)
				{
					return registryKey3;
				}
				continue;
			}
			return null;
		}
		finally
		{
			registryKey?.Close();
		}
	}

	private static RegistryKey FindLogKeyByName(string logName, string machineName, bool writable)
	{
		using RegistryKey registryKey = GetEventLogKey(machineName, writable);
		return registryKey?.OpenSubKey(logName, writable);
	}

	private static RegistryKey FindLogKeyBySource(string source, string machineName, bool writable)
	{
		if (source == null || source.Length == 0)
		{
			return null;
		}
		RegistryKey registryKey = null;
		try
		{
			registryKey = GetEventLogKey(machineName, writable);
			if (registryKey == null)
			{
				return null;
			}
			string[] subKeyNames = registryKey.GetSubKeyNames();
			for (int i = 0; i < subKeyNames.Length; i++)
			{
				RegistryKey registryKey2 = null;
				try
				{
					RegistryKey registryKey3 = registryKey.OpenSubKey(subKeyNames[i], writable);
					if (registryKey3 != null)
					{
						registryKey2 = registryKey3.OpenSubKey(source, writable);
						if (registryKey2 != null)
						{
							return registryKey3;
						}
					}
				}
				finally
				{
					registryKey2?.Close();
				}
			}
			return null;
		}
		finally
		{
			registryKey?.Close();
		}
	}

	private void CloseEventLog(IntPtr hEventLog)
	{
		if (PInvoke.CloseEventLog(hEventLog) != 1)
		{
			throw new Win32Exception(Marshal.GetLastWin32Error());
		}
	}

	private void DeregisterEventSource(IntPtr hEventLog)
	{
		if (PInvoke.DeregisterEventSource(hEventLog) != 1)
		{
			throw new Win32Exception(Marshal.GetLastWin32Error());
		}
	}

	private static string LookupAccountSid(string machineName, byte[] sid)
	{
		StringBuilder stringBuilder = new StringBuilder();
		uint cchName = (uint)stringBuilder.Capacity;
		StringBuilder stringBuilder2 = new StringBuilder();
		uint cchReferencedDomainName = (uint)stringBuilder2.Capacity;
		string text = null;
		while (text == null)
		{
			if (!PInvoke.LookupAccountSid(machineName, sid, stringBuilder, ref cchName, stringBuilder2, ref cchReferencedDomainName, out var _))
			{
				if (Marshal.GetLastWin32Error() == 122)
				{
					stringBuilder.EnsureCapacity((int)cchName);
					stringBuilder2.EnsureCapacity((int)cchReferencedDomainName);
				}
				else
				{
					text = string.Empty;
				}
			}
			else
			{
				text = $"{stringBuilder2.ToString()}\\{stringBuilder.ToString()}";
			}
		}
		return text;
	}

	private static string FetchMessage(string msgDll, uint messageID, string[] replacementStrings)
	{
		IntPtr intPtr = PInvoke.LoadLibraryEx(msgDll, IntPtr.Zero, LoadFlags.LibraryAsDataFile);
		if (intPtr == IntPtr.Zero)
		{
			return null;
		}
		IntPtr lpBuffer = IntPtr.Zero;
		IntPtr[] array = new IntPtr[replacementStrings.Length];
		try
		{
			for (int i = 0; i < replacementStrings.Length; i++)
			{
				array[i] = Marshal.StringToHGlobalAuto(replacementStrings[i]);
			}
			if (PInvoke.FormatMessage(FormatMessageFlags.AllocateBuffer | FormatMessageFlags.FromHModule | FormatMessageFlags.ArgumentArray, intPtr, messageID, 0, ref lpBuffer, 0, array) != 0)
			{
				string text = Marshal.PtrToStringAuto(lpBuffer);
				lpBuffer = PInvoke.LocalFree(lpBuffer);
				return text.TrimEnd(null);
			}
			Marshal.GetLastWin32Error();
			_ = 317;
		}
		finally
		{
			foreach (IntPtr intPtr2 in array)
			{
				if (intPtr2 != IntPtr.Zero)
				{
					Marshal.FreeHGlobal(intPtr2);
				}
			}
			PInvoke.FreeLibrary(intPtr);
		}
		return null;
	}

	private string[] GetMessageResourceDlls(string source, string valueName)
	{
		RegistryKey registryKey = FindSourceKeyByName(source, base.CoreEventLog.MachineName, writable: false);
		if (registryKey != null && registryKey.GetValue(valueName) is string text)
		{
			return text.Split(';');
		}
		return new string[0];
	}

	private IntPtr RegisterEventSource()
	{
		IntPtr intPtr = PInvoke.RegisterEventSource(base.CoreEventLog.MachineName, base.CoreEventLog.Source);
		if (intPtr == IntPtr.Zero)
		{
			throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "Event source '{0}' on computer '{1}' cannot be opened.", base.CoreEventLog.Source, base.CoreEventLog.MachineName), new Win32Exception());
		}
		return intPtr;
	}

	public override void DisableNotification()
	{
		lock (_eventLock)
		{
			if (_notifyResetEvent != null)
			{
				_notifyResetEvent.Close();
				_notifyResetEvent = null;
			}
			_notifyThread = null;
		}
	}

	public override void EnableNotification()
	{
		lock (_eventLock)
		{
			if (_notifyResetEvent == null)
			{
				_notifyResetEvent = new ManualResetEvent(initialState: false);
				_lastEntryWritten = OldestEventLogEntry + base.EntryCount;
				if (PInvoke.NotifyChangeEventLog(ReadHandle, _notifyResetEvent.SafeWaitHandle.DangerousGetHandle()) == 0)
				{
					throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "Unable to receive notifications for log '{0}' on computer '{1}'.", base.CoreEventLog.GetLogName(), base.CoreEventLog.MachineName), new Win32Exception());
				}
				_notifyThread = new Thread((ThreadStart)delegate
				{
					NotifyEventThread(_notifyResetEvent);
				});
				_notifyThread.IsBackground = true;
				_notifyThread.Start();
			}
		}
	}

	private void NotifyEventThread(ManualResetEvent resetEvent)
	{
		if (resetEvent == null)
		{
			return;
		}
		while (true)
		{
			try
			{
				resetEvent.WaitOne();
			}
			catch (ObjectDisposedException)
			{
				break;
			}
			lock (_eventLock)
			{
				if (resetEvent != _notifyResetEvent || _readHandle == IntPtr.Zero)
				{
					break;
				}
				int oldestEventLogEntry = OldestEventLogEntry;
				if (_lastEntryWritten < oldestEventLogEntry)
				{
					_lastEntryWritten = oldestEventLogEntry;
				}
				int num = _lastEntryWritten - oldestEventLogEntry;
				int num2 = base.EntryCount + oldestEventLogEntry;
				for (int i = num; i < num2 - 1; i++)
				{
					EventLogEntry entry = GetEntry(i);
					base.CoreEventLog.OnEntryWritten(entry);
				}
				_lastEntryWritten = num2;
			}
		}
	}

	public override void ModifyOverflowPolicy(OverflowAction action, int retentionDays)
	{
		throw new NotImplementedException();
	}

	public override void RegisterDisplayName(string resourceFile, long resourceId)
	{
		throw new NotImplementedException();
	}
}
