using System.IO;
using Mono.Security.X509;

namespace System.Security.Cryptography.X509Certificates;

internal static class X509Helper2
{
	private class MyNativeHelper : INativeCertificateHelper
	{
		public X509CertificateImpl Import(byte[] data, string password, X509KeyStorageFlags flags)
		{
			return X509Helper2.Import(data, password, flags, disableProvider: false);
		}

		public X509CertificateImpl Import(X509Certificate cert)
		{
			return X509Helper2.Import(cert, disableProvider: false);
		}
	}

	internal static long GetSubjectNameHash(X509Certificate certificate)
	{
		return GetSubjectNameHash(certificate.Impl);
	}

	internal static long GetSubjectNameHash(X509CertificateImpl impl)
	{
		using X509Certificate certificate = GetNativeInstance(impl);
		return GetSubjectNameHash(certificate);
	}

	internal static void ExportAsPEM(X509Certificate certificate, Stream stream, bool includeHumanReadableForm)
	{
		ExportAsPEM(certificate.Impl, stream, includeHumanReadableForm);
	}

	internal static void ExportAsPEM(X509CertificateImpl impl, Stream stream, bool includeHumanReadableForm)
	{
		using X509Certificate certificate = GetNativeInstance(impl);
		ExportAsPEM(certificate, stream, includeHumanReadableForm);
	}

	internal static void Initialize()
	{
		X509Helper.InstallNativeHelper(new MyNativeHelper());
	}

	internal static void ThrowIfContextInvalid(X509CertificateImpl impl)
	{
		X509Helper.ThrowIfContextInvalid(impl);
	}

	private static X509Certificate GetNativeInstance(X509CertificateImpl impl)
	{
		throw new PlatformNotSupportedException();
	}

	internal static X509Certificate2Impl Import(byte[] rawData, string password, X509KeyStorageFlags keyStorageFlags, bool disableProvider = false)
	{
		X509Certificate2ImplMono x509Certificate2ImplMono = new X509Certificate2ImplMono();
		x509Certificate2ImplMono.Import(rawData, password, keyStorageFlags);
		return x509Certificate2ImplMono;
	}

	internal static X509Certificate2Impl Import(X509Certificate cert, bool disableProvider = false)
	{
		if (cert.Impl is X509Certificate2Impl x509Certificate2Impl)
		{
			return (X509Certificate2Impl)x509Certificate2Impl.Clone();
		}
		return Import(cert.GetRawCertData(), null, X509KeyStorageFlags.DefaultKeySet);
	}

	[System.MonoTODO("Investigate replacement; see comments in source.")]
	internal static Mono.Security.X509.X509Certificate GetMonoCertificate(X509Certificate2 certificate)
	{
		X509Certificate2Impl x509Certificate2Impl = certificate.Impl;
		if (x509Certificate2Impl == null)
		{
			x509Certificate2Impl = Import(certificate, disableProvider: true);
		}
		return ((x509Certificate2Impl.FallbackImpl as X509Certificate2ImplMono) ?? throw new NotSupportedException()).MonoCertificate;
	}

	internal static X509ChainImpl CreateChainImpl(bool useMachineContext)
	{
		return new X509ChainImplMono(useMachineContext);
	}

	public static bool IsValid(X509ChainImpl impl)
	{
		return impl?.IsValid ?? false;
	}

	internal static void ThrowIfContextInvalid(X509ChainImpl impl)
	{
		if (!IsValid(impl))
		{
			throw GetInvalidChainContextException();
		}
	}

	internal static Exception GetInvalidChainContextException()
	{
		return new CryptographicException(global::Locale.GetText("Chain instance is empty."));
	}
}
