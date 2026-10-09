using System.Security.Permissions;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Win32.SafeHandles;

namespace System.IO.Pipes;

[MonoTODO("working only on win32 right now")]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public sealed class NamedPipeServerStream : PipeStream
{
	public const int MaxAllowedServerInstances = -1;

	private INamedPipeServer impl;

	private Action wait_connect_delegate;

	public NamedPipeServerStream(string pipeName)
		: this(pipeName, PipeDirection.InOut)
	{
	}

	public NamedPipeServerStream(string pipeName, PipeDirection direction)
		: this(pipeName, direction, 1)
	{
	}

	public NamedPipeServerStream(string pipeName, PipeDirection direction, int maxNumberOfServerInstances)
		: this(pipeName, direction, maxNumberOfServerInstances, PipeTransmissionMode.Byte)
	{
	}

	public NamedPipeServerStream(string pipeName, PipeDirection direction, int maxNumberOfServerInstances, PipeTransmissionMode transmissionMode)
		: this(pipeName, direction, maxNumberOfServerInstances, transmissionMode, PipeOptions.None)
	{
	}

	public NamedPipeServerStream(string pipeName, PipeDirection direction, int maxNumberOfServerInstances, PipeTransmissionMode transmissionMode, PipeOptions options)
		: this(pipeName, direction, maxNumberOfServerInstances, transmissionMode, options, 1024, 1024)
	{
	}

	public NamedPipeServerStream(string pipeName, PipeDirection direction, int maxNumberOfServerInstances, PipeTransmissionMode transmissionMode, PipeOptions options, int inBufferSize, int outBufferSize)
		: base(direction, inBufferSize)
	{
		throw new NotImplementedException();
	}

	public NamedPipeServerStream(string pipeName, PipeDirection direction, int maxNumberOfServerInstances, PipeTransmissionMode transmissionMode, PipeOptions options, int inBufferSize, int outBufferSize, PipeSecurity pipeSecurity)
		: this(pipeName, direction, maxNumberOfServerInstances, transmissionMode, options, inBufferSize, outBufferSize, pipeSecurity, HandleInheritability.None)
	{
	}

	public NamedPipeServerStream(string pipeName, PipeDirection direction, int maxNumberOfServerInstances, PipeTransmissionMode transmissionMode, PipeOptions options, int inBufferSize, int outBufferSize, PipeSecurity pipeSecurity, HandleInheritability inheritability)
		: this(pipeName, direction, maxNumberOfServerInstances, transmissionMode, options, inBufferSize, outBufferSize, pipeSecurity, inheritability, PipeAccessRights.ReadData | PipeAccessRights.WriteData)
	{
	}

	[MonoTODO]
	public NamedPipeServerStream(string pipeName, PipeDirection direction, int maxNumberOfServerInstances, PipeTransmissionMode transmissionMode, PipeOptions options, int inBufferSize, int outBufferSize, PipeSecurity pipeSecurity, HandleInheritability inheritability, PipeAccessRights additionalAccessRights)
		: base(direction, transmissionMode, outBufferSize)
	{
		throw new NotImplementedException();
	}

	public NamedPipeServerStream(PipeDirection direction, bool isAsync, bool isConnected, SafePipeHandle safePipeHandle)
		: base(direction, 1024)
	{
		throw new NotImplementedException();
	}

	~NamedPipeServerStream()
	{
	}

	public void Disconnect()
	{
		impl.Disconnect();
	}

	[MonoTODO]
	[SecurityPermission(SecurityAction.Demand, Flags = SecurityPermissionFlag.ControlPrincipal)]
	public void RunAsClient(PipeStreamImpersonationWorker impersonationWorker)
	{
		throw new NotImplementedException();
	}

	public void WaitForConnection()
	{
		impl.WaitForConnection();
		base.IsConnected = true;
	}

	public Task WaitForConnectionAsync()
	{
		return WaitForConnectionAsync(CancellationToken.None);
	}

	[MonoTODO]
	public Task WaitForConnectionAsync(CancellationToken cancellationToken)
	{
		throw new NotImplementedException();
	}

	[MonoTODO]
	[SecurityPermission(SecurityAction.Demand, Flags = SecurityPermissionFlag.ControlPrincipal)]
	public string GetImpersonationUserName()
	{
		throw new NotImplementedException();
	}

	[HostProtection(SecurityAction.LinkDemand, ExternalThreading = true)]
	public IAsyncResult BeginWaitForConnection(AsyncCallback callback, object state)
	{
		if (wait_connect_delegate == null)
		{
			wait_connect_delegate = WaitForConnection;
		}
		return wait_connect_delegate.BeginInvoke(callback, state);
	}

	public void EndWaitForConnection(IAsyncResult asyncResult)
	{
		wait_connect_delegate.EndInvoke(asyncResult);
	}
}
