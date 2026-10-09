using System.Runtime.CompilerServices;

namespace System.IO.MemoryMappedFiles;

internal static class MemoryMapImpl
{
	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern IntPtr OpenFileInternal(string path, FileMode mode, string mapName, out long capacity, MemoryMappedFileAccess access, MemoryMappedFileOptions options, out int error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern IntPtr OpenHandleInternal(IntPtr handle, string mapName, out long capacity, MemoryMappedFileAccess access, MemoryMappedFileOptions options, out int error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern void CloseMapping(IntPtr handle);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern void Flush(IntPtr file_handle);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern void ConfigureHandleInheritability(IntPtr handle, HandleInheritability inheritability);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern bool Unmap(IntPtr mmap_handle);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern int MapInternal(IntPtr handle, long offset, ref long size, MemoryMappedFileAccess access, out IntPtr mmap_handle, out IntPtr base_address);

	internal static void Map(IntPtr handle, long offset, ref long size, MemoryMappedFileAccess access, out IntPtr mmap_handle, out IntPtr base_address)
	{
		int num = MapInternal(handle, offset, ref size, access, out mmap_handle, out base_address);
		if (num != 0)
		{
			throw CreateException(num, "<none>");
		}
	}

	private static Exception CreateException(int error, string path)
	{
		return error switch
		{
			1 => new ArgumentException("A positive capacity must be specified for a Memory Mapped File backed by an empty file."), 
			2 => new ArgumentOutOfRangeException("capacity", "The capacity may not be smaller than the file size."), 
			3 => new FileNotFoundException(path), 
			4 => new IOException("The file already exists"), 
			5 => new PathTooLongException(), 
			6 => new IOException("Could not open file"), 
			7 => new ArgumentException("Capacity must be bigger than zero for non-file mappings"), 
			8 => new ArgumentException("Invalid FileMode value."), 
			9 => new IOException("Could not map file"), 
			10 => new UnauthorizedAccessException("Access to the path is denied."), 
			11 => new ArgumentOutOfRangeException("capacity", "The capacity cannot be greater than the size of the system's logical address space."), 
			_ => new IOException("Failed with unknown error code " + error), 
		};
	}

	internal static IntPtr OpenFile(string path, FileMode mode, string mapName, out long capacity, MemoryMappedFileAccess access, MemoryMappedFileOptions options)
	{
		int error = 0;
		IntPtr result = OpenFileInternal(path, mode, mapName, out capacity, access, options, out error);
		if (error != 0)
		{
			throw CreateException(error, path);
		}
		return result;
	}

	internal static IntPtr OpenHandle(IntPtr handle, string mapName, out long capacity, MemoryMappedFileAccess access, MemoryMappedFileOptions options)
	{
		int error = 0;
		IntPtr result = OpenHandleInternal(handle, mapName, out capacity, access, options, out error);
		if (error != 0)
		{
			throw CreateException(error, "<none>");
		}
		return result;
	}
}
