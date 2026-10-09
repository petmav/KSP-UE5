namespace System.Security.Cryptography.Xml;

internal abstract class RSAPKCS1SignatureDescription : SignatureDescription
{
	public RSAPKCS1SignatureDescription(string hashAlgorithmName)
	{
		base.KeyAlgorithm = typeof(RSA).AssemblyQualifiedName;
		base.FormatterAlgorithm = typeof(RSAPKCS1SignatureFormatter).AssemblyQualifiedName;
		base.DeformatterAlgorithm = typeof(RSAPKCS1SignatureDeformatter).AssemblyQualifiedName;
		base.DigestAlgorithm = hashAlgorithmName;
	}

	public sealed override AsymmetricSignatureDeformatter CreateDeformatter(AsymmetricAlgorithm key)
	{
		AsymmetricSignatureDeformatter obj = (AsymmetricSignatureDeformatter)CryptoHelpers.CreateFromName(base.DeformatterAlgorithm);
		obj.SetKey(key);
		obj.SetHashAlgorithm(base.DigestAlgorithm);
		return obj;
	}

	public sealed override AsymmetricSignatureFormatter CreateFormatter(AsymmetricAlgorithm key)
	{
		AsymmetricSignatureFormatter obj = (AsymmetricSignatureFormatter)CryptoHelpers.CreateFromName(base.FormatterAlgorithm);
		obj.SetKey(key);
		obj.SetHashAlgorithm(base.DigestAlgorithm);
		return obj;
	}

	public abstract override HashAlgorithm CreateDigest();
}
