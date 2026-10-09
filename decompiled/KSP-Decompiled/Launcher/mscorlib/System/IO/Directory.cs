using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Security;
using System.Security.AccessControl;
using System.Security.Permissions;

namespace System.IO;

[ComVisible(true)]
public static class Directory
{
	internal sealed class SearchData
	{
		public readonly string fullPath;

		public readonly string userPath;

		public readonly SearchOption searchOption;

		public SearchData(string fullPath, string userPath, SearchOption searchOption)
		{
			this.fullPath = fullPath;
			this.userPath = userPath;
			this.searchOption = searchOption;
		}
	}

	public static string[] GetFiles(string path)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		return InternalGetFiles(path, "*", SearchOption.TopDirectoryOnly);
	}

	public static string[] GetFiles(string path, string searchPattern)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		if (searchPattern == null)
		{
			throw new ArgumentNullException("searchPattern");
		}
		return InternalGetFiles(path, searchPattern, SearchOption.TopDirectoryOnly);
	}

	public static string[] GetFiles(string path, string searchPattern, SearchOption searchOption)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		if (searchPattern == null)
		{
			throw new ArgumentNullException("searchPattern");
		}
		if (searchOption != 0 && searchOption != SearchOption.AllDirectories)
		{
			throw new ArgumentOutOfRangeException("searchOption", Environment.GetResourceString("Enum value was out of legal range."));
		}
		return InternalGetFiles(path, searchPattern, searchOption);
	}

	private static string[] InternalGetFiles(string path, string searchPattern, SearchOption searchOption)
	{
		return InternalGetFileDirectoryNames(path, path, searchPattern, includeFiles: true, includeDirs: false, searchOption, checkHost: true);
	}

	[SecurityCritical]
	internal static string[] UnsafeGetFiles(string path, string searchPattern, SearchOption searchOption)
	{
		return InternalGetFileDirectoryNames(path, path, searchPattern, includeFiles: true, includeDirs: false, searchOption, checkHost: false);
	}

	public static string[] GetDirectories(string path)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		return InternalGetDirectories(path, "*", SearchOption.TopDirectoryOnly);
	}

	public static string[] GetDirectories(string path, string searchPattern)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		if (searchPattern == null)
		{
			throw new ArgumentNullException("searchPattern");
		}
		return InternalGetDirectories(path, searchPattern, SearchOption.TopDirectoryOnly);
	}

	public static string[] GetDirectories(string path, string searchPattern, SearchOption searchOption)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		if (searchPattern == null)
		{
			throw new ArgumentNullException("searchPattern");
		}
		if (searchOption != 0 && searchOption != SearchOption.AllDirectories)
		{
			throw new ArgumentOutOfRangeException("searchOption", Environment.GetResourceString("Enum value was out of legal range."));
		}
		return InternalGetDirectories(path, searchPattern, searchOption);
	}

	private static string[] InternalGetDirectories(string path, string searchPattern, SearchOption searchOption)
	{
		return InternalGetFileDirectoryNames(path, path, searchPattern, includeFiles: false, includeDirs: true, searchOption, checkHost: true);
	}

	[SecurityCritical]
	internal static string[] UnsafeGetDirectories(string path, string searchPattern, SearchOption searchOption)
	{
		return InternalGetFileDirectoryNames(path, path, searchPattern, includeFiles: false, includeDirs: true, searchOption, checkHost: false);
	}

	public static string[] GetFileSystemEntries(string path)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		return InternalGetFileSystemEntries(path, "*", SearchOption.TopDirectoryOnly);
	}

	public static string[] GetFileSystemEntries(string path, string searchPattern)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		if (searchPattern == null)
		{
			throw new ArgumentNullException("searchPattern");
		}
		return InternalGetFileSystemEntries(path, searchPattern, SearchOption.TopDirectoryOnly);
	}

	public static string[] GetFileSystemEntries(string path, string searchPattern, SearchOption searchOption)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		if (searchPattern == null)
		{
			throw new ArgumentNullException("searchPattern");
		}
		if (searchOption != 0 && searchOption != SearchOption.AllDirectories)
		{
			throw new ArgumentOutOfRangeException("searchOption", Environment.GetResourceString("Enum value was out of legal range."));
		}
		return InternalGetFileSystemEntries(path, searchPattern, searchOption);
	}

	private static string[] InternalGetFileSystemEntries(string path, string searchPattern, SearchOption searchOption)
	{
		return InternalGetFileDirectoryNames(path, path, searchPattern, includeFiles: true, includeDirs: true, searchOption, checkHost: true);
	}

	internal static string[] InternalGetFileDirectoryNames(string path, string userPathOriginal, string searchPattern, bool includeFiles, bool includeDirs, SearchOption searchOption, bool checkHost)
	{
		return new List<string>(FileSystemEnumerableFactory.CreateFileNameIterator(path, userPathOriginal, searchPattern, includeFiles, includeDirs, searchOption, checkHost)).ToArray();
	}

	public static IEnumerable<string> EnumerateDirectories(string path)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		return InternalEnumerateDirectories(path, "*", SearchOption.TopDirectoryOnly);
	}

	public static IEnumerable<string> EnumerateDirectories(string path, string searchPattern)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		if (searchPattern == null)
		{
			throw new ArgumentNullException("searchPattern");
		}
		return InternalEnumerateDirectories(path, searchPattern, SearchOption.TopDirectoryOnly);
	}

	public static IEnumerable<string> EnumerateDirectories(string path, string searchPattern, SearchOption searchOption)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		if (searchPattern == null)
		{
			throw new ArgumentNullException("searchPattern");
		}
		if (searchOption != 0 && searchOption != SearchOption.AllDirectories)
		{
			throw new ArgumentOutOfRangeException("searchOption", Environment.GetResourceString("Enum value was out of legal range."));
		}
		return InternalEnumerateDirectories(path, searchPattern, searchOption);
	}

	private static IEnumerable<string> InternalEnumerateDirectories(string path, string searchPattern, SearchOption searchOption)
	{
		return EnumerateFileSystemNames(path, searchPattern, searchOption, includeFiles: false, includeDirs: true);
	}

	public static IEnumerable<string> EnumerateFiles(string path)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		return InternalEnumerateFiles(path, "*", SearchOption.TopDirectoryOnly);
	}

	public static IEnumerable<string> EnumerateFiles(string path, string searchPattern)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		if (searchPattern == null)
		{
			throw new ArgumentNullException("searchPattern");
		}
		return InternalEnumerateFiles(path, searchPattern, SearchOption.TopDirectoryOnly);
	}

	public static IEnumerable<string> EnumerateFiles(string path, string searchPattern, SearchOption searchOption)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		if (searchPattern == null)
		{
			throw new ArgumentNullException("searchPattern");
		}
		if (searchOption != 0 && searchOption != SearchOption.AllDirectories)
		{
			throw new ArgumentOutOfRangeException("searchOption", Environment.GetResourceString("Enum value was out of legal range."));
		}
		return InternalEnumerateFiles(path, searchPattern, searchOption);
	}

	private static IEnumerable<string> InternalEnumerateFiles(string path, string searchPattern, SearchOption searchOption)
	{
		return EnumerateFileSystemNames(path, searchPattern, searchOption, includeFiles: true, includeDirs: false);
	}

	public static IEnumerable<string> EnumerateFileSystemEntries(string path)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		return InternalEnumerateFileSystemEntries(path, "*", SearchOption.TopDirectoryOnly);
	}

	public static IEnumerable<string> EnumerateFileSystemEntries(string path, string searchPattern)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		if (searchPattern == null)
		{
			throw new ArgumentNullException("searchPattern");
		}
		return InternalEnumerateFileSystemEntries(path, searchPattern, SearchOption.TopDirectoryOnly);
	}

	public static IEnumerable<string> EnumerateFileSystemEntries(string path, string searchPattern, SearchOption searchOption)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		if (searchPattern == null)
		{
			throw new ArgumentNullException("searchPattern");
		}
		if (searchOption != 0 && searchOption != SearchOption.AllDirectories)
		{
			throw new ArgumentOutOfRangeException("searchOption", Environment.GetResourceString("Enum value was out of legal range."));
		}
		return InternalEnumerateFileSystemEntries(path, searchPattern, searchOption);
	}

	private static IEnumerable<string> InternalEnumerateFileSystemEntries(string path, string searchPattern, SearchOption searchOption)
	{
		return EnumerateFileSystemNames(path, searchPattern, searchOption, includeFiles: true, includeDirs: true);
	}

	private static IEnumerable<string> EnumerateFileSystemNames(string path, string searchPattern, SearchOption searchOption, bool includeFiles, bool includeDirs)
	{
		return FileSystemEnumerableFactory.CreateFileNameIterator(path, path, searchPattern, includeFiles, includeDirs, searchOption, checkHost: true);
	}

	public static string GetDirectoryRoot(string path)
	{
		Path.Validate(path);
		SecurityManager.EnsureElevatedPermissions();
		return new string(Path.DirectorySeparatorChar, 1);
	}

	public static DirectoryInfo CreateDirectory(string path)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		if (path.Length == 0)
		{
			throw new ArgumentException("Path is empty");
		}
		if (path.IndexOfAny(Path.InvalidPathChars) != -1)
		{
			throw new ArgumentException("Path contains invalid chars");
		}
		if (path.Trim().Length == 0)
		{
			throw new ArgumentException("Only blank characters in path");
		}
		SecurityManager.EnsureElevatedPermissions();
		if (File.Exists(path))
		{
			throw new IOException("Cannot create " + path + " because a file with the same name already exists.");
		}
		if (Environment.IsRunningOnWindows && path == ":")
		{
			throw new ArgumentException("Only ':' In path");
		}
		return CreateDirectoriesInternal(path);
	}

	[MonoLimitation("DirectorySecurity not implemented")]
	public static DirectoryInfo CreateDirectory(string path, DirectorySecurity directorySecurity)
	{
		return CreateDirectory(path);
	}

	private static DirectoryInfo CreateDirectoriesInternal(string path)
	{
		DirectoryInfo directoryInfo = new DirectoryInfo(path, simpleOriginalPath: true);
		if (directoryInfo.Parent != null && !directoryInfo.Parent.Exists)
		{
			directoryInfo.Parent.Create();
		}
		if (!MonoIO.CreateDirectory(directoryInfo.FullName, out var error) && error != MonoIOError.ERROR_ALREADY_EXISTS && error != MonoIOError.ERROR_FILE_EXISTS)
		{
			throw MonoIO.GetException(path, error);
		}
		return directoryInfo;
	}

	public static void Delete(string path)
	{
		Path.Validate(path);
		if (Environment.IsRunningOnWindows && path == ":")
		{
			throw new NotSupportedException("Only ':' In path");
		}
		SecurityManager.EnsureElevatedPermissions();
		if ((!MonoIO.ExistsSymlink(path, out var error)) ? MonoIO.RemoveDirectory(path, out error) : MonoIO.DeleteFile(path, out error))
		{
			return;
		}
		if (error == MonoIOError.ERROR_FILE_NOT_FOUND)
		{
			if (File.Exists(path))
			{
				throw new IOException("Directory does not exist, but a file of the same name exists.");
			}
			throw new DirectoryNotFoundException("Directory does not exist.");
		}
		throw MonoIO.GetException(path, error);
	}

	private static void RecursiveDelete(string path)
	{
		string[] directories = GetDirectories(path);
		foreach (string path2 in directories)
		{
			if (MonoIO.ExistsSymlink(path2, out var error))
			{
				MonoIO.DeleteFile(path2, out error);
			}
			else
			{
				RecursiveDelete(path2);
			}
		}
		directories = GetFiles(path);
		for (int i = 0; i < directories.Length; i++)
		{
			File.Delete(directories[i]);
		}
		Delete(path);
	}

	public static void Delete(string path, bool recursive)
	{
		Path.Validate(path);
		SecurityManager.EnsureElevatedPermissions();
		if (recursive)
		{
			RecursiveDelete(path);
		}
		else
		{
			Delete(path);
		}
	}

	public static bool Exists(string path)
	{
		if (path == null)
		{
			return false;
		}
		if (!SecurityManager.CheckElevatedPermissions())
		{
			return false;
		}
		string fullPath;
		try
		{
			fullPath = Path.GetFullPath(path);
		}
		catch
		{
			return false;
		}
		MonoIOError error;
		return MonoIO.ExistsDirectory(fullPath, out error);
	}

	public static DateTime GetLastAccessTime(string path)
	{
		return File.GetLastAccessTime(path);
	}

	public static DateTime GetLastAccessTimeUtc(string path)
	{
		return GetLastAccessTime(path).ToUniversalTime();
	}

	public static DateTime GetLastWriteTime(string path)
	{
		return File.GetLastWriteTime(path);
	}

	public static DateTime GetLastWriteTimeUtc(string path)
	{
		return GetLastWriteTime(path).ToUniversalTime();
	}

	public static DateTime GetCreationTime(string path)
	{
		return File.GetCreationTime(path);
	}

	public static DateTime GetCreationTimeUtc(string path)
	{
		return GetCreationTime(path).ToUniversalTime();
	}

	public static string GetCurrentDirectory()
	{
		SecurityManager.EnsureElevatedPermissions();
		return InsecureGetCurrentDirectory();
	}

	internal static string InsecureGetCurrentDirectory()
	{
		MonoIOError error;
		string currentDirectory = MonoIO.GetCurrentDirectory(out error);
		if (error != 0)
		{
			throw MonoIO.GetException(error);
		}
		return currentDirectory;
	}

	public static string[] GetLogicalDrives()
	{
		return Environment.GetLogicalDrives();
	}

	private static bool IsRootDirectory(string path)
	{
		if (Path.DirectorySeparatorChar == '/' && path == "/")
		{
			return true;
		}
		if (Path.DirectorySeparatorChar == '\\' && path.Length == 3 && path.EndsWith(":\\"))
		{
			return true;
		}
		return false;
	}

	public static DirectoryInfo GetParent(string path)
	{
		Path.Validate(path);
		if (IsRootDirectory(path))
		{
			return null;
		}
		string text = Path.GetDirectoryName(path);
		if (text.Length == 0)
		{
			text = GetCurrentDirectory();
		}
		return new DirectoryInfo(text);
	}

	public static void Move(string sourceDirName, string destDirName)
	{
		if (sourceDirName == null)
		{
			throw new ArgumentNullException("sourceDirName");
		}
		if (destDirName == null)
		{
			throw new ArgumentNullException("destDirName");
		}
		if (sourceDirName.Trim().Length == 0 || sourceDirName.IndexOfAny(Path.InvalidPathChars) != -1)
		{
			throw new ArgumentException("Invalid source directory name: " + sourceDirName, "sourceDirName");
		}
		if (destDirName.Trim().Length == 0 || destDirName.IndexOfAny(Path.InvalidPathChars) != -1)
		{
			throw new ArgumentException("Invalid target directory name: " + destDirName, "destDirName");
		}
		if (sourceDirName == destDirName)
		{
			throw new IOException("Source and destination path must be different.");
		}
		SecurityManager.EnsureElevatedPermissions();
		if (Exists(destDirName))
		{
			throw new IOException(destDirName + " already exists.");
		}
		if (!Exists(sourceDirName) && !File.Exists(sourceDirName))
		{
			throw new DirectoryNotFoundException(sourceDirName + " does not exist");
		}
		if (!MonoIO.MoveFile(sourceDirName, destDirName, out var error))
		{
			throw MonoIO.GetException(error);
		}
	}

	public static void SetAccessControl(string path, DirectorySecurity directorySecurity)
	{
		if (directorySecurity == null)
		{
			throw new ArgumentNullException("directorySecurity");
		}
		directorySecurity.PersistModifications(path);
	}

	public static void SetCreationTime(string path, DateTime creationTime)
	{
		File.SetCreationTime(path, creationTime);
	}

	public static void SetCreationTimeUtc(string path, DateTime creationTimeUtc)
	{
		SetCreationTime(path, creationTimeUtc.ToLocalTime());
	}

	[SecurityPermission(SecurityAction.Demand, UnmanagedCode = true)]
	public static void SetCurrentDirectory(string path)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		if (path.Trim().Length == 0)
		{
			throw new ArgumentException("path string must not be an empty string or whitespace string");
		}
		if (!Exists(path))
		{
			throw new DirectoryNotFoundException("Directory \"" + path + "\" not found.");
		}
		MonoIO.SetCurrentDirectory(path, out var error);
		if (error != 0)
		{
			throw MonoIO.GetException(path, error);
		}
	}

	public static void SetLastAccessTime(string path, DateTime lastAccessTime)
	{
		File.SetLastAccessTime(path, lastAccessTime);
	}

	public static void SetLastAccessTimeUtc(string path, DateTime lastAccessTimeUtc)
	{
		SetLastAccessTime(path, lastAccessTimeUtc.ToLocalTime());
	}

	public static void SetLastWriteTime(string path, DateTime lastWriteTime)
	{
		File.SetLastWriteTime(path, lastWriteTime);
	}

	public static void SetLastWriteTimeUtc(string path, DateTime lastWriteTimeUtc)
	{
		SetLastWriteTime(path, lastWriteTimeUtc.ToLocalTime());
	}

	public static DirectorySecurity GetAccessControl(string path, AccessControlSections includeSections)
	{
		return new DirectorySecurity(path, includeSections);
	}

	public static DirectorySecurity GetAccessControl(string path)
	{
		return GetAccessControl(path, AccessControlSections.Access | AccessControlSections.Owner | AccessControlSections.Group);
	}

	internal static string GetDemandDir(string fullPath, bool thisDirOnly)
	{
		if (thisDirOnly)
		{
			if (fullPath.EndsWith(Path.DirectorySeparatorChar) || fullPath.EndsWith(Path.AltDirectorySeparatorChar))
			{
				return fullPath + ".";
			}
			return fullPath + Path.DirectorySeparatorCharAsString + ".";
		}
		if (!fullPath.EndsWith(Path.DirectorySeparatorChar) && !fullPath.EndsWith(Path.AltDirectorySeparatorChar))
		{
			return fullPath + Path.DirectorySeparatorCharAsString;
		}
		return fullPath;
	}
}
