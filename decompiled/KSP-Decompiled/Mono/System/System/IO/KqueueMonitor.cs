using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace System.IO;

internal class KqueueMonitor : IDisposable
{
	private static bool initialized;

	private const int O_EVTONLY = 32768;

	private const int F_GETPATH = 50;

	private const int __DARWIN_MAXPATHLEN = 1024;

	private const int EINTR = 4;

	private static readonly kevent[] emptyEventList = new kevent[0];

	private int maxFds = int.MaxValue;

	private FileSystemWatcher fsw;

	private int conn;

	private Thread thread;

	private volatile bool requestStop;

	private AutoResetEvent startedEvent = new AutoResetEvent(initialState: false);

	private bool started;

	private bool inDispatch;

	private Exception exc;

	private object stateLock = new object();

	private object connLock = new object();

	private readonly Dictionary<string, PathData> pathsDict = new Dictionary<string, PathData>();

	private readonly Dictionary<int, PathData> fdsDict = new Dictionary<int, PathData>();

	private string fixupPath;

	private string fullPathNoLastSlash;

	public int Connection => conn;

	public KqueueMonitor(FileSystemWatcher fsw)
	{
		this.fsw = fsw;
		conn = -1;
		if (!initialized)
		{
			initialized = true;
			string environmentVariable = Environment.GetEnvironmentVariable("MONO_DARWIN_WATCHER_MAXFDS");
			if (environmentVariable != null && int.TryParse(environmentVariable, out var result))
			{
				maxFds = result;
			}
		}
	}

	public void Dispose()
	{
		CleanUp();
	}

	public void Start()
	{
		lock (stateLock)
		{
			if (!started)
			{
				conn = kqueue();
				if (conn == -1)
				{
					throw new IOException($"kqueue() error at init, error code = '{Marshal.GetLastWin32Error()}'");
				}
				thread = new Thread((ThreadStart)delegate
				{
					DoMonitor();
				});
				thread.IsBackground = true;
				thread.Start();
				startedEvent.WaitOne();
				if (exc != null)
				{
					thread.Join();
					CleanUp();
					throw exc;
				}
				started = true;
			}
		}
	}

	public void Stop()
	{
		lock (stateLock)
		{
			if (!started)
			{
				return;
			}
			requestStop = true;
			if (inDispatch)
			{
				return;
			}
			lock (connLock)
			{
				if (conn != -1)
				{
					close(conn);
				}
				conn = -1;
			}
			if (!thread.Join(2000))
			{
				thread.Abort();
			}
			requestStop = false;
			started = false;
			if (exc == null)
			{
				return;
			}
			throw exc;
		}
	}

	private void CleanUp()
	{
		lock (connLock)
		{
			if (conn != -1)
			{
				close(conn);
			}
			conn = -1;
		}
		foreach (int key in fdsDict.Keys)
		{
			close(key);
		}
		fdsDict.Clear();
		pathsDict.Clear();
	}

	private void DoMonitor()
	{
		try
		{
			Setup();
		}
		catch (Exception ex)
		{
			exc = ex;
		}
		finally
		{
			startedEvent.Set();
		}
		if (exc != null)
		{
			fsw.DispatchErrorEvents(new ErrorEventArgs(exc));
			return;
		}
		try
		{
			Monitor();
		}
		catch (Exception ex2)
		{
			exc = ex2;
		}
		finally
		{
			CleanUp();
			if (!requestStop)
			{
				started = false;
				inDispatch = false;
				fsw.EnableRaisingEvents = false;
			}
			if (exc != null)
			{
				fsw.DispatchErrorEvents(new ErrorEventArgs(exc));
			}
			requestStop = false;
		}
	}

