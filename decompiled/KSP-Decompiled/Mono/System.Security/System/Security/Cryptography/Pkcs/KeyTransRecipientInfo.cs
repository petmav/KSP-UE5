using Unity;

namespace System.Security.Cryptography.Pkcs;

public sealed class KeyTransRecipientInfo : RecipientInfo
{
	private byte[] _encryptedKey;

	private AlgorithmIdentifier _keyEncryptionAlgorithm;

	private SubjectIdentifier _recipientIdentifier;

	private int _version;

	public override byte[] EncryptedKey => _encryptedKey;

	public override AlgorithmIdentifier KeyEncryptionAlgorithm => _keyEncryptionAlgorithm;

	public override SubjectIdentifier RecipientIdentifier => _recipientIdentifier;

	public override int Version => _version;

	internal KeyTransRecipientInfo(byte[] encryptedKey, AlgorithmIdentifier keyEncryptionAlgorithm, SubjectIdentifier recipientIdentifier, int version)
		: base(RecipientInfoType.KeyTransport)
	{
		_encryptedKey = encryptedKey;
		_keyEncryptionAlgorithm = keyEncryptionAlgorithm;
		_recipientIdentifier = recipientIdentifier;
		_version = version;
	}

	internal KeyTransRecipientInfo()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
