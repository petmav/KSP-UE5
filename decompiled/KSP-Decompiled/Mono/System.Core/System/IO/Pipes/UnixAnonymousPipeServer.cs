using Microsoft.Win32.SafeHandles;

namespace System.IO.Pipes;

internal class UnixAnonymousPipeServer : UnixAnonymousPipe, IAnonymousPipeServer, IPipe
{
	private SafePipeHandle server_handle;

	private SafePipeHandle client_handle;

	public override SafePipeHandle Handle => server_handle;

	public SafePipeHandle ClientHandle => client_handle;

	public UnixAnonymousPipeServer(AnonymousPipeServerStream owner, PipeDirection direction, HandleInheritability inheritability, int bufferSize)
	{
		throw new NotImplementedException();
	}

	public UnixAnonymousPipeServer(AnonymousPipeServerStream owner, SafePipeHandle serverHandle, SafePipeHandle clientHandle)
	{
		server_handle = serverHandle;
		client_handle = clientHandle;
		throw new NotImplementedException();
	}

	public void DisposeLocalCopyOfClientHandle()
	{
		throw new NotImplementedException();
	}
}
