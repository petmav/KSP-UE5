using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices.Internal;

[Guid("ecabafd1-7f19-11d2-978e-0000f8757e2a")]
public class ClrObjectFactory : IClrObjectFactory
{
	public ClrObjectFactory()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public object CreateFromAssembly(string AssemblyName, string TypeName, string Mode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public object CreateFromMailbox(string Mailbox, string Mode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public object CreateFromVroot(string VrootUrl, string Mode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public object CreateFromWsdl(string WsdlUrl, string Mode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
