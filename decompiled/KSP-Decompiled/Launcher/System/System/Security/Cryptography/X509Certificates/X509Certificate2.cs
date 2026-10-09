using System.IO;
using System.Runtime.Serialization;
using System.Text;
using Mono.Security;
using Mono.Security.X509;

namespace System.Security.Cryptography.X509Certificates;

[Serializable]
public class X509Certificate2 : X509Certificate
{
	private string friendlyName = string.Empty;

	private static byte[] signedData = new byte[9] { 42, 134, 72, 134, 247, 13, 1, 7, 2 };

	internal new X509Certificate2Impl Impl
	{
		get
		{
			X509Certificate2Impl result = base.Impl as X509Certificate2Impl;
			X509Helper2.ThrowIfContextInvalid(result);
			return result;
		}
	}

	public bool Archived
	{
		get
		{
			return Impl.Archived;
		}
		set
		{
			Impl.Archived = true;
		}
	}

	public X509ExtensionCollection Extensions => Impl.Extensions;

	public string FriendlyName
	{
		get
		{
			ThrowIfContextInvalid();
			return friendlyName;
		}
		set
		{
			ThrowIfContextInvalid();
			friendlyName = value;
		}
	}

	public bool HasPrivateKey => Impl.HasPrivateKey;

	public X500DistinguishedName IssuerName => Impl.IssuerName;

	public DateTime NotAfter => Impl.GetValidUntil().ToLocalTime();

	public DateTime NotBefore => Impl.GetValidFrom().ToLocalTime();

	public AsymmetricAlgorithm PrivateKey
	{
		get
		{
			return Impl.PrivateKey;
		}
		set
		{
			Impl.PrivateKey = value;
		}
	}

	public PublicKey PublicKey => Impl.PublicKey;

	public byte[] RawData => GetRawCertData();

	public string SerialNumber => GetSerialNumberString();

	public Oid SignatureAlgorithm => Impl.SignatureAlgorithm;

	public X500DistinguishedName SubjectName => Impl.SubjectName;

	public string Thumbprint => GetCertHashString();

	public int Version => Impl.Version;

	[System.MonoTODO("See comment in X509Helper2.GetMonoCertificate().")]
	internal Mono.Security.X509.X509Certificate MonoCertificate => X509Helper2.GetMonoCertificate(this);

	public X509Certificate2()
	{
	}

	public X509Certificate2(byte[] rawData)
	{
		Import(rawData, (string)null, X509KeyStorageFlags.DefaultKeySet);
	}

	public X509Certificate2(byte[] rawData, string password)
	{
		Import(rawData, password, X509KeyStorageFlags.DefaultKeySet);
	}

	public X509Certificate2(byte[] rawData, SecureString password)
	{
		Import(rawData, password, X509KeyStorageFlags.DefaultKeySet);
	}

	public X509Certificate2(byte[] rawData, string password, X509KeyStorageFlags keyStorageFlags)
	{
		Import(rawData, password, keyStorageFlags);
	}

	public X509Certificate2(byte[] rawData, SecureString password, X509KeyStorageFlags keyStorageFlags)
	{
		Import(rawData, password, keyStorageFlags);
	}

	public X509Certificate2(string fileName)
	{
		Import(fileName, string.Empty, X509KeyStorageFlags.DefaultKeySet);
	}

	public X509Certificate2(string fileName, string password)
	{
		Import(fileName, password, X509KeyStorageFlags.DefaultKeySet);
	}

	public X509Certificate2(string fileName, SecureString password)
	{
		Import(fileName, password, X509KeyStorageFlags.DefaultKeySet);
	}

	public X509Certificate2(string fileName, string password, X509KeyStorageFlags keyStorageFlags)
	{
		Import(fileName, password, keyStorageFlags);
	}

	public X509Certificate2(string fileName, SecureString password, X509KeyStorageFlags keyStorageFlags)
	{
		Import(fileName, password, keyStorageFlags);
	}

	public X509Certificate2(IntPtr handle)
		: base(handle)
	{
		throw new NotImplementedException();
	}

	public X509Certificate2(X509Certificate certificate)
		: base(X509Helper2.Import(certificate))
	{
	}

	protected X509Certificate2(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}

	internal X509Certificate2(X509Certificate2Impl impl)
		: base(impl)
	{
	}

	[System.MonoTODO("always return String.Empty for UpnName, DnsFromAlternativeName and UrlName")]
	public string GetNameInfo(X509NameType nameType, bool forIssuer)
	{
		return Impl.GetNameInfo(nameType, forIssuer);
	}

	public override void Import(byte[] rawData)
	{
		Import(rawData, (string)null, X509KeyStorageFlags.DefaultKeySet);
	}

	[System.MonoTODO("missing KeyStorageFlags support")]
	public override void Import(byte[] rawData, string password, X509KeyStorageFlags keyStorageFlags)
	{
		X509Certificate2Impl x509Certificate2Impl = X509Helper2.Import(rawData, password, keyStorageFlags);
		ImportHandle(x509Certificate2Impl);
	}

	[System.MonoTODO("SecureString is incomplete")]
	public override void Import(byte[] rawData, SecureString password, X509KeyStorageFlags keyStorageFlags)
	{
		Import(rawData, (string)null, keyStorageFlags);
	}

	public override void Import(string fileName)
	{
		byte[] rawData = File.ReadAllBytes(fileName);
		Import(rawData, (string)null, X509KeyStorageFlags.DefaultKeySet);
	}

