using System.Security.Permissions;
using Unity;

namespace System.Security.Cryptography;

public sealed class DSACng : DSA
{
	public CngKey Key
	{
		[SecuritySafeCritical]
		[SecurityPermission(SecurityAction.Assert, UnmanagedCode = true)]
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public DSACng()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public DSACng(int keySize)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	[SecurityPermission(SecurityAction.Assert, UnmanagedCode = true)]
	public DSACng(CngKey key)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	public override byte[] CreateSignature(byte[] rgbHash)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override DSAParameters ExportParameters(bool includePrivateParameters)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(DSAParameters);
	}

	public override void ImportParameters(DSAParameters parameters)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	public override bool VerifySignature(byte[] rgbHash, byte[] rgbSignature)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}
