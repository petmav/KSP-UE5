using System.Security.Permissions;
using Unity;

namespace System.Security.Cryptography;

[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public sealed class SHA1Cng : SHA1
{
	public SHA1Cng()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override void HashCore(byte[] array, int ibStart, int cbSize)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override byte[] HashFinal()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override void Initialize()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
