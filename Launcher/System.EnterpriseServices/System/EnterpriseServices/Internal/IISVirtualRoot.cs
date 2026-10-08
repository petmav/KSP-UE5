using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices.Internal;

[Guid("d8013ef1-730b-45e2-ba24-874b7242c425")]
public class IISVirtualRoot : IComSoapIISVRoot
{
	public IISVirtualRoot()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Create(string RootWeb, string inPhysicalDirectory, string VirtualDirectory, out string Error)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Delete(string RootWeb, string PhysicalDirectory, string VirtualDirectory, out string Error)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
