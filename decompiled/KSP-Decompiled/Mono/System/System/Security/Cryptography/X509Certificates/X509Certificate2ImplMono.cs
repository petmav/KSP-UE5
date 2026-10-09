using System.Collections;
using System.IO;
using System.Text;
using Mono.Security;
using Mono.Security.Cryptography;
using Mono.Security.X509;

namespace System.Security.Cryptography.X509Certificates;

internal class X509Certificate2ImplMono : X509Certificate2Impl
{
	private bool _archived;

	private X509ExtensionCollection _extensions;

	private PublicKey _publicKey;

	private X500DistinguishedName issuer_name;

	private X500DistinguishedName subject_name;

	private Oid signature_algorithm;

	private X509CertificateImplCollection intermediateCerts;

	private Mono.Security.X509.X509Certificate _cert;

	private static string empty_error = global::Locale.GetText("Certificate instance is empty.");

	private static byte[] commonName = new byte[3] { 85, 4, 3 };

	private static byte[] email = new byte[9] { 42, 134, 72, 134, 247, 13, 1, 9, 1 };

	private static byte[] signedData = new byte[9] { 42, 134, 72, 134, 247, 13, 1, 7, 2 };

	public override bool IsValid => _cert != null;

	public override IntPtr Handle => IntPtr.Zero;

	public override bool Archived
	{
		get
		{
			if (_cert == null)
			{
				throw new CryptographicException(empty_error);
			}
			return _archived;
		}
		set
		{
			if (_cert == null)
			{
				throw new CryptographicException(empty_error);
			}
			_archived = value;
		}
	}

	public override X509ExtensionCollection Extensions
	{
		get
		{
			if (_cert == null)
			{
				throw new CryptographicException(empty_error);
			}
			if (_extensions == null)
			{
				_extensions = new X509ExtensionCollection(_cert);
			}
			return _extensions;
		}
	}

	public override bool HasPrivateKey => PrivateKey != null;

	public override X500DistinguishedName IssuerName
	{
		get
		{
			if (_cert == null)
			{
				throw new CryptographicException(empty_error);
			}
			if (issuer_name == null)
			{
				issuer_name = new X500DistinguishedName(_cert.GetIssuerName().GetBytes());
			}
			return issuer_name;
		}
	}

	public override AsymmetricAlgorithm PrivateKey
	{
		get
		{
			if (_cert == null)
			{
				throw new CryptographicException(empty_error);
			}
			try
			{
				if (_cert.RSA != null)
				{
					if (_cert.RSA is RSACryptoServiceProvider rSACryptoServiceProvider)
					{
						return rSACryptoServiceProvider.PublicOnly ? null : rSACryptoServiceProvider;
					}
					if (_cert.RSA is RSAManaged rSAManaged)
					{
						return rSAManaged.PublicOnly ? null : rSAManaged;
					}
					_cert.RSA.ExportParameters(includePrivateParameters: true);
					return _cert.RSA;
				}
				if (_cert.DSA != null)
				{
					if (_cert.DSA is DSACryptoServiceProvider dSACryptoServiceProvider)
					{
						return dSACryptoServiceProvider.PublicOnly ? null : dSACryptoServiceProvider;
					}
					_cert.DSA.ExportParameters(includePrivateParameters: true);
					return _cert.DSA;
				}
			}
			catch
			{
			}
			return null;
		}
		set
		{
			if (_cert == null)
			{
				throw new CryptographicException(empty_error);
			}
			if (value == null)
			{
				_cert.RSA = null;
				_cert.DSA = null;
				return;
			}
			if (value is RSA)
			{
				_cert.RSA = (RSA)value;
				return;
			}
			if (value is DSA)
			{
				_cert.DSA = (DSA)value;
				return;
			}
			throw new NotSupportedException();
		}
	}

	public override PublicKey PublicKey
	{
		get
		{
			if (_cert == null)
			{
				throw new CryptographicException(empty_error);
			}
			if (_publicKey == null)
			{
				try
				{
					_publicKey = new PublicKey(_cert);
				}
				catch (Exception inner)
				{
					throw new CryptographicException(global::Locale.GetText("Unable to decode public key."), inner);
				}
			}
			return _publicKey;
		}
	}

	public override Oid SignatureAlgorithm
	{
		get
		{
			if (_cert == null)
			{
				throw new CryptographicException(empty_error);
			}
			if (signature_algorithm == null)
			{
				signature_algorithm = new Oid(_cert.SignatureAlgorithm);
			}
			return signature_algorithm;
		}
	}

