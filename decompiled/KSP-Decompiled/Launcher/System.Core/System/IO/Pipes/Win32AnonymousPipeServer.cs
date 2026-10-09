using Microsoft.Win32.SafeHandles;

namespace System.IO.Pipes;

internal class Win32AnonymousPipeServer : Win32AnonymousPipe, IAnonymousPipeServer, IPipe
{
	private SafePipeHandle server_handle;

	private SafePipeHandle client_handle;

	public override SafePipeHandle Handle => server_handle;

	public SafePipeHandle ClientHandle => client_handle;

	public unsafe Win32AnonymousPipeServer(AnonymousPipeServerStream owner, PipeDirection direction, HandleInheritability inheritability, int bufferSize, PipeSecurity pipeSecurity)
	{
		byte[] array = null;
		if (pipeSecurity != null)
		{
			array = pipeSecurity.GetSecurityDescriptorBinaryForm();
		}
		IntPtr readHandle;
		IntPtr writeHandle;
		fixed (byte* ptr = array)
		{
			SecurityAttributes pipeAtts = new SecurityAttributes(inheritability, (IntPtr)ptr);
			if (!Win32Marshal.CreatePipe(out readHandle, out writeHandle, ref pipeAtts, bufferSize))
			{
				throw Win32PipeError.GetException();
			}
		}
		SafePipeHandle safePipeHandle = new SafePipeHandle(readHandle, ownsHandle: true);
		SafePipeHandle safePipeHandle2 = new SafePipeHandle(writeHandle, ownsHandle: true);
		if (direction == PipeDirection.Out)
		{
			server_handle = safePipeHandle2;
			client_handle = safePipeHandle;
		}
		else
		{
			server_handle = safePipeHandle;
			client_handle = safePipeHandle2;
		}
	}

	public Win32AnonymousPipeServer(AnonymousPipeServerStream owner, SafePipeHandle serverHandle, SafePipeHandle clientHandle)
	{
		server_handle = serverHandle;
		client_handle = clientHandle;
	}

	public void DisposeLocalCopyOfClientHandle()
	{
		throw new NotImplementedException();
	}
}
