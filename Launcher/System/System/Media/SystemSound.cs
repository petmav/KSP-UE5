using System.Security.Permissions;
using Unity;

namespace System.Media;

[HostProtection(SecurityAction.LinkDemand, UI = true)]
public class SystemSound
{
	internal SystemSound()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Play()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
