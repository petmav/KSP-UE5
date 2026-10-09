using System.Security.Permissions;
using Unity;

namespace System.Diagnostics;

[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public class UnescapedXmlDiagnosticData
{
	public string UnescapedXml
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public UnescapedXmlDiagnosticData(string xmlPayload)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
