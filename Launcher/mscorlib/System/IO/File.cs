using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Security;
using System.Security.AccessControl;
using System.Text;

namespace System.IO;

[ComVisible(true)]
public static class File
{
	private static DateTime? defaultLocalFileTime;

	private static DateTime DefaultLocalFileTime
	{
		get
		{
			if (!defaultLocalFileTime.HasValue)
			{
				defaultLocalFileTime = new DateTime(1601, 1, 1).ToLocalTime();
			}
			return defaultLocalFileTime.Value;
		}
	}

	public static void AppendAllText(string path, string contents)
	{
		using TextWriter textWriter = new StreamWriter(path, append: true);
		textWriter.Write(contents);
	}

	public static void AppendAllText(string path, string contents, Encoding encoding)
	{
		using TextWriter textWriter = new StreamWriter(path, append: true, encoding);
		textWriter.Write(contents);
	}

	public static StreamWriter AppendText(string path)
	{
		return new StreamWriter(path, append: true);
	}

	public static void Copy(string sourceFileName, string destFileName)
	{
		Copy(sourceFileName, destFileName, overwrite: false);
	}

	public static void Copy(string sourceFileName, string destFileName, bool overwrite)
	{
		if (sourceFileName == null)
		{
			throw new ArgumentNullException("sourceFileName");
		}
		if (destFileName == null)
		{
			throw new ArgumentNullException("destFileName");
		}
		if (sourceFileName.Length == 0)
		{
			throw new ArgumentException("An empty file name is not valid.", "sourceFileName");
		}
		if (sourceFileName.Trim().Length == 0 || sourceFileName.IndexOfAny(Path.InvalidPathChars) != -1)
		{
			throw new ArgumentException("The file name is not valid.");
		}
		if (destFileName.Length == 0)
		{
			throw new ArgumentException("An empty file name is not valid.", "destFileName");
		}
		if (destFileName.Trim().Length == 0 || destFileName.IndexOfAny(Path.InvalidPathChars) != -1)
		{
			throw new ArgumentException("The file name is not valid.");
		}
		SecurityManager.EnsureElevatedPermissions();
		if (!MonoIO.Exists(sourceFileName, out var error))
		{
			throw new FileNotFoundException(Locale.GetText("{0} does not exist", sourceFileName), sourceFileName);
		}
		if ((GetAttributes(sourceFileName) & FileAttributes.Directory) == FileAttributes.Directory)
		{
			throw new ArgumentException(Locale.GetText("{0} is a directory", sourceFileName));
		}
		if (MonoIO.Exists(destFileName, out error))
		{
			if ((GetAttributes(destFileName) & FileAttributes.Directory) == FileAttributes.Directory)
			{
				throw new ArgumentException(Locale.GetText("{0} is a directory", destFileName));
			}
			if (!overwrite)
			{
				throw new IOException(Locale.GetText("{0} already exists", destFileName));
			}
		}
		string directoryName = Path.GetDirectoryName(destFileName);
		if (directoryName != string.Empty && !Directory.Exists(directoryName))
		{
			throw new DirectoryNotFoundException(Locale.GetText("Destination directory not found: {0}", directoryName));
		}
		if (!MonoIO.CopyFile(sourceFileName, destFileName, overwrite, out error))
		{
			throw MonoIO.GetException(Locale.GetText("{0}\" or \"{1}", sourceFileName, destFileName), error);
		}
	}

	internal static string InternalCopy(string sourceFileName, string destFileName, bool overwrite, bool checkHost)
	{
		string fullPathInternal = Path.GetFullPathInternal(sourceFileName);
		string fullPathInternal2 = Path.GetFullPathInternal(destFileName);
		if (!MonoIO.CopyFile(fullPathInternal, fullPathInternal2, overwrite, out var error))
		{
			throw MonoIO.GetException(Locale.GetText("{0}\" or \"{1}", sourceFileName, destFileName), error);
		}
		return fullPathInternal2;
	}

	public static FileStream Create(string path)
	{
		return Create(path, 8192);
	}

	public static FileStream Create(string path, int bufferSize)
	{
		return new FileStream(path, FileMode.Create, FileAccess.ReadWrite, FileShare.None, bufferSize);
	}

	[MonoLimitation("FileOptions are ignored")]
	public static FileStream Create(string path, int bufferSize, FileOptions options)
	{
		return new FileStream(path, FileMode.Create, FileAccess.ReadWrite, FileShare.None, bufferSize, options);
	}

