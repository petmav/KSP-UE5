using Unity;

namespace System.Security.Cryptography.Pkcs;

public abstract class RecipientInfo
{
	private RecipientInfoType _type;

	public abstract byte[] EncryptedKey { get; }

	public abstract AlgorithmIdentifier KeyEncryptionAlgorithm { get; }

	public abstract SubjectIdentifier RecipientIdentifier { get; }

	public RecipientInfoType Type => _type;

	public abstract int Version { get; }

	internal RecipientInfo(RecipientInfoType recipInfoType)
	{
		_type = recipInfoType;
	}

	internal RecipientInfo()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
