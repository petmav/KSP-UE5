using Unity;

namespace System.Security.Cryptography.Pkcs;

public sealed class PublicKeyInfo
{
	private AlgorithmIdentifier _algorithm;

	private byte[] _key;

	public AlgorithmIdentifier Algorithm => _algorithm;

	public byte[] KeyValue => _key;

	internal PublicKeyInfo(AlgorithmIdentifier algorithm, byte[] key)
	{
		_algorithm = algorithm;
		_key = key;
	}

	internal PublicKeyInfo()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
