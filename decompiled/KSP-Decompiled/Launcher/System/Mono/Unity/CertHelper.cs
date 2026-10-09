using System;
using System.Security.Cryptography.X509Certificates;

namespace Mono.Unity;

internal static class CertHelper
{
	public unsafe static void AddCertificatesToNativeChain(UnityTls.unitytls_x509list* nativeCertificateChain, X509CertificateCollection certificates, UnityTls.unitytls_errorstate* errorState)
	{
		foreach (X509Certificate certificate in certificates)
		{
			AddCertificateToNativeChain(nativeCertificateChain, certificate, errorState);
		}
	}

	public unsafe static void AddCertificateToNativeChain(UnityTls.unitytls_x509list* nativeCertificateChain, X509Certificate certificate, UnityTls.unitytls_errorstate* errorState)
	{
		byte[] rawCertData = certificate.GetRawCertData();
		fixed (byte* buffer = rawCertData)
		{
			UnityTls.NativeInterface.unitytls_x509list_append_der(nativeCertificateChain, buffer, (IntPtr)rawCertData.Length, errorState);
		}
		if (certificate.Impl is X509Certificate2Impl { IntermediateCertificates: { Count: >0 } intermediateCertificates })
		{
			for (int i = 0; i < intermediateCertificates.Count; i++)
			{
				AddCertificateToNativeChain(nativeCertificateChain, new X509Certificate(intermediateCertificates[i]), errorState);
			}
		}
	}

	public unsafe static X509CertificateCollection NativeChainToManagedCollection(UnityTls.unitytls_x509list_ref nativeCertificateChain, UnityTls.unitytls_errorstate* errorState)
	{
		X509CertificateCollection x509CertificateCollection = new X509CertificateCollection();
		UnityTls.unitytls_x509_ref cert = UnityTls.NativeInterface.unitytls_x509list_get_x509(nativeCertificateChain, (IntPtr)0, errorState);
		int num = 0;
		while (cert.handle != UnityTls.NativeInterface.UNITYTLS_INVALID_HANDLE)
		{
			IntPtr intPtr = UnityTls.NativeInterface.unitytls_x509_export_der(cert, null, (IntPtr)0, errorState);
			byte[] array = new byte[(int)intPtr];
			fixed (byte* buffer = array)
			{
				UnityTls.NativeInterface.unitytls_x509_export_der(cert, buffer, intPtr, errorState);
			}
			x509CertificateCollection.Add(new X509Certificate(array));
			cert = UnityTls.NativeInterface.unitytls_x509list_get_x509(nativeCertificateChain, (IntPtr)num, errorState);
			num++;
		}
		return x509CertificateCollection;
	}
}