	[MonoLimitation("FileOptions and FileSecurity are ignored")]
	public static FileStream Create(string path, int bufferSize, FileOptions options, FileSecurity fileSecurity)
	{
		return new FileStream(path, FileMode.Create, FileAccess.ReadWrite, FileShare.None, bufferSize, options);
	}

	public static StreamWriter CreateText(string path)
	{
		return new StreamWriter(path, append: false);
	}

	public static void Delete(string path)
	{
		Path.Validate(path);
		if (Directory.Exists(path))
		{
			throw new UnauthorizedAccessException(Locale.GetText("{0} is a directory", path));
		}
		string directoryName = Path.GetDirectoryName(path);
		if (directoryName != string.Empty && !Directory.Exists(directoryName))
		{
			throw new DirectoryNotFoundException(Locale.GetText("Could not find a part of the path \"{0}\".", path));
		}
		SecurityManager.EnsureElevatedPermissions();
		if (!MonoIO.DeleteFile(path, out var error) && error != MonoIOError.ERROR_FILE_NOT_FOUND)
		{
			throw MonoIO.GetException(path, error);
		}
	}

	public static bool Exists(string path)
	{
		if (string.IsNullOrWhiteSpace(path) || path.IndexOfAny(Path.InvalidPathChars) >= 0)
		{
			return false;
		}
		if (!SecurityManager.CheckElevatedPermissions())
		{
			return false;
		}
		MonoIOError error;
		return MonoIO.ExistsFile(path, out error);
	}

	public static FileSecurity GetAccessControl(string path)
	{
		return GetAccessControl(path, AccessControlSections.Access | AccessControlSections.Owner | AccessControlSections.Group);
	}

	public static FileSecurity GetAccessControl(string path, AccessControlSections includeSections)
	{
		return new FileSecurity(path, includeSections);
	}

	public static FileAttributes GetAttributes(string path)
	{
		Path.Validate(path);
		SecurityManager.EnsureElevatedPermissions();
		MonoIOError error;
		FileAttributes fileAttributes = MonoIO.GetFileAttributes(path, out error);
		if (error != 0)
		{
			throw MonoIO.GetException(path, error);
		}
		return fileAttributes;
	}

	public static DateTime GetCreationTime(string path)
	{
		Path.Validate(path);
		SecurityManager.EnsureElevatedPermissions();
		if (!MonoIO.GetFileStat(path, out var stat, out var error))
		{
			if (error == MonoIOError.ERROR_PATH_NOT_FOUND || error == MonoIOError.ERROR_FILE_NOT_FOUND)
			{
				return DefaultLocalFileTime;
			}
			throw new IOException(path);
		}
		return DateTime.FromFileTime(stat.CreationTime);
	}

	public static DateTime GetCreationTimeUtc(string path)
	{
		return GetCreationTime(path).ToUniversalTime();
	}

	public static DateTime GetLastAccessTime(string path)
	{
		Path.Validate(path);
		SecurityManager.EnsureElevatedPermissions();
		if (!MonoIO.GetFileStat(path, out var stat, out var error))
		{
			if (error == MonoIOError.ERROR_PATH_NOT_FOUND || error == MonoIOError.ERROR_FILE_NOT_FOUND)
			{
				return DefaultLocalFileTime;
			}
			throw new IOException(path);
		}
		return DateTime.FromFileTime(stat.LastAccessTime);
	}

	public static DateTime GetLastAccessTimeUtc(string path)
	{
		return GetLastAccessTime(path).ToUniversalTime();
	}

	public static DateTime GetLastWriteTime(string path)
	{
		Path.Validate(path);
		SecurityManager.EnsureElevatedPermissions();
		if (!MonoIO.GetFileStat(path, out var stat, out var error))
		{
			if (error == MonoIOError.ERROR_PATH_NOT_FOUND || error == MonoIOError.ERROR_FILE_NOT_FOUND)
			{
				return DefaultLocalFileTime;
			}
			throw new IOException(path);
		}
		return DateTime.FromFileTime(stat.LastWriteTime);
	}

	public static DateTime GetLastWriteTimeUtc(string path)
	{
		return GetLastWriteTime(path).ToUniversalTime();
	}

