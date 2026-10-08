using System.Globalization;
using System.Security.Permissions;
using Microsoft.Win32.SafeHandles;

namespace System.IO.Pipes;

[MonoTODO("Anonymous pipes are not working even on win32, due to some access authorization issue")]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public sealed class AnonymousPipeServerStream : PipeStream
{
	private IAnonymousPipeServer impl;

	[MonoTODO]
	public SafePipeHandle ClientSafePipeHandle { get; private set; }

	public override PipeTransmissionMode ReadMode
	{
		set
		{
			if (value == PipeTransmissionMode.Message)
			{
				throw new NotSupportedException();
			}
		}
	}

	public override PipeTransmissionMode TransmissionMode => PipeTransmissionMode.Byte;

	public AnonymousPipeServerStream()
		: this(PipeDirection.Out)
	{
	}

	public AnonymousPipeServerStream(PipeDirection direction)
		: this(direction, HandleInheritability.None)
	{
	}

	public AnonymousPipeServerStream(PipeDirection direction, HandleInheritability inheritability)
		: this(direction, inheritability, 1024)
	{
	}

	public AnonymousPipeServerStream(PipeDirection direction, HandleInheritability inheritability, int bufferSize)
		: base(direction, bufferSize)
	{
		throw new NotImplementedException();
	}

	public AnonymousPipeServerStream(PipeDirection direction, HandleInheritability inheritability, int bufferSize, PipeSecurity pipeSecurity)
		: base(direction, bufferSize)
	{
		if (direction == PipeDirection.InOut)
		{
			throw new NotSupportedException("Anonymous pipe direction can only be either in or out.");
		}
		throw new NotImplementedException();
	}

	[MonoTODO]
	public AnonymousPipeServerStream(PipeDirection direction, SafePipeHandle serverSafePipeHandle, SafePipeHandle clientSafePipeHandle)
		: base(direction, 1024)
	{
		if (serverSafePipeHandle == null)
		{
			throw new ArgumentNullException("serverSafePipeHandle");
		}
		if (clientSafePipeHandle == null)
		{
			throw new ArgumentNullException("clientSafePipeHandle");
		}
		if (direction == PipeDirection.InOut)
		{
			throw new NotSupportedException("Anonymous pipe direction can only be either in or out.");
		}
		throw new NotImplementedException();
	}

	~AnonymousPipeServerStream()
	{
	}

	[MonoTODO]
	public void DisposeLocalCopyOfClientHandle()
	{
		impl.DisposeLocalCopyOfClientHandle();
	}

	public string GetClientHandleAsString()
	{
		return impl.Handle.DangerousGetHandle().ToInt64().ToString(NumberFormatInfo.InvariantInfo);
	}
}
