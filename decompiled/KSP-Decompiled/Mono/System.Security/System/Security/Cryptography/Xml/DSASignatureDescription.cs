namespace System.Security.Cryptography.Xml;

internal class DSASignatureDescription : SignatureDescription
{
	private const string HashAlgorithm = "SHA1";

	public DSASignatureDescription()
	{
		base.KeyAlgorithm = typeof(DSA).AssemblyQualifiedName;
		base.FormatterAlgorithm = typeof(DSASignatureFormatter).AssemblyQualifiedName;
		base.DeformatterAlgorithm = typeof(DSASignatureDeformatter).AssemblyQualifiedName;
		base.DigestAlgorithm = "SHA1";
	}

	public sealed override AsymmetricSignatureDeformatter CreateDeformatter(AsymmetricAlgorithm key)
	{
		AsymmetricSignatureDeformatter obj = (AsymmetricSignatureDeformatter)CryptoHelpers.CreateFromName(base.DeformatterAlgorithm);
		obj.SetKey(key);
		obj.SetHashAlgorithm("SHA1");
		return obj;
	}

	public sealed override AsymmetricSignatureFormatter CreateFormatter(AsymmetricAlgorithm key)
	{
		AsymmetricSignatureFormatter obj = (AsymmetricSignatureFormatter)CryptoHelpers.CreateFromName(base.FormatterAlgorithm);
		obj.SetKey(key);
		obj.SetHashAlgorithm("SHA1");
		return obj;
	}

	public sealed override HashAlgorithm CreateDigest()
	{
		return SHA1.Create();
	}
}
