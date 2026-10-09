using System.Security.Cryptography;
using System.Text;

namespace Mono.Security.Protocol.Tls;

internal class SslCipherSuite : CipherSuite
{
	private byte[] pad1;

	private byte[] pad2;

	private const int MacHeaderLength = 11;

	private byte[] header;

	public SslCipherSuite(short code, string name, CipherAlgorithmType cipherAlgorithmType, HashAlgorithmType hashAlgorithmType, ExchangeAlgorithmType exchangeAlgorithmType, bool exportable, bool blockMode, byte keyMaterialSize, byte expandedKeyMaterialSize, short effectiveKeyBytes, byte ivSize, byte blockSize)
		: base(code, name, cipherAlgorithmType, hashAlgorithmType, exchangeAlgorithmType, exportable, blockMode, keyMaterialSize, expandedKeyMaterialSize, effectiveKeyBytes, ivSize, blockSize)
	{
		int num = ((hashAlgorithmType == HashAlgorithmType.Md5) ? 48 : 40);
		pad1 = new byte[num];
		pad2 = new byte[num];
		for (int i = 0; i < num; i++)
		{
			pad1[i] = 54;
			pad2[i] = 92;
		}
	}

	public override byte[] ComputeServerRecordMAC(ContentType contentType, byte[] fragment)
	{
		HashAlgorithm hashAlgorithm = CreateHashAlgorithm();
		byte[] serverWriteMAC = base.Context.Read.ServerWriteMAC;
		hashAlgorithm.TransformBlock(serverWriteMAC, 0, serverWriteMAC.Length, serverWriteMAC, 0);
		hashAlgorithm.TransformBlock(pad1, 0, pad1.Length, pad1, 0);
		if (header == null)
		{
			header = new byte[11];
		}
		Write(value: (base.Context is ClientContext) ? base.Context.ReadSequenceNumber : base.Context.WriteSequenceNumber, array: header, offset: 0);
		header[8] = (byte)contentType;
		Write(header, 9, (short)fragment.Length);
		hashAlgorithm.TransformBlock(header, 0, header.Length, header, 0);
		hashAlgorithm.TransformBlock(fragment, 0, fragment.Length, fragment, 0);
		hashAlgorithm.TransformFinalBlock(CipherSuite.EmptyArray, 0, 0);
		byte[] hash = hashAlgorithm.Hash;
		hashAlgorithm.Initialize();
		hashAlgorithm.TransformBlock(serverWriteMAC, 0, serverWriteMAC.Length, serverWriteMAC, 0);
		hashAlgorithm.TransformBlock(pad2, 0, pad2.Length, pad2, 0);
		hashAlgorithm.TransformBlock(hash, 0, hash.Length, hash, 0);
		hashAlgorithm.TransformFinalBlock(CipherSuite.EmptyArray, 0, 0);
		return hashAlgorithm.Hash;
	}

	public override byte[] ComputeClientRecordMAC(ContentType contentType, byte[] fragment)
	{
		HashAlgorithm hashAlgorithm = CreateHashAlgorithm();
		byte[] clientWriteMAC = base.Context.Current.ClientWriteMAC;
		hashAlgorithm.TransformBlock(clientWriteMAC, 0, clientWriteMAC.Length, clientWriteMAC, 0);
		hashAlgorithm.TransformBlock(pad1, 0, pad1.Length, pad1, 0);
		if (header == null)
		{
			header = new byte[11];
		}
		Write(value: (base.Context is ClientContext) ? base.Context.WriteSequenceNumber : base.Context.ReadSequenceNumber, array: header, offset: 0);
		header[8] = (byte)contentType;
		Write(header, 9, (short)fragment.Length);
		hashAlgorithm.TransformBlock(header, 0, header.Length, header, 0);
		hashAlgorithm.TransformBlock(fragment, 0, fragment.Length, fragment, 0);
		hashAlgorithm.TransformFinalBlock(CipherSuite.EmptyArray, 0, 0);
		byte[] hash = hashAlgorithm.Hash;
		hashAlgorithm.Initialize();
		hashAlgorithm.TransformBlock(clientWriteMAC, 0, clientWriteMAC.Length, clientWriteMAC, 0);
		hashAlgorithm.TransformBlock(pad2, 0, pad2.Length, pad2, 0);
		hashAlgorithm.TransformBlock(hash, 0, hash.Length, hash, 0);
		hashAlgorithm.TransformFinalBlock(CipherSuite.EmptyArray, 0, 0);
		return hashAlgorithm.Hash;
	}

	public override void ComputeMasterSecret(byte[] preMasterSecret)
	{
		TlsStream tlsStream = new TlsStream();
		tlsStream.Write(prf(preMasterSecret, "A", base.Context.RandomCS));
		tlsStream.Write(prf(preMasterSecret, "BB", base.Context.RandomCS));
		tlsStream.Write(prf(preMasterSecret, "CCC", base.Context.RandomCS));
		base.Context.MasterSecret = tlsStream.ToArray();
	}

	public override void ComputeKeys()
	{
		TlsStream tlsStream = new TlsStream();
		char c = 'A';
		int num = 1;
		while (tlsStream.Length < base.KeyBlockSize)
		{
			string text = string.Empty;
			for (int i = 0; i < num; i++)
			{
				text += c;
			}
			byte[] array = prf(base.Context.MasterSecret, text.ToString(), base.Context.RandomSC);
			int count = ((tlsStream.Length + array.Length > base.KeyBlockSize) ? (base.KeyBlockSize - (int)tlsStream.Length) : array.Length);
			tlsStream.Write(array, 0, count);
			c = (char)(c + 1);
			num++;
		}
		TlsStream tlsStream2 = new TlsStream(tlsStream.ToArray());
		base.Context.Negotiating.ClientWriteMAC = tlsStream2.ReadBytes(base.HashSize);
		base.Context.Negotiating.ServerWriteMAC = tlsStream2.ReadBytes(base.HashSize);
		base.Context.ClientWriteKey = tlsStream2.ReadBytes(base.KeyMaterialSize);
		base.Context.ServerWriteKey = tlsStream2.ReadBytes(base.KeyMaterialSize);
		if (base.IvSize != 0)
		{
			base.Context.ClientWriteIV = tlsStream2.ReadBytes(base.IvSize);
			base.Context.ServerWriteIV = tlsStream2.ReadBytes(base.IvSize);
		}
		else
		{
			base.Context.ClientWriteIV = CipherSuite.EmptyArray;
			base.Context.ServerWriteIV = CipherSuite.EmptyArray;
		}
		ClientSessionCache.SetContextInCache(base.Context);
		tlsStream2.Reset();
		tlsStream.Reset();
	}

	private byte[] prf(byte[] secret, string label, byte[] random)
	{
		MD5 mD = MD5.Create();
		SHA1 sHA = SHA1.Create();
		TlsStream tlsStream = new TlsStream();
		tlsStream.Write(Encoding.ASCII.GetBytes(label));
		tlsStream.Write(secret);
		tlsStream.Write(random);
		byte[] buffer = sHA.ComputeHash(tlsStream.ToArray(), 0, (int)tlsStream.Length);
		tlsStream.Reset();
		tlsStream.Write(secret);
		tlsStream.Write(buffer);
		byte[] result = mD.ComputeHash(tlsStream.ToArray(), 0, (int)tlsStream.Length);
		tlsStream.Reset();
		return result;
	}
}