	public override X500DistinguishedName SubjectName
	{
		get
		{
			if (_cert == null)
			{
				throw new CryptographicException(empty_error);
			}
			if (subject_name == null)
			{
				subject_name = new X500DistinguishedName(_cert.GetSubjectName().GetBytes());
			}
			return subject_name;
		}
	}

	public override int Version
	{
		get
		{
			if (_cert == null)
			{
				throw new CryptographicException(empty_error);
			}
			return _cert.Version;
		}
	}

	internal override X509CertificateImplCollection IntermediateCertificates => intermediateCerts;

	internal Mono.Security.X509.X509Certificate MonoCertificate => _cert;

	internal override X509Certificate2Impl FallbackImpl => this;

	public override IntPtr GetNativeAppleCertificate()
	{
		return IntPtr.Zero;
	}

	private X509Certificate2ImplMono(Mono.Security.X509.X509Certificate cert)
	{
		_cert = cert;
	}

	private X509Certificate2ImplMono(X509Certificate2ImplMono other)
	{
		_cert = other._cert;
		if (other.intermediateCerts != null)
		{
			intermediateCerts = other.intermediateCerts.Clone();
		}
	}

	public override X509CertificateImpl Clone()
	{
		ThrowIfContextInvalid();
		return new X509Certificate2ImplMono(this);
	}

	public override string GetIssuerName(bool legacyV1Mode)
	{
		ThrowIfContextInvalid();
		if (legacyV1Mode)
		{
			return _cert.IssuerName;
		}
		return X501.ToString(_cert.GetIssuerName(), reversed: true, ", ", quotes: true);
	}

	public override string GetSubjectName(bool legacyV1Mode)
	{
		ThrowIfContextInvalid();
		if (legacyV1Mode)
		{
			return _cert.SubjectName;
		}
		return X501.ToString(_cert.GetSubjectName(), reversed: true, ", ", quotes: true);
	}

	public override byte[] GetRawCertData()
	{
		ThrowIfContextInvalid();
		return _cert.RawData;
	}

	protected override byte[] GetCertHash(bool lazy)
	{
		ThrowIfContextInvalid();
		return SHA1.Create().ComputeHash(_cert.RawData);
	}

	public override DateTime GetValidFrom()
	{
		ThrowIfContextInvalid();
		return _cert.ValidFrom;
	}

	public override DateTime GetValidUntil()
	{
		ThrowIfContextInvalid();
		return _cert.ValidUntil;
	}

	public override bool Equals(X509CertificateImpl other, out bool result)
	{
		result = false;
		return false;
	}

	public override string GetKeyAlgorithm()
	{
		ThrowIfContextInvalid();
		return _cert.KeyAlgorithm;
	}

	public override byte[] GetKeyAlgorithmParameters()
	{
		ThrowIfContextInvalid();
		return _cert.KeyAlgorithmParameters;
	}

	public override byte[] GetPublicKey()
	{
		ThrowIfContextInvalid();
		return _cert.PublicKey;
	}

	public override byte[] GetSerialNumber()
	{
		ThrowIfContextInvalid();
		return _cert.SerialNumber;
	}

	public override byte[] Export(X509ContentType contentType, byte[] password)
	{
		ThrowIfContextInvalid();
		return contentType switch
		{
			X509ContentType.Cert => GetRawCertData(), 
			X509ContentType.Pfx => throw new NotSupportedException(), 
			X509ContentType.SerializedCert => throw new NotSupportedException(), 
			_ => throw new CryptographicException(global::Locale.GetText("This certificate format '{0}' cannot be exported.", contentType)), 
		};
	}

	public X509Certificate2ImplMono()
	{
		_cert = null;
	}

	[System.MonoTODO("always return String.Empty for UpnName, DnsFromAlternativeName and UrlName")]
	public override string GetNameInfo(X509NameType nameType, bool forIssuer)
	{
		switch (nameType)
		{
		case X509NameType.SimpleName:
		{
			if (_cert == null)
			{
				throw new CryptographicException(empty_error);
			}
			ASN1 aSN2 = (forIssuer ? _cert.GetIssuerName() : _cert.GetSubjectName());
			ASN1 aSN3 = Find(commonName, aSN2);
			if (aSN3 != null)
			{
				return GetValueAsString(aSN3);
			}
			if (aSN2.Count == 0)
			{
				return string.Empty;
			}
			ASN1 aSN4 = aSN2[aSN2.Count - 1];
			if (aSN4.Count == 0)
			{
				return string.Empty;
			}
			return GetValueAsString(aSN4[0]);
		}
		case X509NameType.EmailName:
		{
			ASN1 aSN5 = Find(email, forIssuer ? _cert.GetIssuerName() : _cert.GetSubjectName());
			if (aSN5 != null)
			{
				return GetValueAsString(aSN5);
			}
			return string.Empty;
		}
		case X509NameType.UpnName:
			return string.Empty;
		case X509NameType.DnsName:
		{
			ASN1 aSN = Find(commonName, forIssuer ? _cert.GetIssuerName() : _cert.GetSubjectName());
			if (aSN != null)
			{
				return GetValueAsString(aSN);
			}
			return string.Empty;
		}
		case X509NameType.DnsFromAlternativeName:
			return string.Empty;
		case X509NameType.UrlName:
			return string.Empty;
		default:
			throw new ArgumentException("nameType");
		}
	}