	private void Setup()
	{
		List<int> fds = new List<int>();
		if (fsw.FullPath != "/" && fsw.FullPath.EndsWith("/", StringComparison.Ordinal))
		{
			fullPathNoLastSlash = fsw.FullPath.Substring(0, fsw.FullPath.Length - 1);
		}
		else
		{
			fullPathNoLastSlash = fsw.FullPath;
		}
		StringBuilder stringBuilder = new StringBuilder(1024);
		if (realpath(fsw.FullPath, stringBuilder) == IntPtr.Zero)
		{
			throw new IOException($"realpath({fsw.FullPath}) failed, error code = '{Marshal.GetLastWin32Error()}'");
		}
		string text = stringBuilder.ToString();
		if (text != fullPathNoLastSlash)
		{
			fixupPath = text;
		}
		else
		{
			fixupPath = null;
		}
		Scan(fullPathNoLastSlash, postEvents: false, ref fds);
		timespec timespec2 = default(timespec);
		timespec2.tv_sec = (IntPtr)0;
		timespec2.tv_nsec = (IntPtr)0;
		timespec time = timespec2;
		kevent[] array = new kevent[0];
		kevent[] array2 = CreateChangeList(ref fds);
		int num = 0;
		int num2;
		do
		{
			num2 = kevent(conn, array2, array2.Length, array, array.Length, ref time);
			if (num2 == -1)
			{
				num = Marshal.GetLastWin32Error();
			}
		}
		while (num2 == -1 && num == 4);
		if (num2 == -1)
		{
			throw new IOException($"kevent() error at initial event registration, error code = '{num}'");
		}
	}

	private kevent[] CreateChangeList(ref List<int> FdList)
	{
		if (FdList.Count == 0)
		{
			return emptyEventList;
		}
		List<kevent> list = new List<kevent>();
		foreach (int Fd in FdList)
		{
			kevent kevent2 = default(kevent);
			kevent2.ident = (UIntPtr)(ulong)Fd;
			kevent2.filter = EventFilter.Vnode;
			kevent2.flags = EventFlags.Add | EventFlags.Enable | EventFlags.Clear;
			kevent2.fflags = FilterFlags.ReadLowWaterMark | FilterFlags.VNodeWrite | FilterFlags.VNodeExtend | FilterFlags.VNodeAttrib | FilterFlags.VNodeLink | FilterFlags.VNodeRename | FilterFlags.VNodeRevoke;
			kevent2.data = IntPtr.Zero;
			kevent2.udata = IntPtr.Zero;
			kevent item = kevent2;
			list.Add(item);
		}
		FdList.Clear();
		return list.ToArray();
	}

	private void Monitor()
	{
		kevent[] array = new kevent[32];
		List<int> newFds = new List<int>();
		List<PathData> list = new List<PathData>();
		List<string> list2 = new List<string>();
		int num = 0;
		while (!requestStop)
		{
			kevent[] array2 = CreateChangeList(ref newFds);
			int num2 = Marshal.SizeOf<kevent>();
			IntPtr intPtr = Marshal.AllocHGlobal(num2 * array2.Length);
			for (int i = 0; i < array2.Length; i++)
			{
				Marshal.StructureToPtr(array2[i], intPtr + i * num2, fDeleteOld: false);
			}
			IntPtr intPtr2 = Marshal.AllocHGlobal(num2 * array.Length);
			int num3 = kevent_notimeout(ref conn, intPtr, array2.Length, intPtr2, array.Length);
			Marshal.FreeHGlobal(intPtr);
			for (int j = 0; j < num3; j++)
			{
				array[j] = Marshal.PtrToStructure<kevent>(intPtr2 + j * num2);
			}
			Marshal.FreeHGlobal(intPtr2);
			if (num3 == -1)
			{
				if (!requestStop)
				{
					int lastWin32Error = Marshal.GetLastWin32Error();
					if (lastWin32Error != 4 && ++num == 3)
					{
						throw new IOException($"persistent kevent() error, error code = '{lastWin32Error}'");
					}
					continue;
				}
				break;
			}
			num = 0;
			for (int k = 0; k < num3; k++)
			{
				kevent kevent2 = array[k];
				if (!fdsDict.ContainsKey((int)(uint)kevent2.ident))
				{
					continue;
				}
				PathData pathData = fdsDict[(int)(uint)kevent2.ident];
				if ((kevent2.flags & EventFlags.Error) == EventFlags.Error)
				{
					string message = $"kevent() error watching path '{pathData.Path}', error code = '{kevent2.data}'";
					fsw.DispatchErrorEvents(new ErrorEventArgs(new IOException(message)));
					continue;
				}
				if ((kevent2.fflags & FilterFlags.ReadLowWaterMark) == FilterFlags.ReadLowWaterMark || (kevent2.fflags & FilterFlags.VNodeRevoke) == FilterFlags.VNodeRevoke)
				{
					if (pathData.Path == fullPathNoLastSlash)
					{
						return;
					}
					list.Add(pathData);
					continue;
				}
				if ((kevent2.fflags & FilterFlags.VNodeRename) == FilterFlags.VNodeRename)
				{
					UpdatePath(pathData);
				}
				if ((kevent2.fflags & FilterFlags.VNodeWrite) == FilterFlags.VNodeWrite)
				{
					if (pathData.IsDirectory)
					{
						list2.Add(pathData.Path);
					}
					else
					{
						PostEvent(FileAction.Modified, pathData.Path);
					}
				}
				if ((kevent2.fflags & FilterFlags.VNodeAttrib) == FilterFlags.VNodeAttrib || (kevent2.fflags & FilterFlags.VNodeExtend) == FilterFlags.VNodeExtend)
				{
					PostEvent(FileAction.Modified, pathData.Path);
				}
			}
			list.ForEach(Remove);
			list.Clear();
			list2.ForEach(delegate(string path)
			{
				Scan(path, postEvents: true, ref newFds);
			});
			list2.Clear();
		}
	}