	public static void Move(string sourceFileName, string destFileName)
	{
		if (sourceFileName == null)
		{
			throw new ArgumentNullException("sourceFileName");
		}
		if (destFileName == null)
		{
			throw new ArgumentNullException("destFileName");
		}
		if (sourceFileName.Length == 0)
		{
			throw new ArgumentException("An empty file name is not valid.", "sourceFileName");
		}
		if (sourceFileName.Trim().Length == 0 || sourceFileName.IndexOfAny(Path.InvalidPathChars) != -1)
		{
			throw new ArgumentException("The file name is not valid.");
		}
		if (destFileName.Length == 0)
		{
			throw new ArgumentException("An empty file name is not valid.", "destFileName");
		}
		if (destFileName.Trim().Length == 0 || destFileName.IndexOfAny(Path.InvalidPathChars) != -1)
		{
			throw new ArgumentException("The file name is not valid.");
		}
		SecurityManager.EnsureElevatedPermissions();
		if (!MonoIO.Exists(sourceFileName, out var error))
		{
			throw new FileNotFoundException(Locale.GetText("{0} does not exist", sourceFileName), sourceFileName);
		}
		string directoryName = Path.GetDirectoryName(destFileName);
		if (directoryName != string.Empty && !Directory.Exists(directoryName))
		{
			throw new DirectoryNotFoundException(Locale.GetText("Could not find a part of the path."));
		}
		if (!MonoIO.MoveFile(sourceFileName, destFileName, out error))
		{
			switch (error)
			{
			case MonoIOError.ERROR_ALREADY_EXISTS:
				throw MonoIO.GetException(error);
			case MonoIOError.ERROR_SHARING_VIOLATION:
				throw MonoIO.GetException(sourceFileName, error);
			default:
				throw MonoIO.GetException(error);
			}
		}
	}

	public static FileStream Open(string path, FileMode mode)
	{
		return new FileStream(path, mode, (mode == FileMode.Append) ? FileAccess.Write : FileAccess.ReadWrite, FileShare.None);
	}

	public static FileStream Open(string path, FileMode mode, FileAccess access)
	{
		return new FileStream(path, mode, access, FileShare.None);
	}

	public static FileStream Open(string path, FileMode mode, FileAccess access, FileShare share)
	{
		return new FileStream(path, mode, access, share);
	}

	public static FileStream OpenRead(string path)
	{
		return new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
	}

	public static StreamReader OpenText(string path)
	{
		return new StreamReader(path);
	}

	public static FileStream OpenWrite(string path)
	{
		return new FileStream(path, FileMode.OpenOrCreate, FileAccess.Write, FileShare.None);
	}

	public static void Replace(string sourceFileName, string destinationFileName, string destinationBackupFileName)
	{
		Replace(sourceFileName, destinationFileName, destinationBackupFileName, ignoreMetadataErrors: false);
	}

	public static void Replace(string sourceFileName, string destinationFileName, string destinationBackupFileName, bool ignoreMetadataErrors)
	{
		if (sourceFileName == null)
		{
			throw new ArgumentNullException("sourceFileName");
		}
		if (destinationFileName == null)
		{
			throw new ArgumentNullException("destinationFileName");
		}
		if (sourceFileName.Trim().Length == 0 || sourceFileName.IndexOfAny(Path.InvalidPathChars) != -1)
		{
			throw new ArgumentException("sourceFileName");
		}
		if (destinationFileName.Trim().Length == 0 || destinationFileName.IndexOfAny(Path.InvalidPathChars) != -1)
		{
			throw new ArgumentException("destinationFileName");
		}
		string fullPath = Path.GetFullPath(sourceFileName);
		string fullPath2 = Path.GetFullPath(destinationFileName);
		if (MonoIO.ExistsDirectory(fullPath, out var error))
		{
			throw new IOException(Locale.GetText("{0} is a directory", sourceFileName));
		}
		if (MonoIO.ExistsDirectory(fullPath2, out error))
		{
			throw new IOException(Locale.GetText("{0} is a directory", destinationFileName));
		}
		if (!Exists(fullPath))
		{
			throw new FileNotFoundException(Locale.GetText("{0} does not exist", sourceFileName), sourceFileName);
		}
		if (!Exists(fullPath2))
		{
			throw new FileNotFoundException(Locale.GetText("{0} does not exist", destinationFileName), destinationFileName);
		}
		if (fullPath == fullPath2)
		{
			throw new IOException(Locale.GetText("Source and destination arguments are the same file."));
		}
		string text = null;
		if (destinationBackupFileName != null)
		{
			if (destinationBackupFileName.Trim().Length == 0 || destinationBackupFileName.IndexOfAny(Path.InvalidPathChars) != -1)
			{
				throw new ArgumentException("destinationBackupFileName");
			}
			text = Path.GetFullPath(destinationBackupFileName);
			if (MonoIO.ExistsDirectory(text, out error))
			{
				throw new IOException(Locale.GetText("{0} is a directory", destinationBackupFileName));
			}
			if (fullPath == text)
			{
				throw new IOException(Locale.GetText("Source and backup arguments are the same file."));
			}
			if (fullPath2 == text)
			{
				throw new IOException(Locale.GetText("Destination and backup arguments are the same file."));
			}
		}
		if ((GetAttributes(fullPath2) & FileAttributes.ReadOnly) != 0)
		{
			throw MonoIO.GetException(MonoIOError.ERROR_ACCESS_DENIED);
		}
		if (!MonoIO.ReplaceFile(fullPath, fullPath2, text, ignoreMetadataErrors, out error))
		{
			throw MonoIO.GetException(error);
		}
	}

