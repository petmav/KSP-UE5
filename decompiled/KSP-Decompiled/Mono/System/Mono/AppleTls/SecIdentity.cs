using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Security.Cryptography.X509Certificates;
using Mono.Net;
using ObjCRuntimeInternal;

namespace Mono.AppleTls;

internal class SecIdentity : INativeObject, IDisposable
{
	internal class ImportOptions
	{
		public SecAccess Access { get; set; }

		public SecKeyChain KeyChain { get; set; }
	}

	private static readonly CFString ImportExportPassphase;

	private static readonly CFString ImportItemIdentity;

	private static readonly CFString ImportExportAccess;

	private static readonly CFString ImportExportKeychain;

	internal IntPtr handle;

	public SecCertificate Certificate
	{
		get
		{
			if (handle == IntPtr.Zero)
			{
				throw new ObjectDisposedException("SecIdentity");
			}
			IntPtr certificateRef;
			SecStatusCode secStatusCode = SecIdentityCopyCertificate(handle, out certificateRef);
			if (secStatusCode != 0)
			{
				throw new InvalidOperationException(secStatusCode.ToString());
			}
			return new SecCertificate(certificateRef, owns: true);
		}
	}

	public IntPtr Handle => handle;

	static SecIdentity()
	{
		IntPtr intPtr = CFObject.dlopen("/System/Library/Frameworks/Security.framework/Security", 0);
		if (intPtr == IntPtr.Zero)
		{
			return;
		}
		try
		{
			ImportExportPassphase = CFObject.GetStringConstant(intPtr, "kSecImportExportPassphrase");
			ImportItemIdentity = CFObject.GetStringConstant(intPtr, "kSecImportItemIdentity");
			ImportExportAccess = CFObject.GetStringConstant(intPtr, "kSecImportExportAccess");
			ImportExportKeychain = CFObject.GetStringConstant(intPtr, "kSecImportExportKeychain");
		}
		finally
		{
			CFObject.dlclose(intPtr);
		}
	}

	internal SecIdentity(IntPtr handle, bool owns = false)
	{
		this.handle = handle;
		if (!owns)
		{
			CFObject.CFRetain(handle);
		}
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security", EntryPoint = "SecIdentityGetTypeID")]
	public static extern IntPtr GetTypeID();

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SecStatusCode SecIdentityCopyCertificate(IntPtr identityRef, out IntPtr certificateRef);

	private static CFDictionary CreateImportOptions(CFString password, ImportOptions options = null)
	{
		if (options == null)
		{
			return CFDictionary.FromObjectAndKey(password.Handle, ImportExportPassphase.Handle);
		}
		List<Tuple<IntPtr, IntPtr>> list = new List<Tuple<IntPtr, IntPtr>>();
		list.Add(new Tuple<IntPtr, IntPtr>(ImportExportPassphase.Handle, password.Handle));
		if (options.KeyChain != null)
		{
			list.Add(new Tuple<IntPtr, IntPtr>(ImportExportKeychain.Handle, options.KeyChain.Handle));
		}
		if (options.Access != null)
		{
			list.Add(new Tuple<IntPtr, IntPtr>(ImportExportAccess.Handle, options.Access.Handle));
		}
		return CFDictionary.FromKeysAndObjects(list);
	}

	public static SecIdentity Import(byte[] data, string password, ImportOptions options = null)
	{
		if (data == null)
		{
			throw new ArgumentNullException("data");
		}
		if (string.IsNullOrEmpty(password))
		{
			throw new ArgumentException("password");
		}
		using CFString password2 = CFString.Create(password);
		using CFDictionary options2 = CreateImportOptions(password2, options);
		CFDictionary[] array;
		SecStatusCode secStatusCode = SecImportExport.ImportPkcs12(data, options2, out array);
		if (secStatusCode != 0)
		{
			throw new InvalidOperationException(secStatusCode.ToString());
		}
		return new SecIdentity(array[0].GetValue(ImportItemIdentity.Handle));
	}

	public static SecIdentity Import(X509Certificate2 certificate, ImportOptions options = null)
	{
		if (certificate == null)
		{
			throw new ArgumentNullException("certificate");
		}
		if (!certificate.HasPrivateKey)
		{
			throw new InvalidOperationException("Need X509Certificate2 with a private key.");
		}
		string password = Guid.NewGuid().ToString();
		return Import(certificate.Export(X509ContentType.Pfx, password), password, options);
	}

	~SecIdentity()
	{
		Dispose(disposing: false);
	}

	public void Dispose()
	{
		Dispose(disposing: true);
		GC.SuppressFinalize(this);
	}

	protected virtual void Dispose(bool disposing)
	{
		if (handle != IntPtr.Zero)
		{
			CFObject.CFRelease(handle);
			handle = IntPtr.Zero;
		}
	}
}
