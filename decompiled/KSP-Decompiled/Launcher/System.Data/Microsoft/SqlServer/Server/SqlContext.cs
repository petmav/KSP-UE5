using System.Security.Principal;
using Unity;

namespace Microsoft.SqlServer.Server;

public sealed class SqlContext
{
	public static bool IsAvailable
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public static SqlPipe Pipe
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public static SqlTriggerContext TriggerContext
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public static WindowsIdentity WindowsIdentity
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal SqlContext()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
