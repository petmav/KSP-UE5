using System;
using System.Net;
using Unity;

namespace Microsoft.Win32;

public class IntranetZoneCredentialPolicy : ICredentialPolicy
{
	public IntranetZoneCredentialPolicy()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual bool ShouldSendCredential(Uri challengeUri, WebRequest request, NetworkCredential credential, IAuthenticationModule authModule)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}
