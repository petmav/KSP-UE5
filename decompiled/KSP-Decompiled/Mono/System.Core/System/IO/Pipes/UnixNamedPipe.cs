using Microsoft.Win32.SafeHandles;
using Mono.Unix.Native;

namespace System.IO.Pipes;

internal abstract class UnixNamedPipe : IPipe
{
	public abstract SafePipeHandle Handle { get; }

	public void WaitForPipeDrain()
	{
		throw new NotImplementedException();
	}

	public void EnsureTargetFile(string name)
	{
		if (!File.Exists(name))
		{
			int num = Syscall.mknod(name, FilePermissions.ALLPERMS | FilePermissions.S_IFIFO, 0uL);
			if (num != 0)
			{
				throw new IOException($"Error on creating named pipe: error code {num}");
			}
		}
	}

	protected void ValidateOptions(PipeOptions options, PipeTransmissionMode mode)
	{
		if (((uint)options & 0x80000000u) != 0)
		{
			throw new NotImplementedException("WriteThrough is not supported");
		}
		if ((mode & PipeTransmissionMode.Message) != 0)
		{
			throw new NotImplementedException("Message transmission mode is not supported");
		}
		if ((options & PipeOptions.Asynchronous) != 0)
		{
			throw new NotImplementedException("Asynchronous pipe mode is not supported");
		}
	}

	protected string RightsToAccess(PipeAccessRights rights)
	{
		string text = null;
		if ((rights & PipeAccessRights.ReadData) != 0)
		{
			if ((rights & PipeAccessRights.WriteData) != 0)
			{
				return "r+";
			}
			return "r";
		}
		if ((rights & PipeAccessRights.WriteData) != 0)
		{
			return "w";
		}
		throw new InvalidOperationException("The pipe must be opened to either read or write");
	}

	protected FileAccess RightsToFileAccess(PipeAccessRights rights)
	{
		if ((rights & PipeAccessRights.ReadData) != 0)
		{
			if ((rights & PipeAccessRights.WriteData) != 0)
			{
				return FileAccess.ReadWrite;
			}
			return FileAccess.Read;
		}
		if ((rights & PipeAccessRights.WriteData) != 0)
		{
			return FileAccess.Write;
		}
		throw new InvalidOperationException("The pipe must be opened to either read or write");
	}
}