	private ASN1 Find(byte[] oid, ASN1 dn)
	{
		if (dn.Count == 0)
		{
			return null;
		}
		for (int i = 0; i < dn.Count; i++)
		{
			ASN1 aSN = dn[i];
			for (int j = 0; j < aSN.Count; j++)
			{
				ASN1 aSN2 = aSN[j];
				if (aSN2.Count == 2)
				{
					ASN1 aSN3 = aSN2[0];
					if (aSN3 != null && aSN3.CompareValue(oid))
					{
						return aSN2;
					}
				}
			}
		}
		return null;
	}

	private string GetValueAsString(ASN1 pair)
	{
		if (pair.Count != 2)
		{
			return string.Empty;
		}
		ASN1 aSN = pair[1];
		if (aSN.Value == null || aSN.Length == 0)
		{
			return string.Empty;
		}
		if (aSN.Tag == 30)
		{
			StringBuilder stringBuilder = new StringBuilder();
			for (int i = 1; i < aSN.Value.Length; i += 2)
			{
				stringBuilder.Append((char)aSN.Value[i]);
			}
			return stringBuilder.ToString();
		}
		return Encoding.UTF8.GetString(aSN.Value);
	}

	private Mono.Security.X509.X509Certificate ImportPkcs12(byte[] rawData, string password)
	{
		PKCS12 pKCS = null;
		if (string.IsNullOrEmpty(password))
		{
			try
			{
				pKCS = new PKCS12(rawData, (string)null);
			}
			catch
			{
				pKCS = new PKCS12(rawData, string.Empty);
			}
		}
		else
		{
			pKCS = new PKCS12(rawData, password);
		}
		if (pKCS.Certificates.Count == 0)
		{
			return null;
		}
		if (pKCS.Keys.Count == 0)
		{
			return pKCS.Certificates[0];
		}
		Mono.Security.X509.X509Certificate x509Certificate = null;
		AsymmetricAlgorithm asymmetricAlgorithm = pKCS.Keys[0] as AsymmetricAlgorithm;
		string text = asymmetricAlgorithm.ToXmlString(includePrivateParameters: false);
		foreach (Mono.Security.X509.X509Certificate certificate in pKCS.Certificates)
		{
			if ((certificate.RSA != null && text == certificate.RSA.ToXmlString(includePrivateParameters: false)) || (certificate.DSA != null && text == certificate.DSA.ToXmlString(includePrivateParameters: false)))
			{
				x509Certificate = certificate;
				break;
			}
		}
		if (x509Certificate == null)
		{
			x509Certificate = pKCS.Certificates[0];
		}
		else
		{
			x509Certificate.RSA = asymmetricAlgorithm as RSA;
			x509Certificate.DSA = asymmetricAlgorithm as DSA;
		}
		if (pKCS.Certificates.Count > 1)
		{
			intermediateCerts = new X509CertificateImplCollection();
			foreach (Mono.Security.X509.X509Certificate certificate2 in pKCS.Certificates)
			{
				if (certificate2 != x509Certificate)
				{
					X509Certificate2ImplMono impl = new X509Certificate2ImplMono(certificate2);
					intermediateCerts.Add(impl, takeOwnership: true);
				}
			}
		}
		return x509Certificate;
	}

	[System.MonoTODO("missing KeyStorageFlags support")]
	public override void Import(byte[] rawData, string password, X509KeyStorageFlags keyStorageFlags)
	{
		Reset();
		Mono.Security.X509.X509Certificate x509Certificate = null;
		if (password == null)
		{
			try
			{
				x509Certificate = new Mono.Security.X509.X509Certificate(rawData);
			}
			catch (Exception inner)
			{
				try
				{
					x509Certificate = ImportPkcs12(rawData, null);
				}
				catch
				{
					throw new CryptographicException(global::Locale.GetText("Unable to decode certificate."), inner);
				}
			}
		}
		else
		{
			try
			{
				x509Certificate = ImportPkcs12(rawData, password);
			}
			catch
			{
				x509Certificate = new Mono.Security.X509.X509Certificate(rawData);
			}
		}
		_cert = x509Certificate;
	}

