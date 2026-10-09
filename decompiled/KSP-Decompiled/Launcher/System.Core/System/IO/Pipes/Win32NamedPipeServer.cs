using Microsoft.Win32.SafeHandles;

namespace System.IO.Pipes;

internal class Win32NamedPipeServer : Win32NamedPipe, INamedPipeServer, IPipe
{
	private SafePipeHandle handle;

	public override SafePipeHandle Handle => handle;

	public Win32NamedPipeServer(NamedPipeServerStream owner, SafePipeHandle safePipeHandle)
	{
		handle = safePipeHandle;
	}

	public unsafe Win32NamedPipeServer(NamedPipeServerStream owner, string pipeName, int maxNumberOfServerInstances, PipeTransmissionMode transmissionMode, PipeAccessRights rights, PipeOptions options, int inBufferSize, int outBufferSize, PipeSecurity pipeSecurity, HandleInheritability inheritability)
	{
		string name = $"\\\\.\\pipe\\{pipeName}";
		uint openMode = (uint)rights | (uint)options;
		int num = 0;
		if ((owner.TransmissionMode & PipeTransmissionMode.Message) != 0)
		{
			num |= 4;
		}
		if ((options & PipeOptions.Asynchronous) != 0)
		{
			num |= 1;
		}
		byte[] array = null;
		if (pipeSecurity != null)
		{
			array = pipeSecurity.GetSecurityDescriptorBinaryForm();
		}
		fixed (byte* ptr = array)
		{
			SecurityAttributes securityAttributes = new SecurityAttributes(inheritability, (IntPtr)ptr);
			IntPtr intPtr = Win32Marshal.CreateNamedPipe(name, openMode, num, maxNumberOfServerInstances, outBufferSize, inBufferSize, 0, ref securityAttributes, IntPtr.Zero);
			if (intPtr == new IntPtr(-1L))
			{
				throw Win32PipeError.GetException();
			}
			handle = new SafePipeHandle(intPtr, ownsHandle: true);
		}
	}

	public void Disconnect()
	{
		Win32Marshal.DisconnectNamedPipe(Handle);
	}

	public void WaitForConnection()
	{
		if (!Win32Marshal.ConnectNamedPipe(Handle, IntPtr.Zero))
		{
			throw Win32PipeError.GetException();
		}
	}
}
