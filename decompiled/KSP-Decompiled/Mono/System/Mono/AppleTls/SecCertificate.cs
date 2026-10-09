using System;
using System.Runtime.InteropServices;
using System.Security.Cryptography.X509Certificates;
using Mono.Net;
using ObjCRuntimeInternal;

namespace Mono.AppleTls;

internal class SecCertificate : INativeObject, IDisposable
{
	internal IntPtr handle;

	public string SubjectSummary
	{
		get
		{
			if (handle == IntPtr.Zero)
			{
				throw new ObjectDisposedException("SecCertificate");
			}
			IntPtr intPtr = IntPtr.Zero;
			try
			{
				intPtr = SecCertificateCopySubjectSummary(handle);
				return (CFString)CFString.AsString(intPtr);
			}
			finally
			{
				if (intPtr != IntPtr.Zero)
				{
					CFObject.CFRelease(intPtr);
				}
			}
		}
	}

	public CFData DerData
	{
		get
		{
			if (handle == IntPtr.Zero)
			{
				throw new ObjectDisposedException("SecCertificate");
			}
			IntPtr intPtr = SecCertificateCopyData(handle);
			if (intPtr == IntPtr.Zero)
			{
				throw new ArgumentException("Not a valid certificate");
			}
			return new CFData(intPtr, own: true);
		}
	}

	public IntPtr Handle => handle;

	internal SecCertificate(IntPtr handle, bool owns = false)
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

	[DllImport("/System/Library/Frameworks/Security.framework/Security", EntryPoint = "SecCertificateGetTypeID")]
	public static extern IntPtr GetTypeID();

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern IntPtr SecCertificateCreateWithData(IntPtr allocator, IntPtr cfData);

	public SecCertificate(X509Certificate certificate)
	{
		if (certificate == null)
		{
			throw new ArgumentNullException("certificate");
		}
		handle = certificate.Impl.GetNativeAppleCertificate();
		if (handle != IntPtr.Zero)
		{
			CFObject.CFRetain(handle);
			return;
		}
		using CFData data = CFData.FromData(certificate.GetRawCertData());
		Initialize(data);
	}

	internal SecCertificate(X509CertificateImpl impl)
	{
		handle = impl.GetNativeAppleCertificate();
		if (handle != IntPtr.Zero)
		{
			CFObject.CFRetain(handle);
			return;
		}
		using CFData data = CFData.FromData(impl.GetRawCertData());
		Initialize(data);
	}

	private void Initialize(CFData data)
	{
		handle = SecCertificateCreateWithData(IntPtr.Zero, data.Handle);
		if (handle == IntPtr.Zero)
		{
			throw new ArgumentException("Not a valid DER-encoded X.509 certificate");
		}
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern IntPtr SecCertificateCopySubjectSummary(IntPtr cert);

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern IntPtr SecCertificateCopyData(IntPtr cert);

	public X509Certificate ToX509Certificate()
	{
		if (handle == IntPtr.Zero)
		{
			throw new ObjectDisposedException("SecCertificate");
		}
		return new X509Certificate(handle);
	}

	internal static bool Equals(SecCertificate first, SecCertificate second)
	{
		if (first == null)
		{
			throw new ArgumentNullException("first");
		}
		if (second == null)
		{
			throw new ArgumentNullException("second");
		}
		if (first.Handle == second.Handle)
		{
			return true;
		}
		using CFData cFData = first.DerData;
		using CFData cFData2 = second.DerData;
		if (cFData.Handle == cFData2.Handle)
		{
			return true;
		}
		if (cFData.Length != cFData2.Length)
		{
			return false;
		}
		IntPtr length = cFData.Length;
		for (long num = 0L; num < (long)length; num++)
		{
			if (cFData[num] != cFData2[num])
			{
				return false;
			}
		}
		return true;
	}

	~SecCertificate()
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
