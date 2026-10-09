using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security;
using System.Security.AccessControl;
using Microsoft.Win32.SafeHandles;

namespace System.IO;

[Serializable]
[ComVisible(true)]
public sealed class DirectoryInfo : FileSystemInfo
{
	private string current;

	private string parent;

	public override bool Exists
	{
		get
		{
			if (_dataInitialised == -1)
			{
				Refresh();
			}
			if (_data.fileAttributes == (FileAttributes)(-1))
			{
				return false;
			}
			if ((_data.fileAttributes & FileAttributes.Directory) == 0)
			{
				return false;
			}
			return true;
		}
	}

	public override string Name => current;

	public DirectoryInfo Parent
	{
		get
		{
			if (parent == null || parent.Length == 0)
			{
				return null;
			}
			return new DirectoryInfo(parent);
		}
	}

	public DirectoryInfo Root
	{
		get
		{
			string pathRoot = Path.GetPathRoot(FullPath);
			if (pathRoot == null)
			{
				return null;
			}
			return new DirectoryInfo(pathRoot);
		}
	}

	public DirectoryInfo(string path)
		: this(path, simpleOriginalPath: false)
	{
	}

	internal DirectoryInfo(string path, bool simpleOriginalPath)
	{
		CheckPath(path);
		SecurityManager.EnsureElevatedPermissions();
		FullPath = Path.GetFullPath(path);
		if (simpleOriginalPath)
		{
			OriginalPath = Path.GetFileName(FullPath);
		}
		else
		{
			OriginalPath = path;
		}
		Initialize();
	}

	private DirectoryInfo(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
		Initialize();
	}

	private void Initialize()
	{
		int num = FullPath.Length - 1;
		if (num > 1 && FullPath[num] == Path.DirectorySeparatorChar)
		{
			num--;
		}
		int num2 = FullPath.LastIndexOf(Path.DirectorySeparatorChar, num);
		if (num2 == -1 || (num2 == 0 && num == 0))
		{
			current = FullPath;
			parent = null;
			return;
		}
		current = FullPath.Substring(num2 + 1, num - num2);
		if (num2 == 0 && !Environment.IsRunningOnWindows)
		{
			parent = Path.DirectorySeparatorStr;
		}
		else
		{
			parent = FullPath.Substring(0, num2);
		}
		if (Environment.IsRunningOnWindows && parent.Length == 2 && parent[1] == ':' && char.IsLetter(parent[0]))
		{
			parent += Path.DirectorySeparatorChar;
		}
	}

	public void Create()
	{
		Directory.CreateDirectory(FullPath);
	}

	public DirectoryInfo CreateSubdirectory(string path)
	{
		CheckPath(path);
		path = Path.Combine(FullPath, path);
		Directory.CreateDirectory(path);
		return new DirectoryInfo(path);
	}

	public FileInfo[] GetFiles()
	{
		return GetFiles("*");
	}

	public FileInfo[] GetFiles(string searchPattern)
	{
		if (searchPattern == null)
		{
			throw new ArgumentNullException("searchPattern");
		}
		string[] files = Directory.GetFiles(FullPath, searchPattern);
		FileInfo[] array = new FileInfo[files.Length];
		int num = 0;
		string[] array2 = files;
		foreach (string fileName in array2)
		{
			array[num++] = new FileInfo(fileName);
		}
		return array;
	}

	public DirectoryInfo[] GetDirectories()
	{
		return GetDirectories("*");
	}

	public DirectoryInfo[] GetDirectories(string searchPattern)
	{
		if (searchPattern == null)
		{
			throw new ArgumentNullException("searchPattern");
		}
		string[] directories = Directory.GetDirectories(FullPath, searchPattern);
		DirectoryInfo[] array = new DirectoryInfo[directories.Length];
		int num = 0;
		string[] array2 = directories;
		foreach (string path in array2)
		{
			array[num++] = new DirectoryInfo(path);
		}
		return array;
	}

	public FileSystemInfo[] GetFileSystemInfos()
	{
		return GetFileSystemInfos("*");
	}

	public FileSystemInfo[] GetFileSystemInfos(string searchPattern)
	{
		return GetFileSystemInfos(searchPattern, SearchOption.TopDirectoryOnly);
	}

	public FileSystemInfo[] GetFileSystemInfos(string searchPattern, SearchOption searchOption)
	{
		if (searchPattern == null)
		{
			throw new ArgumentNullException("searchPattern");
		}
		if (searchOption != 0 && searchOption != SearchOption.AllDirectories)
		{
			throw new ArgumentOutOfRangeException("searchOption", "Must be TopDirectoryOnly or AllDirectories");
		}
		if (!Directory.Exists(FullPath))
		{
			throw new IOException("Invalid directory");
		}
		List<FileSystemInfo> list = new List<FileSystemInfo>();
		InternalGetFileSystemInfos(searchPattern, searchOption, list);
		return list.ToArray();
	}

