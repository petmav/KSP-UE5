using System.Security.Permissions;
using Unity;

namespace System.Diagnostics;

[HostProtection(SecurityAction.LinkDemand, Synchronization = true)]
public class ConsoleTraceListener : TextWriterTraceListener
{
	public ConsoleTraceListener()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ConsoleTraceListener(bool useErrorStream)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
