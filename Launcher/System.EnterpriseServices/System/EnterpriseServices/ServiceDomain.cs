using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices;

[ComVisible(false)]
public sealed class ServiceDomain
{
	internal ServiceDomain()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void Enter(ServiceConfig cfg)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static TransactionStatus Leave()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(TransactionStatus);
	}
}
