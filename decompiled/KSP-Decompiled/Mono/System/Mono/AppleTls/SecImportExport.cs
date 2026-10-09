using System;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using Mono.Net;
using Mono.Security.Cryptography;

namespace Mono.AppleTls;

internal class SecImportExport
{
	private enum SecExternalFormat
	{
		Unknown = 0,
		OpenSSL = 1,
		X509Cert = 9,
		PEMSequence = 10,
		PKCS7 = 11,
		PKCS12 = 12
	}

	private enum SecExternalItemType
	{
		Unknown,
		PrivateKey,
		PublicKey,
		SessionKey,
		Certificate,
		Aggregate
	}

	private enum SecItemImportExportFlags
	{
		None,
		PemArmour
	}

	private struct SecItemImportExportKeyParameters
	{
		public int version;

		public int flags;

		public IntPtr passphrase;

		private IntPtr alertTitle;

		private IntPtr alertPrompt;

		public IntPtr accessRef;

		private IntPtr keyUsage;

		private IntPtr keyAttributes;
	}

	private const int SEC_KEY_IMPORT_EXPORT_PARAMS_VERSION = 0;

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SecStatusCode SecPKCS12Import(IntPtr pkcs12_data, IntPtr options, out IntPtr items);

	public static SecStatusCode ImportPkcs12(byte[] buffer, CFDictionary options, out CFDictionary[] array)
	{
		using CFData data = CFData.FromData(buffer);
		return ImportPkcs12(data, options, out array);
	}

	public static SecStatusCode ImportPkcs12(CFData data, CFDictionary options, out CFDictionary[] array)
	{
		if (options == null)
		{
			throw new ArgumentNullException("options");
		}
		IntPtr items;
		SecStatusCode result = SecPKCS12Import(data.Handle, options.Handle, out items);
		array = CFArray.ArrayFromHandle(items, (IntPtr h) => new CFDictionary(h, own: false));
		if (items != IntPtr.Zero)
		{
			CFObject.CFRelease(items);
		}
		return result;
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SecStatusCode SecItemImport(IntPtr importedData, IntPtr fileNameOrExtension, ref SecExternalFormat inputFormat, ref SecExternalItemType itemType, SecItemImportExportFlags flags, IntPtr keyParams, IntPtr importKeychain, out IntPtr outItems);

	public static CFArray ItemImport(byte[] buffer, string password)
	{
		using CFData data = CFData.FromData(buffer);
		using CFString cFString = CFString.Create(password);
		SecItemImportExportKeyParameters value = default(SecItemImportExportKeyParameters);
		value.passphrase = cFString.Handle;
		return ItemImport(data, SecExternalFormat.PKCS12, SecExternalItemType.Aggregate, SecItemImportExportFlags.None, value);
	}

	private static CFArray ItemImport(CFData data, SecExternalFormat format, SecExternalItemType itemType, SecItemImportExportFlags flags = SecItemImportExportFlags.None, SecItemImportExportKeyParameters? keyParams = null)
	{
		return ItemImport(data, ref format, ref itemType, flags, keyParams);
	}

	private static CFArray ItemImport(CFData data, ref SecExternalFormat format, ref SecExternalItemType itemType, SecItemImportExportFlags flags = SecItemImportExportFlags.None, SecItemImportExportKeyParameters? keyParams = null)
	{
		IntPtr intPtr = IntPtr.Zero;
		if (keyParams.HasValue)
		{
			intPtr = Marshal.AllocHGlobal(Marshal.SizeOf(keyParams.Value));
			if (intPtr == IntPtr.Zero)
			{
				throw new OutOfMemoryException();
			}
			Marshal.StructureToPtr(keyParams.Value, intPtr, fDeleteOld: false);
		}
		IntPtr outItems;
		SecStatusCode secStatusCode = SecItemImport(data.Handle, IntPtr.Zero, ref format, ref itemType, flags, intPtr, IntPtr.Zero, out outItems);
		if (intPtr != IntPtr.Zero)
		{
			Marshal.FreeHGlobal(intPtr);
		}
		if (secStatusCode != 0)
		{
			throw new NotSupportedException(secStatusCode.ToString());
		}
		return new CFArray(outItems, own: true);
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern IntPtr SecIdentityCreate(IntPtr allocator, IntPtr certificate, IntPtr privateKey);

	public static SecIdentity ItemImport(X509Certificate2 certificate)
	{
		if (!certificate.HasPrivateKey)
		{
			throw new NotSupportedException();
		}
		using SecKey secKey = ImportPrivateKey(certificate);
		using SecCertificate secCertificate = new SecCertificate(certificate);
		IntPtr intPtr = SecIdentityCreate(IntPtr.Zero, secCertificate.Handle, secKey.Handle);
		if (CFType.GetTypeID(intPtr) != SecIdentity.GetTypeID())
		{
			throw new InvalidOperationException();
		}
		return new SecIdentity(intPtr, owns: true);
	}

	private static byte[] ExportKey(RSA key)
	{
		return PKCS8.PrivateKeyInfo.Encode(key);
	}

	private static SecKey ImportPrivateKey(X509Certificate2 certificate)
	{
		if (!certificate.HasPrivateKey)
		{
			throw new NotSupportedException();
		}
		CFArray cFArray;
		using (CFData data = CFData.FromData(ExportKey((RSA)certificate.PrivateKey)))
		{
			cFArray = ItemImport(data, SecExternalFormat.OpenSSL, SecExternalItemType.PrivateKey);
		}
		try
		{
			if (cFArray.Count != 1)
			{
				throw new InvalidOperationException("Private key import failed.");
			}
			IntPtr intPtr = cFArray[0];
			if (CFType.GetTypeID(intPtr) != SecKey.GetTypeID())
			{
				throw new InvalidOperationException("Private key import doesn't return SecKey.");
			}
			return new SecKey(intPtr, cFArray.Handle);
		}
		finally
		{
			cFArray.Dispose();
		}
	}
}