	[System.MonoTODO("X509ContentType.SerializedCert is not supported")]
	public override byte[] Export(X509ContentType contentType, string password)
	{
		if (_cert == null)
		{
			throw new CryptographicException(empty_error);
		}
		return contentType switch
		{
			X509ContentType.Cert => _cert.RawData, 
			X509ContentType.Pfx => ExportPkcs12(password), 
			X509ContentType.SerializedCert => throw new NotSupportedException(), 
			_ => throw new CryptographicException(global::Locale.GetText("This certificate format '{0}' cannot be exported.", contentType)), 
		};
	}

	private byte[] ExportPkcs12(string password)
	{
		PKCS12 pKCS = new PKCS12();
		try
		{
			Hashtable hashtable = new Hashtable();
			ArrayList arrayList = new ArrayList();
			arrayList.Add(new byte[4] { 1, 0, 0, 0 });
			hashtable.Add("1.2.840.113549.1.9.21", arrayList);
			if (password != null)
			{
				pKCS.Password = password;
			}
			pKCS.AddCertificate(_cert, hashtable);
			AsymmetricAlgorithm privateKey = PrivateKey;
			if (privateKey != null)
			{
				pKCS.AddPkcs8ShroudedKeyBag(privateKey, hashtable);
			}
			return pKCS.GetBytes();
		}
		finally
		{
			pKCS.Password = null;
		}
	}

	public override void Reset()
	{
		_cert = null;
		_archived = false;
		_extensions = null;
		_publicKey = null;
		issuer_name = null;
		subject_name = null;
		signature_algorithm = null;
		if (intermediateCerts != null)
		{
			intermediateCerts.Dispose();
			intermediateCerts = null;
		}
	}

	public override string ToString()
	{
		if (_cert == null)
		{
			return "System.Security.Cryptography.X509Certificates.X509Certificate2";
		}
		return ToString(verbose: true);
	}

	public override string ToString(bool verbose)
	{
		if (_cert == null)
		{
			return "System.Security.Cryptography.X509Certificates.X509Certificate2";
		}
		string newLine = Environment.NewLine;
		StringBuilder stringBuilder = new StringBuilder();
		if (!verbose)
		{
			stringBuilder.AppendFormat("[Subject]{0}  {1}{0}{0}", newLine, GetSubjectName(legacyV1Mode: false));
			stringBuilder.AppendFormat("[Issuer]{0}  {1}{0}{0}", newLine, GetIssuerName(legacyV1Mode: false));
			stringBuilder.AppendFormat("[Not Before]{0}  {1}{0}{0}", newLine, GetValidFrom().ToLocalTime());
			stringBuilder.AppendFormat("[Not After]{0}  {1}{0}{0}", newLine, GetValidUntil().ToLocalTime());
			stringBuilder.AppendFormat("[Thumbprint]{0}  {1}{0}", newLine, X509Helper.ToHexString(GetCertHash()));
			stringBuilder.Append(newLine);
			return stringBuilder.ToString();
		}
		stringBuilder.AppendFormat("[Version]{0}  V{1}{0}{0}", newLine, Version);
		stringBuilder.AppendFormat("[Subject]{0}  {1}{0}{0}", newLine, GetSubjectName(legacyV1Mode: false));
		stringBuilder.AppendFormat("[Issuer]{0}  {1}{0}{0}", newLine, GetIssuerName(legacyV1Mode: false));
		stringBuilder.AppendFormat("[Serial Number]{0}  {1}{0}{0}", newLine, GetSerialNumber());
		stringBuilder.AppendFormat("[Not Before]{0}  {1}{0}{0}", newLine, GetValidFrom().ToLocalTime());
		stringBuilder.AppendFormat("[Not After]{0}  {1}{0}{0}", newLine, GetValidUntil().ToLocalTime());
		stringBuilder.AppendFormat("[Thumbprint]{0}  {1}{0}", newLine, X509Helper.ToHexString(GetCertHash()));
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
	public override bool Verify(X509Certificate2 thisCertificate)
	{
		if (_cert == null)
		{
			throw new CryptographicException(empty_error);
		}
		if (!X509Chain.Create().Build(thisCertificate))
		{
			return false;
		}
		return true;
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
				throw new CryptographicException(global::Locale.GetText("Unable to decode certificate."));
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
			throw new CryptographicException(global::Locale.GetText("Unable to decode certificate."), inner);
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
