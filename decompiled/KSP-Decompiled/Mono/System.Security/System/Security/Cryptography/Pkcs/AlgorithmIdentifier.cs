namespace System.Security.Cryptography.Pkcs;

public sealed class AlgorithmIdentifier
{
	private Oid _oid;

	private int _length;

	private byte[] _params;

	public int KeyLength
	{
		get
		{
			return _length;
		}
		set
		{
			_length = value;
		}
	}

	public Oid Oid
	{
		get
		{
			return _oid;
		}
		set
		{
			_oid = value;
		}
	}

	public byte[] Parameters
	{
		get
		{
			return _params;
		}
		set
		{
			_params = value;
		}
	}

	public AlgorithmIdentifier()
	{
		_oid = new Oid("1.2.840.113549.3.7", "3des");
		_params = new byte[0];
	}

	public AlgorithmIdentifier(Oid oid)
	{
		_oid = oid;
		_params = new byte[0];
	}

	public AlgorithmIdentifier(Oid oid, int keyLength)
	{
		_oid = oid;
		_length = keyLength;
		_params = new byte[0];
	}
}