	[System.MonoTODO("missing KeyStorageFlags support")]
	public override void Import(string fileName, string password, X509KeyStorageFlags keyStorageFlags)
	{
		byte[] rawData = File.ReadAllBytes(fileName);
		Import(rawData, password, keyStorageFlags);
	}

	[System.MonoTODO("SecureString is incomplete")]
	public override void Import(string fileName, SecureString password, X509KeyStorageFlags keyStorageFlags)
	{
		byte[] rawData = File.ReadAllBytes(fileName);
		Import(rawData, (string)null, keyStorageFlags);
	}

	[System.MonoTODO("X509ContentType.SerializedCert is not supported")]
	public override byte[] Export(X509ContentType contentType, string password)
	{
		return Impl.Export(contentType, password);
	}

	public override void Reset()
	{
		friendlyName = string.Empty;
		base.Reset();
	}

	public override string ToString()
	{
		if (!base.IsValid)
		{
			return "System.Security.Cryptography.X509Certificates.X509Certificate2";
		}
		return base.ToString(fVerbose: true);
	}

	public override string ToString(bool verbose)
	{
		if (!base.IsValid)
		{
			return "System.Security.Cryptography.X509Certificates.X509Certificate2";
		}
		if (!verbose)
		{
			return base.ToString(fVerbose: true);
		}
		string newLine = Environment.NewLine;
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.AppendFormat("[Version]{0}  V{1}{0}{0}", newLine, Version);
		stringBuilder.AppendFormat("[Subject]{0}  {1}{0}{0}", newLine, base.Subject);
		stringBuilder.AppendFormat("[Issuer]{0}  {1}{0}{0}", newLine, base.Issuer);
		stringBuilder.AppendFormat("[Serial Number]{0}  {1}{0}{0}", newLine, SerialNumber);
		stringBuilder.AppendFormat("[Not Before]{0}  {1}{0}{0}", newLine, NotBefore);
		stringBuilder.AppendFormat("[Not After]{0}  {1}{0}{0}", newLine, NotAfter);
		stringBuilder.AppendFormat("[Thumbprint]{0}  {1}{0}{0}", newLine, Thumbprint);
		stringBuilder.AppendFormat("[Signature Algorithm]{0}  {1}({2}){0}{0}", newLine, SignatureAlgorithm.FriendlyName, SignatureAlgorithm.Value);
		AsymmetricAlgorithm key = PublicKey.Key;
		stringBuilder.AppendFormat("[Public Key]{0}  Algorithm: ", newLine);
		if (key is RSA)
		{
			stringBuilder.Append("RSA");
		}
		else if (key is DSA)
		{
			stringBuilder.Append("DSA");
		}
		else
		{
			stringBuilder.Append(key.ToString());
		}
		stringBuilder.AppendFormat("{0}  Length: {1}{0}  Key Blob: ", newLine, key.KeySize);
		AppendBuffer(stringBuilder, PublicKey.EncodedKeyValue.RawData);
		stringBuilder.AppendFormat("{0}  Parameters: ", newLine);
		AppendBuffer(stringBuilder, PublicKey.EncodedParameters.RawData);
		stringBuilder.Append(newLine);
		return stringBuilder.ToString();
	}

	private static void AppendBuffer(StringBuilder sb, byte[] buffer)
	{
		if (buffer == null)
		{
			return;
		}
		for (int i = 0; i < buffer.Length; i++)
		{
			sb.Append(buffer[i].ToString("x2"));
			if (i < buffer.Length - 1)
			{
				sb.Append(" ");
			}
		}
	}

	[System.MonoTODO("by default this depends on the incomplete X509Chain")]
	public bool Verify()
	{
		return Impl.Verify(this);
	}

	[System.MonoTODO("Detection limited to Cert, Pfx, Pkcs12, Pkcs7 and Unknown")]
	public static X509ContentType GetCertContentType(byte[] rawData)
	{
		if (rawData == null || rawData.Length == 0)
		{
			throw new ArgumentException("rawData");
		}
		X509ContentType result = X509ContentType.Unknown;
		try
		{
			ASN1 aSN = new ASN1(rawData);
			if (aSN.Tag != 48)
			{
				throw new CryptographicException(Locale.GetText("Unable to decode certificate."));
			}
			if (aSN.Count == 0)
			{
				return result;
			}
			if (aSN.Count == 3)
			{
				switch (aSN[0].Tag)
				{
				case 48:
					if (aSN[1].Tag == 48 && aSN[2].Tag == 3)
					{
						result = X509ContentType.Cert;
					}
					break;
				case 2:
					if (aSN[1].Tag == 48 && aSN[2].Tag == 48)
					{
						result = X509ContentType.Pfx;
					}
					break;
				}
			}
			if (aSN[0].Tag == 6 && aSN[0].CompareValue(signedData))
			{
				result = X509ContentType.Pkcs7;
			}
		}
		catch (Exception inner)
		{
			throw new CryptographicException(Locale.GetText("Unable to decode certificate."), inner);
		}
		return result;
	}

	[System.MonoTODO("Detection limited to Cert, Pfx, Pkcs12 and Unknown")]
	public static X509ContentType GetCertContentType(string fileName)
	{
		if (fileName == null)
		{
			throw new ArgumentNullException("fileName");
		}
		if (fileName.Length == 0)
		{
			throw new ArgumentException("fileName");
		}
		return GetCertContentType(File.ReadAllBytes(fileName));
	}
}
