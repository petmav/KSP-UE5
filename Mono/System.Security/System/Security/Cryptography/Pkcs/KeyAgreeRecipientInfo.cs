namespace System.Security.Cryptography.Pkcs;

[System.MonoTODO]
public sealed class KeyAgreeRecipientInfo : RecipientInfo
{
	public DateTime Date => DateTime.MinValue;

	public override byte[] EncryptedKey => null;

	public override AlgorithmIdentifier KeyEncryptionAlgorithm => null;

	public SubjectIdentifierOrKey OriginatorIdentifierOrKey => null;

	public CryptographicAttributeObject OtherKeyAttribute => null;

	public override SubjectIdentifier RecipientIdentifier => null;

	public override int Version => 0;

	internal KeyAgreeRecipientInfo()
		: base(RecipientInfoType.KeyAgreement)
	{
	}
}
