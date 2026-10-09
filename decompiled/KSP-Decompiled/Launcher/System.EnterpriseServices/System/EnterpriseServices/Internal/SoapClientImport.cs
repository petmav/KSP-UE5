using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices.Internal;

[Guid("346D5B9F-45E1-45c0-AADF-1B7D221E9063")]
public sealed class SoapClientImport : ISoapClientImport
{
	public SoapClientImport()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ProcessClientTlbEx(string progId, string virtualRoot, string baseUrl, string authentication, string assemblyName, string typeName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
