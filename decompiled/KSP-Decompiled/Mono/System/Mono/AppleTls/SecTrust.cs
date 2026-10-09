using System;
using System.Runtime.InteropServices;
using System.Security.Cryptography.X509Certificates;
using Mono.Net;
using ObjCRuntimeInternal;

namespace Mono.AppleTls;

internal class SecTrust : INativeObject, IDisposable
{
	private IntPtr handle;

	public int Count
	{
		get
		{
			if (handle == IntPtr.Zero)
			{
				return 0;
			}
			return (int)SecTrustGetCertificateCount(handle);
		}
	}

	public SecCertificate this[IntPtr index]
	{
		get
		{
			if (handle == IntPtr.Zero)
			{
				throw new ObjectDisposedException("SecTrust");
			}
			if ((long)index < 0 || (long)index >= Count)
			{
				throw new ArgumentOutOfRangeException("index");
			}
			return new SecCertificate(SecTrustGetCertificateAtIndex(handle, index));
		}
	}

	public IntPtr Handle => handle;

	internal SecTrust(IntPtr handle, bool owns = false)
	{
		if (handle == IntPtr.Zero)
		{
			throw new Exception("Invalid handle");
		}
		this.handle = handle;
		if (!owns)
		{
			CFObject.CFRetain(handle);
		}
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SecStatusCode SecTrustCreateWithCertificates(IntPtr certOrCertArray, IntPtr policies, out IntPtr sectrustref);

	public SecTrust(X509CertificateCollection certificates, SecPolicy policy)
	{
		if (certificates == null)
		{
			throw new ArgumentNullException("certificates");
		}
		SecCertificate[] array = new SecCertificate[certificates.Count];
		int num = 0;
		foreach (X509Certificate certificate in certificates)
		{
			array[num++] = new SecCertificate(certificate);
		}
		Initialize(array, policy);
		for (num = 0; num < array.Length; num++)
		{
			array[num].Dispose();
		}
	}

	private void Initialize(SecCertificate[] array, SecPolicy policy)
	{
		using CFArray cFArray = CFArray.CreateArray(array);
		Initialize(cFArray.Handle, policy);
	}

	private void Initialize(IntPtr certHandle, SecPolicy policy)
	{
		SecStatusCode secStatusCode = SecTrustCreateWithCertificates(certHandle, policy?.Handle ?? IntPtr.Zero, out handle);
		if (secStatusCode != 0)
		{
			throw new ArgumentException(secStatusCode.ToString());
		}
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SecStatusCode SecTrustEvaluate(IntPtr trust, out SecTrustResult result);

	public SecTrustResult Evaluate()
	{
		if (handle == IntPtr.Zero)
		{
			throw new ObjectDisposedException("SecTrust");
		}
		SecTrustResult result;
		SecStatusCode secStatusCode = SecTrustEvaluate(handle, out result);
		if (secStatusCode != 0)
		{
			throw new InvalidOperationException(secStatusCode.ToString());
		}
		return result;
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern IntPtr SecTrustGetCertificateCount(IntPtr trust);

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern IntPtr SecTrustGetCertificateAtIndex(IntPtr trust, IntPtr ix);

	internal X509Certificate GetCertificate(int index)
	{
		if (handle == IntPtr.Zero)
		{
			throw new ObjectDisposedException("SecTrust");
		}
		if (index < 0 || index >= Count)
		{
			throw new ArgumentOutOfRangeException("index");
		}
		return new X509Certificate(SecTrustGetCertificateAtIndex(handle, (IntPtr)index));
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SecStatusCode SecTrustSetAnchorCertificates(IntPtr trust, IntPtr anchorCertificates);

	public SecStatusCode SetAnchorCertificates(X509CertificateCollection certificates)
	{
		if (handle == IntPtr.Zero)
		{
			throw new ObjectDisposedException("SecTrust");
		}
		if (certificates == null)
		{
			return SecTrustSetAnchorCertificates(handle, IntPtr.Zero);
		}
		SecCertificate[] array = new SecCertificate[certificates.Count];
		int num = 0;
		foreach (X509Certificate certificate in certificates)
		{
			array[num++] = new SecCertificate(certificate);
		}
		return SetAnchorCertificates(array);
	}

	public SecStatusCode SetAnchorCertificates(SecCertificate[] array)
	{
		if (array == null)
		{
			return SecTrustSetAnchorCertificates(handle, IntPtr.Zero);
		}
		using CFArray cFArray = CFArray.FromNativeObjects(array);
		return SecTrustSetAnchorCertificates(handle, cFArray.Handle);
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SecStatusCode SecTrustSetAnchorCertificatesOnly(IntPtr trust, bool anchorCertificatesOnly);

	public SecStatusCode SetAnchorCertificatesOnly(bool anchorCertificatesOnly)
	{
		if (handle == IntPtr.Zero)
		{
			throw new ObjectDisposedException("SecTrust");
		}
		return SecTrustSetAnchorCertificatesOnly(handle, anchorCertificatesOnly);
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SecStatusCode SecTrustSetVerifyDate(IntPtr trust, IntPtr date);

	public SecStatusCode SetVerifyDate(DateTime date)
	{
		using CFDate cFDate = CFDate.Create(date);
		return SecTrustSetVerifyDate(handle, cFDate.Handle);
	}

	~SecTrust()
	{
		Dispose(disposing: false);
	}

	protected virtual void Dispose(bool disposing)
	{
		if (handle != IntPtr.Zero)
		{
			CFObject.CFRelease(handle);
			handle = IntPtr.Zero;
		}
	}

	public void Dispose()
	{
		Dispose(disposing: true);
		GC.SuppressFinalize(this);
	}
}
