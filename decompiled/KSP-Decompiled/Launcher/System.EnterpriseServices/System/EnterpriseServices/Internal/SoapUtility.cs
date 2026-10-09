using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices.Internal;

[Guid("5F9A955F-AA55-4127-A32B-33496AA8A44E")]
public sealed class SoapUtility : ISoapUtility
{
	public SoapUtility()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void GetServerBinPath(string rootWebServer, string inBaseUrl, string inVirtualRoot, out string binPath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void GetServerPhysicalPath(string rootWebServer, string inBaseUrl, string inVirtualRoot, out string physicalPath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Present()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