	private PathData Add(string path, bool postEvents, ref List<int> fds)
	{
		pathsDict.TryGetValue(path, out var value);
		if (value != null)
		{
			return value;
		}
		if (fdsDict.Count >= maxFds)
		{
			throw new IOException("kqueue() FileSystemWatcher has reached the maximum number of files to watch.");
		}
		int num = open(path, 32768, 0);
		if (num == -1)
		{
			fsw.DispatchErrorEvents(new ErrorEventArgs(new IOException($"open() error while attempting to process path '{path}', error code = '{Marshal.GetLastWin32Error()}'")));
			return null;
		}
		try
		{
			fds.Add(num);
			FileAttributes attributes = File.GetAttributes(path);
			value = new PathData
			{
				Path = path,
				Fd = num,
				IsDirectory = ((attributes & FileAttributes.Directory) == FileAttributes.Directory)
			};
			pathsDict.Add(path, value);
			fdsDict.Add(num, value);
			if (postEvents)
			{
				PostEvent(FileAction.Added, path);
			}
			return value;
		}
		catch (Exception exception)
		{
			close(num);
			fsw.DispatchErrorEvents(new ErrorEventArgs(exception));
			return null;
		}
	}

	private void Remove(PathData pathData)
	{
		fdsDict.Remove(pathData.Fd);
		pathsDict.Remove(pathData.Path);
		close(pathData.Fd);
		PostEvent(FileAction.Removed, pathData.Path);
	}

	private void RemoveTree(PathData pathData)
	{
		List<PathData> list = new List<PathData>();
		list.Add(pathData);
		if (pathData.IsDirectory)
		{
			string value = pathData.Path + Path.DirectorySeparatorChar;
			foreach (string key in pathsDict.Keys)
			{
				if (key.StartsWith(value))
				{
					list.Add(pathsDict[key]);
				}
			}
		}
		list.ForEach(Remove);
	}

	private void UpdatePath(PathData pathData)
	{
		string filenameFromFd = GetFilenameFromFd(pathData.Fd);
		if (!filenameFromFd.StartsWith(fullPathNoLastSlash))
		{
			RemoveTree(pathData);
			return;
		}
		List<PathData> list = new List<PathData>();
		string path = pathData.Path;
		list.Add(pathData);
		if (pathData.IsDirectory)
		{
			string value = path + Path.DirectorySeparatorChar;
			foreach (string key2 in pathsDict.Keys)
			{
				if (key2.StartsWith(value))
				{
					list.Add(pathsDict[key2]);
				}
			}
		}
		foreach (PathData item in list)
		{
			string path2 = item.Path;
			string key = (item.Path = filenameFromFd + path2.Substring(path.Length));
			pathsDict.Remove(path2);
			if (pathsDict.ContainsKey(key))
			{
				PathData pathData2 = pathsDict[key];
				if (GetFilenameFromFd(item.Fd) == GetFilenameFromFd(pathData2.Fd))
				{
					Remove(pathData2);
				}
				else
				{
					UpdatePath(pathData2);
				}
			}
			pathsDict.Add(key, item);
		}
		PostEvent(FileAction.RenamedNewName, path, filenameFromFd);
	}

