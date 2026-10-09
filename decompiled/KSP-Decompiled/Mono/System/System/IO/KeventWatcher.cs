using System.Collections;
using System.Runtime.InteropServices;

namespace System.IO;

internal class KeventWatcher : IFileWatcher
{
	private static bool failed;

	private static KeventWatcher instance;

	private static Hashtable watches;

	private KeventWatcher()
	{
	}

	public static bool GetInstance(out IFileWatcher watcher)
	{
		if (failed)
		{
			watcher = null;
			return false;
		}
		if (instance != null)
		{
			watcher = instance;
			return true;
		}
		watches = Hashtable.Synchronized(new Hashtable());
		int num = kqueue();
		if (num == -1)
		{
			failed = true;
			watcher = null;
			return false;
		}
		close(num);
		instance = new KeventWatcher();
		watcher = instance;
		return true;
	}

	public void StartDispatching(FileSystemWatcher fsw)
	{
		KqueueMonitor kqueueMonitor;
		if (watches.ContainsKey(fsw))
		{
			kqueueMonitor = (KqueueMonitor)watches[fsw];
		}
		else
		{
			kqueueMonitor = new KqueueMonitor(fsw);
			watches.Add(fsw, kqueueMonitor);
		}
		kqueueMonitor.Start();
	}

	public void StopDispatching(FileSystemWatcher fsw)
	{
		((KqueueMonitor)watches[fsw])?.Stop();
	}

	[DllImport("libc")]
	private static extern int close(int fd);

	[DllImport("libc")]
	private static extern int kqueue();
}
