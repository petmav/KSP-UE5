using System.IO;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Text;
using Mono.Security.Authenticode;

namespace System.Security.Cryptography.X509Certificates;

[Serializable]
[ComVisible(true)]
[MonoTODO("X509ContentType.SerializedCert isn't supported (anywhere in the class)")]
public class X509Certificate : IDeserializationCallback, ISerializable, IDisposable
{
	private X509CertificateImpl impl;

	private bool hideDates;

	private string issuer_name;

	private string subject_name;

	internal X509CertificateImpl Impl
	{
		get
		{
			X509Helper.ThrowIfContextInvalid(impl);
			return impl;
		}
	}

	internal bool IsValid => X509Helper.IsValid(impl);

	public string Issuer
	{
		get
		{
			X509Helper.ThrowIfContextInvalid(impl);
			if (issuer_name == null)
			{
				issuer_name = impl.GetIssuerName(legacyV1Mode: false);
			}
			return issuer_name;
		}
	}

	public string Subject
	{
		get
		{
			X509Helper.ThrowIfContextInvalid(impl);
			if (subject_name == null)
			{
				subject_name = impl.GetSubjectName(legacyV1Mode: false);
			}
			return subject_name;
		}
	}

	[ComVisible(false)]
	public IntPtr Handle
	{
		get
		{
			if (X509Helper.IsValid(impl))
			{
				return impl.Handle;
			}
			return IntPtr.Zero;
		}
	}

	public static X509Certificate CreateFromCertFile(string filename)
	{
		return new X509Certificate(File.ReadAllBytes(filename));
	}

	[MonoTODO("Incomplete - minimal validation in this version")]
	public static X509Certificate CreateFromSignedFile(string filename)
	{
		try
		{
			AuthenticodeDeformatter authenticodeDeformatter = new AuthenticodeDeformatter(filename);
			if (authenticodeDeformatter.SigningCertificate != null)
			{
				return new X509Certificate(authenticodeDeformatter.SigningCertificate.RawData);
			}
		}
		catch (SecurityException)
		{
			throw;
		}
		catch (Exception inner)
		{
			throw new COMException(Locale.GetText("Couldn't extract digital signature from {0}.", filename), inner);
		}
		throw new CryptographicException(Locale.GetText("{0} isn't signed.", filename));
	}

	internal X509Certificate(byte[] data, bool dates)
	{
		if (data != null)
		{
			Import(data, (string)null, X509KeyStorageFlags.DefaultKeySet);
			hideDates = !dates;
		}
	}

	public X509Certificate(byte[] data)
		: this(data, dates: true)
	{
	}

	public X509Certificate(IntPtr handle)
	{
		if (handle == IntPtr.Zero)
		{
			throw new ArgumentException("Invalid handle.");
		}
		impl = X509Helper.InitFromHandle(handle);
	}

	internal X509Certificate(X509CertificateImpl impl)
	{
		if (impl == null)
		{
			throw new ArgumentNullException("impl");
		}
		this.impl = X509Helper.InitFromCertificate(impl);
	}

	public X509Certificate(X509Certificate cert)
	{
		if (cert == null)
		{
			throw new ArgumentNullException("cert");
		}
		impl = X509Helper.InitFromCertificate(cert);
		hideDates = false;
	}

	internal void ImportHandle(X509CertificateImpl impl)
	{
		Reset();
		this.impl = impl;
	}

	internal void ThrowIfContextInvalid()
	{
		X509Helper.ThrowIfContextInvalid(impl);
	}

	public virtual bool Equals(X509Certificate other)
	{
		if (other == null)
		{
			return false;
		}
		if (!X509Helper.IsValid(other.impl))
		{
			if (!X509Helper.IsValid(impl))
			{
				return true;
			}
			throw new CryptographicException(Locale.GetText("Certificate instance is empty."));
		}
		return object.Equals(impl, other.impl);
	}

	public virtual byte[] GetCertHash()
	{
		X509Helper.ThrowIfContextInvalid(impl);
		return impl.GetCertHash();
	}

	public virtual string GetCertHashString()
	{
		return X509Helper.ToHexString(GetCertHash());
	}

	public virtual string GetEffectiveDateString()
	{
		if (hideDates)
		{
			return null;
		}
		X509Helper.ThrowIfContextInvalid(impl);
		return impl.GetValidFrom().ToLocalTime().ToString();
	}