	public static void SetAccessControl(string path, FileSecurity fileSecurity)
	{
		if (fileSecurity == null)
		{
			throw new ArgumentNullException("fileSecurity");
		}
		fileSecurity.PersistModifications(path);
	}

	public static void SetAttributes(string path, FileAttributes fileAttributes)
	{
		Path.Validate(path);
		if (!MonoIO.SetFileAttributes(path, fileAttributes, out var error))
		{
			throw MonoIO.GetException(path, error);
		}
	}

	public static void SetCreationTime(string path, DateTime creationTime)
	{
		Path.Validate(path);
		if (!MonoIO.Exists(path, out var error))
		{
			throw MonoIO.GetException(path, error);
		}
		if (!MonoIO.SetCreationTime(path, creationTime, out error))
		{
			throw MonoIO.GetException(path, error);
		}
	}

	public static void SetCreationTimeUtc(string path, DateTime creationTimeUtc)
	{
		SetCreationTime(path, creationTimeUtc.ToLocalTime());
	}

	public static void SetLastAccessTime(string path, DateTime lastAccessTime)
	{
		Path.Validate(path);
		if (!MonoIO.Exists(path, out var error))
		{
			throw MonoIO.GetException(path, error);
		}
		if (!MonoIO.SetLastAccessTime(path, lastAccessTime, out error))
		{
			throw MonoIO.GetException(path, error);
		}
	}

	public static void SetLastAccessTimeUtc(string path, DateTime lastAccessTimeUtc)
	{
		SetLastAccessTime(path, lastAccessTimeUtc.ToLocalTime());
	}

	public static void SetLastWriteTime(string path, DateTime lastWriteTime)
	{
		Path.Validate(path);
		if (!MonoIO.Exists(path, out var error))
		{
			throw MonoIO.GetException(path, error);
		}
		if (!MonoIO.SetLastWriteTime(path, lastWriteTime, out error))
		{
			throw MonoIO.GetException(path, error);
		}
	}

	public static void SetLastWriteTimeUtc(string path, DateTime lastWriteTimeUtc)
	{
		SetLastWriteTime(path, lastWriteTimeUtc.ToLocalTime());
	}

	public static byte[] ReadAllBytes(string path)
	{
		using FileStream fileStream = OpenRead(path);
		long length = fileStream.Length;
		if (length > int.MaxValue)
		{
			throw new IOException("Reading more than 2GB with this call is not supported");
		}
		int num = 0;
		int num2 = (int)length;
		byte[] array = new byte[length];
		while (num2 > 0)
		{
			int num3 = fileStream.Read(array, num, num2);
			if (num3 == 0)
			{
				throw new IOException("Unexpected end of stream");
			}
			num += num3;
			num2 -= num3;
		}
		return array;
	}

	public static string[] ReadAllLines(string path)
	{
		using StreamReader reader = OpenText(path);
		return ReadAllLines(reader);
	}

	public static string[] ReadAllLines(string path, Encoding encoding)
	{
		using StreamReader reader = new StreamReader(path, encoding);
		return ReadAllLines(reader);
	}

	private static string[] ReadAllLines(StreamReader reader)
	{
		List<string> list = new List<string>();
		while (!reader.EndOfStream)
		{
			list.Add(reader.ReadLine());
		}
		return list.ToArray();
	}

