using System.Runtime.InteropServices;

namespace System.Security.Cryptography;

[ComVisible(true)]
public class HMACSHA512 : HMAC
{
	private bool m_useLegacyBlockSize = Utils._ProduceLegacyHmacValues();

	private int BlockSize
	{
		get
		{
			if (!m_useLegacyBlockSize)
			{
				return 128;
			}
			return 64;
		}
	}

	public bool ProduceLegacyHmacValues
	{
		get
		{
			return m_useLegacyBlockSize;
		}
		set
		{
			m_useLegacyBlockSize = value;
			base.BlockSizeValue = BlockSize;
			InitializeKey(KeyValue);
		}
	}

	public HMACSHA512()
		: this(Utils.GenerateRandom(128))
	{
	}

	[SecuritySafeCritical]
	public HMACSHA512(byte[] key)
	{
		m_hashName = "SHA512";
		m_hash1 = new SHA512Managed();
		m_hash2 = new SHA512Managed();
		HashSizeValue = 512;
		base.BlockSizeValue = BlockSize;
		InitializeKey(key);
	}
}
