using System.Linq;
using System.Net;
using System.Threading;
using Microsoft.Win32.SafeHandles;

namespace System.IO.Pipes;

internal class UnixNamedPipeClient : UnixNamedPipe, INamedPipeClient, IPipe
{
	private NamedPipeClientStream owner;

	private SafePipeHandle handle;

	private Action opener;

	public override SafePipeHandle Handle => handle;

	public bool IsAsync => false;

	public int NumberOfServerInstances
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	public UnixNamedPipeClient(NamedPipeClientStream owner, SafePipeHandle safePipeHandle)
	{
		this.owner = owner;
		handle = safePipeHandle;
	}

	public UnixNamedPipeClient(NamedPipeClientStream owner, string serverName, string pipeName, PipeAccessRights desiredAccessRights, PipeOptions options, HandleInheritability inheritability)
	{
		UnixNamedPipeClient unixNamedPipeClient = this;
		this.owner = owner;
		if (serverName != "." && !Dns.GetHostEntry(serverName).AddressList.Contains(IPAddress.Loopback))
		{
			throw new NotImplementedException("Unix fifo does not support remote server connection");
		}
		string name = Path.Combine("/var/tmp/", pipeName);
		EnsureTargetFile(name);
		RightsToAccess(desiredAccessRights);
		ValidateOptions(options, owner.TransmissionMode);
		opener = delegate
		{
			FileStream fileStream = new FileStream(name, FileMode.Open, unixNamedPipeClient.RightsToFileAccess(desiredAccessRights), FileShare.ReadWrite);
			owner.Stream = fileStream;
			unixNamedPipeClient.handle = new SafePipeHandle(fileStream.SafeFileHandle.DangerousGetHandle(), ownsHandle: false);
		};
	}

	public void Connect()
	{
		if (owner.IsConnected)
		{
			throw new InvalidOperationException("The named pipe is already connected");
		}
		opener();
	}

	public void Connect(int timeout)
	{
		AutoResetEvent waitHandle = new AutoResetEvent(initialState: false);
		opener.BeginInvoke(delegate(IAsyncResult result)
		{
			opener.EndInvoke(result);
			waitHandle.Set();
		}, null);
		if (!waitHandle.WaitOne(TimeSpan.FromMilliseconds(timeout)))
		{
			throw new TimeoutException();
		}
	}
}
