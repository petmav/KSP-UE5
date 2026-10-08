using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security;
using Microsoft.Win32;

namespace System.IO;

[Serializable]
[ComVisible(true)]
public abstract class FileSystemInfo : MarshalByRefObject, ISerializable
{
	internal MonoIOStat _data;

	internal int _dataInitialised = -1;

	private const int ERROR_INVALID_PARAMETER = 87;

	internal const int ERROR_ACCESS_DENIED = 5;

	protected string FullPath;

	protected string OriginalPath;

	private string _displayPath = "";

	public virtual string FullName
	{
		[SecuritySafeCritical]
		get
		{
			return FullPath;
		}
	}

	internal virtual string UnsafeGetFullName
	{
		[SecurityCritical]
		get
		{
			return FullPath;
		}
	}

	public string Extension
	{
		get
		{
			int length = FullPath.Length;
			int num = length;
			while (--num >= 0)
			{
				char c = FullPath[num];
				if (c == '.')
				{
					return FullPath.Substring(num, length - num);
				}
				if (c == Path.DirectorySeparatorChar || c == Path.AltDirectorySeparatorChar || c == Path.VolumeSeparatorChar)
				{
					break;
				}
			}
			return string.Empty;
		}
	}

	public abstract string Name { get; }

	public abstract bool Exists { get; }

	public DateTime CreationTime
	{
		get
		{
			return CreationTimeUtc.ToLocalTime();
		}
		set
		{
			CreationTimeUtc = value.ToUniversalTime();
		}
	}

	[ComVisible(false)]
	public DateTime CreationTimeUtc
	{
		[SecuritySafeCritical]
		get
		{
			if (_dataInitialised == -1)
			{
				Refresh();
			}
			if (_dataInitialised != 0)
			{
				__Error.WinIOError(_dataInitialised, DisplayPath);
			}
			return DateTime.FromFileTimeUtc(_data.CreationTime);
		}
		set
		{
			if (this is DirectoryInfo)
			{
				Directory.SetCreationTimeUtc(FullPath, value);
			}
			else
			{
				File.SetCreationTimeUtc(FullPath, value);
			}
			_dataInitialised = -1;
		}
	}

	public DateTime LastAccessTime
	{
		get
		{
			return LastAccessTimeUtc.ToLocalTime();
		}
		set
		{
			LastAccessTimeUtc = value.ToUniversalTime();
		}
	}

	[ComVisible(false)]
	public DateTime LastAccessTimeUtc
	{
		[SecuritySafeCritical]
		get
		{
			if (_dataInitialised == -1)
			{
				Refresh();
			}
			if (_dataInitialised != 0)
			{
				__Error.WinIOError(_dataInitialised, DisplayPath);
			}
			return DateTime.FromFileTimeUtc(_data.LastAccessTime);
		}
		set
		{
			if (this is DirectoryInfo)
			{
				Directory.SetLastAccessTimeUtc(FullPath, value);
			}
			else
			{
				File.SetLastAccessTimeUtc(FullPath, value);
			}
			_dataInitialised = -1;
		}
	}

	public DateTime LastWriteTime
	{
		get
		{
			return LastWriteTimeUtc.ToLocalTime();
		}
		set
		{
			LastWriteTimeUtc = value.ToUniversalTime();
		}
	}

	[ComVisible(false)]
	public DateTime LastWriteTimeUtc
	{
		[SecuritySafeCritical]
		get
		{
			if (_dataInitialised == -1)
			{
				Refresh();
			}
			if (_dataInitialised != 0)
			{
				__Error.WinIOError(_dataInitialised, DisplayPath);
			}
			return DateTime.FromFileTimeUtc(_data.LastWriteTime);
		}
		set
		{
			if (this is DirectoryInfo)
			{
				Directory.SetLastWriteTimeUtc(FullPath, value);
			}
			else
			{
				File.SetLastWriteTimeUtc(FullPath, value);
			}
			_dataInitialised = -1;
		}
	}

	public FileAttributes Attributes
	{
		[SecuritySafeCritical]
		get
		{
			if (_dataInitialised == -1)
			{
				Refresh();
			}
			if (_dataInitialised != 0)
			{
				__Error.WinIOError(_dataInitialised, DisplayPath);
			}
			return _data.fileAttributes;
		}
		[SecuritySafeCritical]
		set
		{
			if (!MonoIO.SetFileAttributes(FullPath, value, out var error))
			{
				MonoIOError num = error;
				switch (num)
				{
				case MonoIOError.ERROR_INVALID_PARAMETER:
					throw new ArgumentException(Environment.GetResourceString("Invalid File or Directory attributes value."));
				case MonoIOError.ERROR_ACCESS_DENIED:
					throw new ArgumentException(Environment.GetResourceString("Access to the path is denied."));
				}
				__Error.WinIOError((int)num, DisplayPath);
			}
			_dataInitialised = -1;
		}
	}

	internal string DisplayPath
	{
		get
		{
			return _displayPath;
		}
		set
		{
			_displayPath = value;
		}
	}

	protected FileSystemInfo()
	{
	}

	protected FileSystemInfo(SerializationInfo info, StreamingContext context)
	{
		if (info == null)
		{
			throw new ArgumentNullException("info");
		}
		FullPath = Path.GetFullPathInternal(info.GetString("FullPath"));
		OriginalPath = info.GetString("OriginalPath");
		_dataInitialised = -1;
	}

	[SecurityCritical]
	internal void InitializeFrom(Win32Native.WIN32_FIND_DATA findData)
	{
		throw new NotImplementedException();
	}

	public abstract void Delete();

	[SecuritySafeCritical]
	public void Refresh()
	{
		_dataInitialised = File.FillAttributeInfo(FullPath, ref _data, tryagain: false, returnErrorOnNotFound: false);
	}

	[SecurityCritical]
	[ComVisible(false)]
	public virtual void GetObjectData(SerializationInfo info, StreamingContext context)
	{
		info.AddValue("OriginalPath", OriginalPath, typeof(string));
		info.AddValue("FullPath", FullPath, typeof(string));
	}
}
