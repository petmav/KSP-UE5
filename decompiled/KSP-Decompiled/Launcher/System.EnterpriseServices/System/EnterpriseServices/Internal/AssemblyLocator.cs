using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices.Internal;

[Guid("458aa3b5-265a-4b75-bc05-9bea4630cf18")]
public class AssemblyLocator : MarshalByRefObject
{
	public AssemblyLocator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
