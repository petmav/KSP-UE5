using System.Security.Cryptography;

namespace Mono.Security.Protocol.Tls;

internal class TlsCipherSuite : CipherSuite
{
	private const int MacHeaderLength = 13;

	private byte[] header;

	private object headerLock = new object();

	public TlsCipherSuite(short code, string name, CipherAlgorithmType cipherAlgorithmType, HashAlgorithmType hashAlgorithmType, ExchangeAlgorithmType exchangeAlgorithmType, bool exportable, bool blockMode, byte keyMaterialSize, byte expandedKeyMaterialSize, short effectiveKeyBytes, byte ivSize, byte blockSize)
		: base(code, name, cipherAlgorithmType, hashAlgorithmType, exchangeAlgorithmType, exportable, blockMode, keyMaterialSize, expandedKeyMaterialSize, effectiveKeyBytes, ivSize, blockSize)
	{
	}

	public override byte[] ComputeServerRecordMAC(ContentType contentType, byte[] fragment)
	{
		lock (headerLock)
		{
			if (header == null)
			{
				header = new byte[13];
			}
			ulong value = ((base.Context is ClientContext) ? base.Context.ReadSequenceNumber : base.Context.WriteSequenceNumber);
			Write(header, 0, value);
			header[8] = (byte)contentType;
			Write(header, 9, base.Context.Protocol);
			Write(header, 11, (short)fragment.Length);
			KeyedHashAlgorithm keyedHashAlgorithm = base.ServerHMAC;
			keyedHashAlgorithm.TransformBlock(header, 0, header.Length, header, 0);
			keyedHashAlgorithm.TransformBlock(fragment, 0, fragment.Length, fragment, 0);
			keyedHashAlgorithm.TransformFinalBlock(CipherSuite.EmptyArray, 0, 0);
			return keyedHashAlgorithm.Hash;
		}
	}

	public override byte[] ComputeClientRecordMAC(ContentType contentType, byte[] fragment)
	{
		lock (headerLock)
		{
			if (header == null)
			{
				header = new byte[13];
			}
			ulong value = ((base.Context is ClientContext) ? base.Context.WriteSequenceNumber : base.Context.ReadSequenceNumber);
			Write(header, 0, value);
			header[8] = (byte)contentType;
			Write(header, 9, base.Context.Protocol);
			Write(header, 11, (short)fragment.Length);
			KeyedHashAlgorithm keyedHashAlgorithm = base.ClientHMAC;
			keyedHashAlgorithm.TransformBlock(header, 0, header.Length, header, 0);
			keyedHashAlgorithm.TransformBlock(fragment, 0, fragment.Length, fragment, 0);
			keyedHashAlgorithm.TransformFinalBlock(CipherSuite.EmptyArray, 0, 0);
			return keyedHashAlgorithm.Hash;
		}
	}

	public override void ComputeMasterSecret(byte[] preMasterSecret)
	{
		base.Context.MasterSecret = new byte[preMasterSecret.Length];
		base.Context.MasterSecret = PRF(preMasterSecret, "master secret", base.Context.RandomCS, 48);
	}

	public override void ComputeKeys()
	{
		TlsStream tlsStream = new TlsStream(PRF(base.Context.MasterSecret, "key expansion", base.Context.RandomSC, base.KeyBlockSize));
		base.Context.Negotiating.ClientWriteMAC = tlsStream.ReadBytes(base.HashSize);
		base.Context.Negotiating.ServerWriteMAC = tlsStream.ReadBytes(base.HashSize);
		base.Context.ClientWriteKey = tlsStream.ReadBytes(base.KeyMaterialSize);
		base.Context.ServerWriteKey = tlsStream.ReadBytes(base.KeyMaterialSize);
		if (base.IvSize != 0)
		{
			base.Context.ClientWriteIV = tlsStream.ReadBytes(base.IvSize);
			base.Context.ServerWriteIV = tlsStream.ReadBytes(base.IvSize);
		}
		else
		{
			base.Context.ClientWriteIV = CipherSuite.EmptyArray;
			base.Context.ServerWriteIV = CipherSuite.EmptyArray;
		}
		ClientSessionCache.SetContextInCache(base.Context);
		tlsStream.Reset();
	}
}
