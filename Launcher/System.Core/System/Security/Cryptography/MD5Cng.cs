using System.Security.Permissions;
using Unity;

namespace System.Security.Cryptography;

[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public sealed class MD5Cng : MD5
{
	public MD5Cng()
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