	private void Scan(string path, bool postEvents, ref List<int> fds)
	{
		if (requestStop)
		{
			return;
		}
		PathData pathData = Add(path, postEvents, ref fds);
		if (pathData == null || !pathData.IsDirectory)
		{
			return;
		}
		List<string> list = new List<string>();
		list.Add(path);
		while (list.Count > 0)
		{
			string path2 = list[0];
			list.RemoveAt(0);
			DirectoryInfo directoryInfo = new DirectoryInfo(path2);
			FileSystemInfo[] array = null;
			try
			{
				array = directoryInfo.GetFileSystemInfos();
			}
			catch (IOException)
			{
				array = new FileSystemInfo[0];
			}
			FileSystemInfo[] array2 = array;
			foreach (FileSystemInfo fileSystemInfo in array2)
			{
				if (((fileSystemInfo.Attributes & FileAttributes.Directory) != FileAttributes.Directory || fsw.IncludeSubdirectories) && ((fileSystemInfo.Attributes & FileAttributes.Directory) == FileAttributes.Directory || fsw.Pattern.IsMatch(fileSystemInfo.FullName)))
				{
					PathData pathData2 = Add(fileSystemInfo.FullName, postEvents, ref fds);
					if (pathData2 != null && pathData2.IsDirectory)
					{
						list.Add(fileSystemInfo.FullName);
					}
				}
			}
		}
	}

	private void PostEvent(FileAction action, string path, string newPath = null)
	{
		RenamedEventArgs renamed = null;
		if (requestStop || action == (FileAction)0)
		{
			return;
		}
		string text = ((path.Length > fullPathNoLastSlash.Length) ? path.Substring(fullPathNoLastSlash.Length + 1) : string.Empty);
		if (!fsw.Pattern.IsMatch(path) && (newPath == null || !fsw.Pattern.IsMatch(newPath)))
		{
			return;
		}
		if (action == FileAction.RenamedNewName)
		{
			string name = ((newPath.Length > fullPathNoLastSlash.Length) ? newPath.Substring(fullPathNoLastSlash.Length + 1) : string.Empty);
			renamed = new RenamedEventArgs(WatcherChangeTypes.Renamed, fsw.Path, name, text);
		}
		fsw.DispatchEvents(action, text, ref renamed);
		if (!fsw.Waiting)
		{
			return;
		}
		lock (fsw)
		{
			fsw.Waiting = false;
			System.Threading.Monitor.PulseAll(fsw);
		}
	}

	private string GetFilenameFromFd(int fd)
	{
		StringBuilder stringBuilder = new StringBuilder(1024);
		if (fcntl(fd, 50, stringBuilder) != -1)
		{
			if (fixupPath != null)
			{
				stringBuilder.Replace(fixupPath, fullPathNoLastSlash, 0, fixupPath.Length);
			}
			return stringBuilder.ToString();
		}
		fsw.DispatchErrorEvents(new ErrorEventArgs(new IOException($"fcntl() error while attempting to get path for fd '{fd}', error code = '{Marshal.GetLastWin32Error()}'")));
		return string.Empty;
	}

	[DllImport("libc", CharSet = CharSet.Auto, SetLastError = true)]
	private static extern int fcntl(int file_names_by_descriptor, int cmd, StringBuilder sb);

	[DllImport("libc", CharSet = CharSet.Auto, SetLastError = true)]
	private static extern IntPtr realpath(string pathname, StringBuilder sb);

	[DllImport("libc", SetLastError = true)]
	private static extern int open(string path, int flags, int mode_t);

	[DllImport("libc")]
	private static extern int close(int fd);

	[DllImport("libc", SetLastError = true)]
	private static extern int kqueue();

	[DllImport("libc", SetLastError = true)]
	private static extern int kevent(int kq, [In] kevent[] ev, int nchanges, [Out] kevent[] evtlist, int nevents, [In] ref timespec time);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern int kevent_notimeout(ref int kq, IntPtr ev, int nchanges, IntPtr evtlist, int nevents);
}