	private void InternalGetFileSystemInfos(string searchPattern, SearchOption searchOption, List<FileSystemInfo> infos)
	{
		string[] directories = Directory.GetDirectories(FullPath, searchPattern);
		string[] files = Directory.GetFiles(FullPath, searchPattern);
		Array.ForEach(directories, delegate(string dir)
		{
			infos.Add(new DirectoryInfo(dir));
		});
		Array.ForEach(files, delegate(string file)
		{
			infos.Add(new FileInfo(file));
		});
		if (directories.Length != 0 && searchOption != 0)
		{
			string[] array = directories;
			for (int i = 0; i < array.Length; i++)
			{
				new DirectoryInfo(array[i]).InternalGetFileSystemInfos(searchPattern, searchOption, infos);
			}
		}
	}

	public override void Delete()
	{
		Delete(recursive: false);
	}

	public void Delete(bool recursive)
	{
		Directory.Delete(FullPath, recursive);
	}

	public void MoveTo(string destDirName)
	{
		if (destDirName == null)
		{
			throw new ArgumentNullException("destDirName");
		}
		if (destDirName.Length == 0)
		{
			throw new ArgumentException("An empty file name is not valid.", "destDirName");
		}
		Directory.Move(FullPath, Path.GetFullPath(destDirName));
		FullPath = (OriginalPath = destDirName);
		Initialize();
	}

	public override string ToString()
	{
		return OriginalPath;
	}

	public DirectoryInfo[] GetDirectories(string searchPattern, SearchOption searchOption)
	{
		string[] directories = Directory.GetDirectories(FullPath, searchPattern, searchOption);
		DirectoryInfo[] array = new DirectoryInfo[directories.Length];
		for (int i = 0; i < directories.Length; i++)
		{
			string path = directories[i];
			array[i] = new DirectoryInfo(path);
		}
		return array;
	}

	internal int GetFilesSubdirs(ArrayList l, string pattern)
	{
		FileInfo[] array = null;
		try
		{
			array = GetFiles(pattern);
		}
		catch (UnauthorizedAccessException)
		{
			return 0;
		}
		int num = array.Length;
		l.Add(array);
		DirectoryInfo[] directories = GetDirectories();
		foreach (DirectoryInfo directoryInfo in directories)
		{
			num += directoryInfo.GetFilesSubdirs(l, pattern);
		}
		return num;
	}

	public FileInfo[] GetFiles(string searchPattern, SearchOption searchOption)
	{
		switch (searchOption)
		{
		case SearchOption.TopDirectoryOnly:
			return GetFiles(searchPattern);
		case SearchOption.AllDirectories:
		{
			ArrayList arrayList = new ArrayList();
			int filesSubdirs = GetFilesSubdirs(arrayList, searchPattern);
			int num = 0;
			FileInfo[] array = new FileInfo[filesSubdirs];
			{
				foreach (FileInfo[] item in arrayList)
				{
					item.CopyTo(array, num);
					num += item.Length;
				}
				return array;
			}
		}
		default:
		{
			string text = Locale.GetText("Invalid enum value '{0}' for '{1}'.", searchOption, "SearchOption");
			throw new ArgumentOutOfRangeException("searchOption", text);
		}
		}
	}

	[MonoLimitation("DirectorySecurity isn't implemented")]
	public void Create(DirectorySecurity directorySecurity)
	{
		if (directorySecurity != null)
		{
			throw new UnauthorizedAccessException();
		}
		Create();
	}

	[MonoLimitation("DirectorySecurity isn't implemented")]
	public DirectoryInfo CreateSubdirectory(string path, DirectorySecurity directorySecurity)
	{
		if (directorySecurity != null)
		{
			throw new UnauthorizedAccessException();
		}
		return CreateSubdirectory(path);
	}

	public DirectorySecurity GetAccessControl()
	{
		return Directory.GetAccessControl(FullPath);
	}

	public DirectorySecurity GetAccessControl(AccessControlSections includeSections)
	{
		return Directory.GetAccessControl(FullPath, includeSections);
	}

	public void SetAccessControl(DirectorySecurity directorySecurity)
	{
		Directory.SetAccessControl(FullPath, directorySecurity);
	}

	public IEnumerable<DirectoryInfo> EnumerateDirectories()
	{
		return EnumerateDirectories("*", SearchOption.TopDirectoryOnly);
	}

	public IEnumerable<DirectoryInfo> EnumerateDirectories(string searchPattern)
	{
		return EnumerateDirectories(searchPattern, SearchOption.TopDirectoryOnly);
	}

