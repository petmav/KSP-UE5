using System.Collections;
using System.Collections.Generic;
using System.Threading;

namespace System.IO;

internal class DefaultWatcher : IFileWatcher
{
	private static DefaultWatcher instance;

	private static Thread thread;

	private static Hashtable watches;

	private static string[] NoStringsArray = new string[0];

	private DefaultWatcher()
	{
	}

	public static bool GetInstance(out IFileWatcher watcher)
	{
		if (instance != null)
		{
			watcher = instance;
			return true;
		}
		instance = new DefaultWatcher();
		watcher = instance;
		return true;
	}

	public void StartDispatching(FileSystemWatcher fsw)
	{
		lock (this)
		{
			if (watches == null)
			{
				watches = new Hashtable();
			}
			if (thread == null)
			{
				thread = new Thread(Monitor);
				thread.IsBackground = true;
				thread.Start();
			}
		}
		lock (watches)
		{
			DefaultWatcherData defaultWatcherData = (DefaultWatcherData)watches[fsw];
			if (defaultWatcherData == null)
			{
				defaultWatcherData = new DefaultWatcherData();
				defaultWatcherData.Files = new Hashtable();
				watches[fsw] = defaultWatcherData;
			}
			defaultWatcherData.FSW = fsw;
			defaultWatcherData.Directory = fsw.FullPath;
			defaultWatcherData.NoWildcards = !fsw.Pattern.HasWildcard;
			if (defaultWatcherData.NoWildcards)
			{
				defaultWatcherData.FileMask = Path.Combine(defaultWatcherData.Directory, fsw.MangledFilter);
			}
			else
			{
				defaultWatcherData.FileMask = fsw.MangledFilter;
			}
			defaultWatcherData.IncludeSubdirs = fsw.IncludeSubdirectories;
			defaultWatcherData.Enabled = true;
			defaultWatcherData.DisabledTime = DateTime.MaxValue;
			UpdateDataAndDispatch(defaultWatcherData, dispatch: false);
		}
	}

	public void StopDispatching(FileSystemWatcher fsw)
	{
		lock (this)
		{
			if (watches == null)
			{
				return;
			}
		}
		lock (watches)
		{
			DefaultWatcherData defaultWatcherData = (DefaultWatcherData)watches[fsw];
			if (defaultWatcherData != null)
			{
				defaultWatcherData.Enabled = false;
				defaultWatcherData.DisabledTime = DateTime.UtcNow;
			}
		}
	}

	private void Monitor()
	{
		int num = 0;
		while (true)
		{
			Thread.Sleep(750);
			Hashtable hashtable;
			lock (watches)
			{
				if (watches.Count == 0)
				{
					if (++num == 20)
					{
						break;
					}
					continue;
				}
				hashtable = (Hashtable)watches.Clone();
			}
			if (hashtable.Count == 0)
			{
				continue;
			}
			num = 0;
			foreach (DefaultWatcherData value in hashtable.Values)
			{
				if (UpdateDataAndDispatch(value, dispatch: true))
				{
					lock (watches)
					{
						watches.Remove(value.FSW);
					}
				}
			}
		}
		lock (this)
		{
			thread = null;
		}
	}

	private bool UpdateDataAndDispatch(DefaultWatcherData data, bool dispatch)
	{
		if (!data.Enabled)
		{
			if (data.DisabledTime != DateTime.MaxValue)
			{
				return (DateTime.UtcNow - data.DisabledTime).TotalSeconds > 5.0;
			}
			return false;
		}
		DoFiles(data, data.Directory, dispatch);
		return false;
	}

	private static void DispatchEvents(FileSystemWatcher fsw, FileAction action, string filename)
	{
		RenamedEventArgs renamed = null;
		lock (fsw)
		{
			fsw.DispatchEvents(action, filename, ref renamed);
			if (fsw.Waiting)
			{
				fsw.Waiting = false;
				System.Threading.Monitor.PulseAll(fsw);
			}
		}
	}

	private void DoFiles(DefaultWatcherData data, string directory, bool dispatch)
	{
		bool flag = Directory.Exists(directory);
		if (flag && data.IncludeSubdirs)
		{
			string[] directories = Directory.GetDirectories(directory);
			foreach (string directory2 in directories)
			{
				DoFiles(data, directory2, dispatch);
			}
		}
		string[] array = null;
		array = ((!flag) ? NoStringsArray : ((!data.NoWildcards) ? Directory.GetFileSystemEntries(directory, data.FileMask) : ((!File.Exists(data.FileMask) && !Directory.Exists(data.FileMask)) ? NoStringsArray : new string[1] { data.FileMask })));
		lock (data.FilesLock)
		{
			IterateAndModifyFilesData(data, directory, dispatch, array);
		}
	}

	private void IterateAndModifyFilesData(DefaultWatcherData data, string directory, bool dispatch, string[] files)
	{
		foreach (string key in data.Files.Keys)
		{
			FileData fileData = (FileData)data.Files[key];
			if (fileData.Directory == directory)
			{
				fileData.NotExists = true;
			}
		}
		foreach (string text in files)
		{
			FileData fileData2 = (FileData)data.Files[text];
			if (fileData2 == null)
			{
				try
				{
					data.Files.Add(text, CreateFileData(directory, text));
				}
				catch
				{
					data.Files.Remove(text);
					continue;
				}
				if (dispatch)
				{
					DispatchEvents(data.FSW, FileAction.Added, text);
				}
			}
			else if (fileData2.Directory == directory)
			{
				fileData2.NotExists = false;
			}
		}
		if (!dispatch)
		{
			return;
		}
		List<string> list = null;
		foreach (string key2 in data.Files.Keys)
		{
			if (((FileData)data.Files[key2]).NotExists)
			{
				if (list == null)
				{
					list = new List<string>();
				}
				list.Add(key2);
				DispatchEvents(data.FSW, FileAction.Removed, key2);
			}
		}
		if (list != null)
		{
			foreach (string item in list)
			{
				data.Files.Remove(item);
			}
			list = null;
		}
		foreach (string key3 in data.Files.Keys)
		{
			FileData fileData3 = (FileData)data.Files[key3];
			DateTime creationTime;
			DateTime lastWriteTime;
			try
			{
				creationTime = File.GetCreationTime(key3);
				lastWriteTime = File.GetLastWriteTime(key3);
			}
			catch
			{
				if (list == null)
				{
					list = new List<string>();
				}
				list.Add(key3);
				DispatchEvents(data.FSW, FileAction.Removed, key3);
				continue;
			}
			if (creationTime != fileData3.CreationTime || lastWriteTime != fileData3.LastWriteTime)
			{
				fileData3.CreationTime = creationTime;
				fileData3.LastWriteTime = lastWriteTime;
				DispatchEvents(data.FSW, FileAction.Modified, key3);
			}
		}
		if (list == null)
		{
			return;
		}
		foreach (string item2 in list)
		{
			data.Files.Remove(item2);
		}
	}

	private static FileData CreateFileData(string directory, string filename)
	{
		FileData fileData = new FileData();
		string path = Path.Combine(directory, filename);
		fileData.Directory = directory;
		fileData.Attributes = File.GetAttributes(path);
		fileData.CreationTime = File.GetCreationTime(path);
		fileData.LastWriteTime = File.GetLastWriteTime(path);
		return fileData;
	}
}
