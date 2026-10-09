using Microsoft.Win32.SafeHandles;

namespace System.IO.Pipes;

internal class Win32NamedPipeClient : Win32NamedPipe, INamedPipeClient, IPipe
{
	private NamedPipeClientStream owner;

	private Func<SafePipeHandle> opener;

	private bool is_async;

	private string name;

	private SafePipeHandle handle;

	public override SafePipeHandle Handle => handle;

	public bool IsAsync => is_async;

	public int NumberOfServerInstances
	{
		get
		{
			byte[] userName = null;
			if (!Win32Marshal.GetNamedPipeHandleState(Handle, out var _, out var curInstances, out var _, out var _, userName, 0))
			{
				throw Win32PipeError.GetException();
			}
			return curInstances;
		}
	}

	public Win32NamedPipeClient(NamedPipeClientStream owner, SafePipeHandle safePipeHandle)
	{
		handle = safePipeHandle;
		this.owner = owner;
	}

	public Win32NamedPipeClient(NamedPipeClientStream owner, string serverName, string pipeName, PipeAccessRights desiredAccessRights, PipeOptions options, HandleInheritability inheritability)
	{
		Win32NamedPipeClient win32NamedPipeClient = this;
		name = $"\\\\{serverName}\\pipe\\{pipeName}";
		SecurityAttributes att = new SecurityAttributes(inheritability, IntPtr.Zero);
		is_async = (options & PipeOptions.Asynchronous) != 0;
		opener = delegate
		{
			IntPtr intPtr = Win32Marshal.CreateFile(win32NamedPipeClient.name, desiredAccessRights, FileShare.None, ref att, 3, 0, IntPtr.Zero);
			if (intPtr == new IntPtr(-1L))
			{
				throw Win32PipeError.GetException();
			}
			return new SafePipeHandle(intPtr, ownsHandle: true);
		};
		this.owner = owner;
	}

	public void Connect()
	{
		if (owner.IsConnected)
		{
			throw new InvalidOperationException("The named pipe is already connected");
		}
		handle = opener();
	}

	public void Connect(int timeout)
	{
		if (owner.IsConnected)
		{
			throw new InvalidOperationException("The named pipe is already connected");
		}
		if (!Win32Marshal.WaitNamedPipe(name, timeout))
		{
			throw Win32PipeError.GetException();
		}
		Connect();
	}
}