	public IEnumerable<DirectoryInfo> EnumerateDirectories(string searchPattern, SearchOption searchOption)
	{
		if (searchPattern == null)
		{
			throw new ArgumentNullException("searchPattern");
		}
		return CreateEnumerateDirectoriesIterator(searchPattern, searchOption);
	}

	private IEnumerable<DirectoryInfo> CreateEnumerateDirectoriesIterator(string searchPattern, SearchOption searchOption)
	{
		foreach (string item in Directory.EnumerateDirectories(FullPath, searchPattern, searchOption))
		{
			yield return new DirectoryInfo(item);
		}
	}

	public IEnumerable<FileInfo> EnumerateFiles()
	{
		return EnumerateFiles("*", SearchOption.TopDirectoryOnly);
	}

	public IEnumerable<FileInfo> EnumerateFiles(string searchPattern)
	{
		return EnumerateFiles(searchPattern, SearchOption.TopDirectoryOnly);
	}

	public IEnumerable<FileInfo> EnumerateFiles(string searchPattern, SearchOption searchOption)
	{
		if (searchPattern == null)
		{
			throw new ArgumentNullException("searchPattern");
		}
		return CreateEnumerateFilesIterator(searchPattern, searchOption);
	}

	private IEnumerable<FileInfo> CreateEnumerateFilesIterator(string searchPattern, SearchOption searchOption)
	{
		foreach (string item in Directory.EnumerateFiles(FullPath, searchPattern, searchOption))
		{
			yield return new FileInfo(item);
		}
	}

	public IEnumerable<FileSystemInfo> EnumerateFileSystemInfos()
	{
		return EnumerateFileSystemInfos("*", SearchOption.TopDirectoryOnly);
	}

	public IEnumerable<FileSystemInfo> EnumerateFileSystemInfos(string searchPattern)
	{
		return EnumerateFileSystemInfos(searchPattern, SearchOption.TopDirectoryOnly);
	}

	public IEnumerable<FileSystemInfo> EnumerateFileSystemInfos(string searchPattern, SearchOption searchOption)
	{
		if (searchPattern == null)
		{
			throw new ArgumentNullException("searchPattern");
		}
		if (searchOption != 0 && searchOption != SearchOption.AllDirectories)
		{
			throw new ArgumentOutOfRangeException("searchoption");
		}
		return EnumerateFileSystemInfos(FullPath, searchPattern, searchOption);
	}

	internal static IEnumerable<FileSystemInfo> EnumerateFileSystemInfos(string basePath, string searchPattern, SearchOption searchOption)
	{
		Path.Validate(basePath);
		SafeFindHandle findHandle = null;
		try
		{
			string text = Path.Combine(basePath, searchPattern);
			string fileName;
			int fileAttr;
			int error;
			try
			{
			}
			finally
			{
				findHandle = new SafeFindHandle(MonoIO.FindFirstFile(text, out fileName, out fileAttr, out error));
			}
			if (findHandle.IsInvalid)
			{
				MonoIOError monoIOError = (MonoIOError)error;
				if (monoIOError != MonoIOError.ERROR_FILE_NOT_FOUND)
				{
					throw MonoIO.GetException(Path.GetDirectoryName(text), monoIOError);
				}
				yield break;
			}
			while (fileName != null)
			{
				if (!(fileName == ".") && !(fileName == ".."))
				{
					FileAttributes attrs = (FileAttributes)fileAttr;
					string fullPath = Path.Combine(basePath, fileName);
					if ((attrs & FileAttributes.ReparsePoint) == 0)
					{
						if ((attrs & FileAttributes.Directory) != 0)
						{
							yield return new DirectoryInfo(fullPath);
						}
						else
						{
							yield return new FileInfo(fullPath);
						}
					}
					if ((attrs & FileAttributes.Directory) != 0 && searchOption == SearchOption.AllDirectories)
					{
						foreach (FileSystemInfo item in EnumerateFileSystemInfos(fullPath, searchPattern, searchOption))
						{
							yield return item;
						}
					}
				}
				if (!MonoIO.FindNextFile(findHandle.DangerousGetHandle(), out fileName, out fileAttr, out var _))
				{
					break;
				}
			}
		}
		finally
		{
			findHandle?.Dispose();
		}
	}

	internal void CheckPath(string path)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		if (path.Length == 0)
		{
			throw new ArgumentException("An empty file name is not valid.");
		}
		if (path.IndexOfAny(Path.InvalidPathChars) != -1)
		{
			throw new ArgumentException("Illegal characters in path.");
		}
		if (Environment.IsRunningOnWindows)
		{
			int num = path.IndexOf(':');
			if (num >= 0 && num != 1)
			{
				throw new ArgumentException("path");
			}
		}
	}
}