	public static string ReadAllText(string path)
	{
		using StreamReader streamReader = new StreamReader(path);
		return streamReader.ReadToEnd();
	}

	public static string ReadAllText(string path, Encoding encoding)
	{
		using StreamReader streamReader = new StreamReader(path, encoding);
		return streamReader.ReadToEnd();
	}

	public static void WriteAllBytes(string path, byte[] bytes)
	{
		using Stream stream = Create(path);
		stream.Write(bytes, 0, bytes.Length);
	}

	public static void WriteAllLines(string path, string[] contents)
	{
		using StreamWriter writer = new StreamWriter(path);
		WriteAllLines(writer, contents);
	}

	public static void WriteAllLines(string path, string[] contents, Encoding encoding)
	{
		using StreamWriter writer = new StreamWriter(path, append: false, encoding);
		WriteAllLines(writer, contents);
	}

	private static void WriteAllLines(StreamWriter writer, string[] contents)
	{
		foreach (string value in contents)
		{
			writer.WriteLine(value);
		}
	}

	public static void WriteAllText(string path, string contents)
	{
		WriteAllText(path, contents, EncodingHelper.UTF8Unmarked);
	}

	public static void WriteAllText(string path, string contents, Encoding encoding)
	{
		using StreamWriter streamWriter = new StreamWriter(path, append: false, encoding);
		streamWriter.Write(contents);
	}

	[MonoLimitation("File encryption isn't supported (even on NTFS).")]
	public static void Encrypt(string path)
	{
		throw new NotSupportedException(Locale.GetText("File encryption isn't supported on any file system."));
	}

	[MonoLimitation("File encryption isn't supported (even on NTFS).")]
	public static void Decrypt(string path)
	{
		throw new NotSupportedException(Locale.GetText("File encryption isn't supported on any file system."));
	}

	public static IEnumerable<string> ReadLines(string path)
	{
		return ReadLines(OpenText(path));
	}

	public static IEnumerable<string> ReadLines(string path, Encoding encoding)
	{
		return ReadLines(new StreamReader(path, encoding));
	}

	private static IEnumerable<string> ReadLines(StreamReader reader)
	{
		using (reader)
		{
			while (true)
			{
				string text;
				string s = (text = reader.ReadLine());
				if (text == null)
				{
					break;
				}
				yield return s;
			}
		}
	}

	public static void AppendAllLines(string path, IEnumerable<string> contents)
	{
		Path.Validate(path);
		if (contents == null)
		{
			return;
		}
		using TextWriter textWriter = new StreamWriter(path, append: true);
		foreach (string content in contents)
		{
			textWriter.WriteLine(content);
		}
	}

	public static void AppendAllLines(string path, IEnumerable<string> contents, Encoding encoding)
	{
		Path.Validate(path);
		if (contents == null)
		{
			return;
		}
		using TextWriter textWriter = new StreamWriter(path, append: true, encoding);
		foreach (string content in contents)
		{
			textWriter.WriteLine(content);
		}
	}

	public static void WriteAllLines(string path, IEnumerable<string> contents)
	{
		Path.Validate(path);
		if (contents == null)
		{
			return;
		}
		using TextWriter textWriter = new StreamWriter(path, append: false);
		foreach (string content in contents)
		{
			textWriter.WriteLine(content);
		}
	}

	public static void WriteAllLines(string path, IEnumerable<string> contents, Encoding encoding)
	{
		Path.Validate(path);
		if (contents == null)
		{
			return;
		}
		using TextWriter textWriter = new StreamWriter(path, append: false, encoding);
		foreach (string content in contents)
		{
			textWriter.WriteLine(content);
		}
	}

	internal static int FillAttributeInfo(string path, ref MonoIOStat data, bool tryagain, bool returnErrorOnNotFound)
	{
		if (tryagain)
		{
			throw new NotImplementedException();
		}
		MonoIO.GetFileStat(path, out data, out var error);
		if (!returnErrorOnNotFound && (error == MonoIOError.ERROR_FILE_NOT_FOUND || error == MonoIOError.ERROR_PATH_NOT_FOUND || error == MonoIOError.ERROR_NOT_READY))
		{
			data = default(MonoIOStat);
			data.fileAttributes = (FileAttributes)(-1);
			return 0;
		}
		return (int)error;
	}
}
