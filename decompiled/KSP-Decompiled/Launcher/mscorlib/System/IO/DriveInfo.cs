using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System.IO;

[Serializable]
[ComVisible(true)]
public sealed class DriveInfo : ISerializable
{
	private string drive_format;

	private string path;

	public long AvailableFreeSpace
	{
		get
		{
			GetDiskFreeSpace(path, out var availableFreeSpace, out var _, out var _);
			if (availableFreeSpace <= long.MaxValue)
			{
				return (long)availableFreeSpace;
			}
			return long.MaxValue;
		}
	}

	public long TotalFreeSpace
	{
		get
		{
			GetDiskFreeSpace(path, out var _, out var _, out var totalFreeSpace);
			if (totalFreeSpace <= long.MaxValue)
			{
				return (long)totalFreeSpace;
			}
			return long.MaxValue;
		}
	}

	public long TotalSize
	{
		get
		{
			GetDiskFreeSpace(path, out var _, out var totalSize, out var _);
			if (totalSize <= long.MaxValue)
			{
				return (long)totalSize;
			}
			return long.MaxValue;
		}
	}

	[MonoTODO("Currently get only works on Mono/Unix; set not implemented")]
	public string VolumeLabel
	{
		get
		{
			return path;
		}
		set
		{
			throw new NotImplementedException();
		}
	}

	public string DriveFormat => drive_format;

	public DriveType DriveType => (DriveType)GetDriveTypeInternal(path);

	public string Name => path;

	public DirectoryInfo RootDirectory => new DirectoryInfo(path);

	public bool IsReady => Directory.Exists(Name);

	private DriveInfo(string path, string fstype)
	{
		drive_format = fstype;
		this.path = path;
	}

	public DriveInfo(string driveName)
	{
		if (!Environment.IsUnix)
		{
			if (driveName == null || driveName.Length == 0)
			{
				throw new ArgumentException("The drive name is null or empty", "driveName");
			}
			if (driveName.Length >= 2 && driveName[1] != ':')
			{
				throw new ArgumentException("Invalid drive name", "driveName");
			}
			driveName = char.ToUpperInvariant(driveName[0]) + ":\\";
		}
		DriveInfo[] drives = GetDrives();
		foreach (DriveInfo driveInfo in drives)
		{
			if (driveInfo.path == driveName)
			{
				path = driveInfo.path;
				drive_format = driveInfo.drive_format;
				path = driveInfo.path;
				return;
			}
		}
		throw new ArgumentException("The drive name does not exist", "driveName");
	}

	private static void GetDiskFreeSpace(string path, out ulong availableFreeSpace, out ulong totalSize, out ulong totalFreeSpace)
	{
		if (!GetDiskFreeSpaceInternal(path, out availableFreeSpace, out totalSize, out totalFreeSpace, out var error))
		{
			throw MonoIO.GetException(path, error);
		}
	}

	[MonoTODO("In windows, alldrives are 'Fixed'")]
	public static DriveInfo[] GetDrives()
	{
		string[] logicalDrives = Environment.GetLogicalDrives();
		DriveInfo[] array = new DriveInfo[logicalDrives.Length];
		int num = 0;
		string[] array2 = logicalDrives;
		foreach (string rootPathName in array2)
		{
			array[num++] = new DriveInfo(rootPathName, GetDriveFormat(rootPathName));
		}
		return array;
	}

	void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
	{
		throw new NotImplementedException();
	}

	public override string ToString()
	{
		return Name;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern bool GetDiskFreeSpaceInternal(string pathName, out ulong freeBytesAvail, out ulong totalNumberOfBytes, out ulong totalNumberOfFreeBytes, out MonoIOError error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern uint GetDriveTypeInternal(string rootPathName);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern string GetDriveFormat(string rootPathName);
}