	public virtual string GetExpirationDateString()
	{
		if (hideDates)
		{
			return null;
		}
		X509Helper.ThrowIfContextInvalid(impl);
		return impl.GetValidUntil().ToLocalTime().ToString();
	}

	public virtual string GetFormat()
	{
		return "X509";
	}

	public override int GetHashCode()
	{
		if (!X509Helper.IsValid(impl))
		{
			return 0;
		}
		return impl.GetHashCode();
	}

	[Obsolete("Use the Issuer property.")]
	public virtual string GetIssuerName()
	{
		X509Helper.ThrowIfContextInvalid(impl);
		return impl.GetIssuerName(legacyV1Mode: true);
	}

	public virtual string GetKeyAlgorithm()
	{
		X509Helper.ThrowIfContextInvalid(impl);
		return impl.GetKeyAlgorithm();
	}

	public virtual byte[] GetKeyAlgorithmParameters()
	{
		X509Helper.ThrowIfContextInvalid(impl);
		return impl.GetKeyAlgorithmParameters() ?? throw new CryptographicException(Locale.GetText("Parameters not part of the certificate"));
	}

	public virtual string GetKeyAlgorithmParametersString()
	{
		return X509Helper.ToHexString(GetKeyAlgorithmParameters());
	}

	[Obsolete("Use the Subject property.")]
	public virtual string GetName()
	{
		X509Helper.ThrowIfContextInvalid(impl);
		return impl.GetSubjectName(legacyV1Mode: true);
	}

	public virtual byte[] GetPublicKey()
	{
		X509Helper.ThrowIfContextInvalid(impl);
		return impl.GetPublicKey();
	}

	public virtual string GetPublicKeyString()
	{
		return X509Helper.ToHexString(GetPublicKey());
	}

	public virtual byte[] GetRawCertData()
	{
		X509Helper.ThrowIfContextInvalid(impl);
		return impl.GetRawCertData();
	}

	public virtual string GetRawCertDataString()
	{
		X509Helper.ThrowIfContextInvalid(impl);
		return X509Helper.ToHexString(impl.GetRawCertData());
	}

	public virtual byte[] GetSerialNumber()
	{
		X509Helper.ThrowIfContextInvalid(impl);
		return impl.GetSerialNumber();
	}

	public virtual string GetSerialNumberString()
	{
		byte[] serialNumber = GetSerialNumber();
		Array.Reverse(serialNumber);
		return X509Helper.ToHexString(serialNumber);
	}

	public override string ToString()
	{
		return base.ToString();
	}

	public virtual string ToString(bool fVerbose)
	{
		if (!fVerbose || !X509Helper.IsValid(impl))
		{
			return base.ToString();
		}
		return impl.ToString(full: true);
	}

	protected static string FormatDate(DateTime date)
	{
		throw new NotImplementedException();
	}

	public X509Certificate()
	{
	}

	public X509Certificate(byte[] rawData, string password)
	{
		Import(rawData, password, X509KeyStorageFlags.DefaultKeySet);
	}

	[MonoTODO("SecureString support is incomplete")]
	public X509Certificate(byte[] rawData, SecureString password)
	{
		Import(rawData, password, X509KeyStorageFlags.DefaultKeySet);
	}

	public X509Certificate(byte[] rawData, string password, X509KeyStorageFlags keyStorageFlags)
	{
		Import(rawData, password, keyStorageFlags);
	}

	[MonoTODO("SecureString support is incomplete")]
	public X509Certificate(byte[] rawData, SecureString password, X509KeyStorageFlags keyStorageFlags)
	{
		Import(rawData, password, keyStorageFlags);
	}

	public X509Certificate(string fileName)
	{
		Import(fileName, (string)null, X509KeyStorageFlags.DefaultKeySet);
	}

	public X509Certificate(string fileName, string password)
	{
		Import(fileName, password, X509KeyStorageFlags.DefaultKeySet);
	}

	[MonoTODO("SecureString support is incomplete")]
	public X509Certificate(string fileName, SecureString password)
	{
		Import(fileName, password, X509KeyStorageFlags.DefaultKeySet);
	}

