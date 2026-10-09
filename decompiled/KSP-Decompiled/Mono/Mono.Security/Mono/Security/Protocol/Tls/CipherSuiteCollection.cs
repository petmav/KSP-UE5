using System;
using System.Collections.Generic;

namespace Mono.Security.Protocol.Tls;

internal sealed class CipherSuiteCollection : List<CipherSuite>
{
	private SecurityProtocolType protocol;

	public CipherSuite this[string name]
	{
		get
		{
			int num = IndexOf(name);
			if (num != -1)
			{
				return base[num];
			}
			return null;
		}
	}

	public CipherSuite this[short code]
	{
		get
		{
			int num = IndexOf(code);
			if (num != -1)
			{
				return base[num];
			}
			return null;
		}
	}

	public CipherSuiteCollection(SecurityProtocolType protocol)
	{
		switch (protocol)
		{
		case SecurityProtocolType.Default:
		case SecurityProtocolType.Ssl3:
		case SecurityProtocolType.Tls:
			this.protocol = protocol;
			break;
		default:
			throw new NotSupportedException("Unsupported security protocol type.");
		}
	}

	public int IndexOf(string name)
	{
		int num = 0;
		using (Enumerator enumerator = GetEnumerator())
		{
			while (enumerator.MoveNext())
			{
				CipherSuite current = enumerator.Current;
				if (string.CompareOrdinal(name, current.Name) == 0)
				{
					return num;
				}
				num++;
			}
		}
		return -1;
	}

	public int IndexOf(short code)
	{
		int num = 0;
		using (Enumerator enumerator = GetEnumerator())
		{
			while (enumerator.MoveNext())
			{
				if (enumerator.Current.Code == code)
				{
					return num;
				}
				num++;
			}
		}
		return -1;
	}

	public void Add(short code, string name, CipherAlgorithmType cipherType, HashAlgorithmType hashType, ExchangeAlgorithmType exchangeType, bool exportable, bool blockMode, byte keyMaterialSize, byte expandedKeyMaterialSize, short effectiveKeyBytes, byte ivSize, byte blockSize)
	{
		switch (protocol)
		{
		case SecurityProtocolType.Default:
		case SecurityProtocolType.Tls:
			Add(new TlsCipherSuite(code, name, cipherType, hashType, exchangeType, exportable, blockMode, keyMaterialSize, expandedKeyMaterialSize, effectiveKeyBytes, ivSize, blockSize));
			break;
		case SecurityProtocolType.Ssl3:
			Add(new SslCipherSuite(code, name, cipherType, hashType, exchangeType, exportable, blockMode, keyMaterialSize, expandedKeyMaterialSize, effectiveKeyBytes, ivSize, blockSize));
			break;
		}
	}

	public IList<string> GetNames()
	{
		List<string> list = new List<string>(base.Count);
		using Enumerator enumerator = GetEnumerator();
		while (enumerator.MoveNext())
		{
			CipherSuite current = enumerator.Current;
			list.Add(current.Name);
		}
		return list;
	}
}
