using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices.Internal;

[Guid("3b0398c9-7812-4007-85cb-18c771f2206f")]
public class ComManagedImportUtil : IComManagedImportUtil
{
	public ComManagedImportUtil()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void GetComponentInfo(string assemblyPath, out string numComponents, out string componentInfo)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void InstallAssembly(string asmpath, string parname, string appname)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