	public X509Certificate(string fileName, string password, X509KeyStorageFlags keyStorageFlags)
	{
		Import(fileName, password, keyStorageFlags);
	}

	[MonoTODO("SecureString support is incomplete")]
	public X509Certificate(string fileName, SecureString password, X509KeyStorageFlags keyStorageFlags)
	{
		Import(fileName, password, keyStorageFlags);
	}

	public X509Certificate(SerializationInfo info, StreamingContext context)
	{
		byte[] rawData = (byte[])info.GetValue("RawData", typeof(byte[]));
		Import(rawData, (string)null, X509KeyStorageFlags.DefaultKeySet);
	}

	[ComVisible(false)]
	public override bool Equals(object obj)
	{
		if (obj is X509Certificate other)
		{
			return Equals(other);
		}
		return false;
	}

	[ComVisible(false)]
	[MonoTODO("X509ContentType.Pfx/Pkcs12 and SerializedCert are not supported")]
	public virtual byte[] Export(X509ContentType contentType)
	{
		return Export(contentType, (byte[])null);
	}

	[MonoTODO("X509ContentType.Pfx/Pkcs12 and SerializedCert are not supported")]
	[ComVisible(false)]
	public virtual byte[] Export(X509ContentType contentType, string password)
	{
		byte[] password2 = ((password == null) ? null : Encoding.UTF8.GetBytes(password));
		return Export(contentType, password2);
	}

	[MonoTODO("X509ContentType.Pfx/Pkcs12 and SerializedCert are not supported. SecureString support is incomplete.")]
	public virtual byte[] Export(X509ContentType contentType, SecureString password)
	{
		byte[] password2 = password?.GetBuffer();
		return Export(contentType, password2);
	}

	internal byte[] Export(X509ContentType contentType, byte[] password)
	{
		try
		{
			X509Helper.ThrowIfContextInvalid(impl);
			return impl.Export(contentType, password);
		}
		finally
		{
			if (password != null)
			{
				Array.Clear(password, 0, password.Length);
			}
		}
	}

	[ComVisible(false)]
	public virtual void Import(byte[] rawData)
	{
		Import(rawData, (string)null, X509KeyStorageFlags.DefaultKeySet);
	}

	[MonoTODO("missing KeyStorageFlags support")]
	[ComVisible(false)]
	public virtual void Import(byte[] rawData, string password, X509KeyStorageFlags keyStorageFlags)
	{
		Reset();
		impl = X509Helper.Import(rawData, password, keyStorageFlags);
	}

	[MonoTODO("SecureString support is incomplete")]
	public virtual void Import(byte[] rawData, SecureString password, X509KeyStorageFlags keyStorageFlags)
	{
		Import(rawData, (string)null, keyStorageFlags);
	}

	[ComVisible(false)]
	public virtual void Import(string fileName)
	{
		byte[] rawData = File.ReadAllBytes(fileName);
		Import(rawData, (string)null, X509KeyStorageFlags.DefaultKeySet);
	}

	[ComVisible(false)]
	[MonoTODO("missing KeyStorageFlags support")]
	public virtual void Import(string fileName, string password, X509KeyStorageFlags keyStorageFlags)
	{
		byte[] rawData = File.ReadAllBytes(fileName);
		Import(rawData, password, keyStorageFlags);
	}

	[MonoTODO("SecureString support is incomplete, missing KeyStorageFlags support")]
	public virtual void Import(string fileName, SecureString password, X509KeyStorageFlags keyStorageFlags)
	{
		byte[] rawData = File.ReadAllBytes(fileName);
		Import(rawData, (string)null, keyStorageFlags);
	}

	void IDeserializationCallback.OnDeserialization(object sender)
	{
	}

	void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
	{
		if (!X509Helper.IsValid(impl))
		{
			throw new NullReferenceException();
		}
		info.AddValue("RawData", impl.GetRawCertData());
	}

	public void Dispose()
	{
		Dispose(disposing: true);
	}

	protected virtual void Dispose(bool disposing)
	{
		if (disposing)
		{
			Reset();
		}
	}

	[ComVisible(false)]
	public virtual void Reset()
	{
		if (impl != null)
		{
			impl.Dispose();
			impl = null;
		}
		issuer_name = null;
		subject_name = null;
		hideDates = false;
	}
}
