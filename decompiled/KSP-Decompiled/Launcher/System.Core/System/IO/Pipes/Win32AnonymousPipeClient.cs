using Microsoft.Win32.SafeHandles;

namespace System.IO.Pipes;

internal class Win32AnonymousPipeClient : Win32AnonymousPipe, IAnonymousPipeClient, IPipe
{
	private SafePipeHandle handle;

	public override SafePipeHandle Handle => handle;

	public Win32AnonymousPipeClient(AnonymousPipeClientStream owner, SafePipeHandle handle)
	{
		this.handle = handle;
	}
}
