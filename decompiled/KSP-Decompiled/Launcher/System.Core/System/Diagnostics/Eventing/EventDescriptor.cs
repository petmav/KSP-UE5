using System.Runtime.InteropServices;
using System.Security.Permissions;
using Unity;

namespace System.Diagnostics.Eventing;

[StructLayout(LayoutKind.Explicit, Size = 16)]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public struct EventDescriptor
{
	public byte Channel
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(byte);
		}
	}

	public int EventId
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public long Keywords
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(long);
		}
	}

	public byte Level
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(byte);
		}
	}

	public byte Opcode
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(byte);
		}
	}

	public int Task
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public byte Version
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(byte);
		}
	}

	public EventDescriptor(int id, byte version, byte channel, byte level, byte opcode, int task, long keywords)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
