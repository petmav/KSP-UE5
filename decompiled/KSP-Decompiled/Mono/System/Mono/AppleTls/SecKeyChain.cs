using System;
using System.Runtime.InteropServices;
using Mono.Net;
using ObjCRuntimeInternal;

namespace Mono.AppleTls;

internal class SecKeyChain : INativeObject, IDisposable
{
	internal static readonly IntPtr MatchLimitAll;

	internal static readonly IntPtr MatchLimitOne;

	internal static readonly IntPtr MatchLimit;

	private IntPtr handle;

	public IntPtr Handle => handle;

	internal SecKeyChain(IntPtr handle, bool owns = false)
	{
		if (handle == IntPtr.Zero)
		{
			throw new ArgumentException("Invalid handle");
		}
		this.handle = handle;
		if (!owns)
		{
			CFObject.CFRetain(handle);
		}
	}

	static SecKeyChain()
	{
		IntPtr intPtr = CFObject.dlopen("/System/Library/Frameworks/Security.framework/Security", 0);
		if (intPtr == IntPtr.Zero)
		{
			return;
		}
		try
		{
			MatchLimit = CFObject.GetIntPtr(intPtr, "kSecMatchLimit");
			MatchLimitAll = CFObject.GetIntPtr(intPtr, "kSecMatchLimitAll");
			MatchLimitOne = CFObject.GetIntPtr(intPtr, "kSecMatchLimitOne");
		}
		finally
		{
			CFObject.dlclose(intPtr);
		}
	}

	public static SecIdentity FindIdentity(SecCertificate certificate, bool throwOnError = false)
	{
		if (certificate == null)
		{
			throw new ArgumentNullException("certificate");
		}
		SecIdentity secIdentity = FindIdentity((SecCertificate cert) => SecCertificate.Equals(certificate, cert));
		if (!throwOnError || secIdentity != null)
		{
			return secIdentity;
		}
		throw new InvalidOperationException($"Could not find SecIdentity for certificate '{certificate.SubjectSummary}' in keychain.");
	}

	private static SecIdentity FindIdentity(Predicate<SecCertificate> filter)
	{
		using (SecRecord query = new SecRecord(SecKind.Identity))
		{
			SecStatusCode result;
			INativeObject[] array = QueryAsReference(query, -1, out result);
			if (result != 0 || array == null)
			{
				return null;
			}
			for (int i = 0; i < array.Length; i++)
			{
				SecIdentity secIdentity = (SecIdentity)array[i];
				if (filter(secIdentity.Certificate))
				{
					return secIdentity;
				}
			}
		}
		return null;
	}

	private static INativeObject[] QueryAsReference(SecRecord query, int max, out SecStatusCode result)
	{
		if (query == null)
		{
			result = SecStatusCode.Param;
			return null;
		}
		using CFMutableDictionary cFMutableDictionary = query.QueryDict.MutableCopy();
		cFMutableDictionary.SetValue(CFBoolean.True.Handle, SecItem.ReturnRef);
		SetLimit(cFMutableDictionary, max);
		return QueryAsReference(cFMutableDictionary, out result);
	}

	private static INativeObject[] QueryAsReference(CFDictionary query, out SecStatusCode result)
	{
		if (query == null)
		{
			result = SecStatusCode.Param;
			return null;
		}
		result = SecItem.SecItemCopyMatching(query.Handle, out var result2);
		if (result == SecStatusCode.Success && result2 != IntPtr.Zero)
		{
			return CFArray.ArrayFromHandle(result2, (Func<IntPtr, INativeObject>)delegate(IntPtr p)
			{
				IntPtr typeID = CFType.GetTypeID(p);
				if (typeID == SecCertificate.GetTypeID())
				{
					return new SecCertificate(p, owns: true);
				}
				if (typeID == SecKey.GetTypeID())
				{
					return new SecKey(p, owns: true);
				}
				if (typeID == SecIdentity.GetTypeID())
				{
					return new SecIdentity(p, owns: true);
				}
				throw new Exception($"Unexpected type: 0x{typeID:x}");
			});
		}
		return null;
	}

	internal static CFNumber SetLimit(CFMutableDictionary dict, int max)
	{
		CFNumber cFNumber = null;
		IntPtr key;
		switch (max)
		{
		case -1:
			key = MatchLimitAll;
			break;
		case 1:
			key = MatchLimitOne;
			break;
		default:
			cFNumber = CFNumber.FromInt32(max);
			key = cFNumber.Handle;
			break;
		}
		dict.SetValue(key, MatchLimit);
		return cFNumber;
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SecStatusCode SecKeychainCreate(IntPtr pathName, uint passwordLength, IntPtr password, bool promptUser, IntPtr initialAccess, out IntPtr keychain);

	internal static SecKeyChain Create(string pathName, string password)
	{
		IntPtr keychain;
		SecStatusCode secStatusCode = SecKeychainCreate(Marshal.StringToHGlobalAnsi(pathName), password: Marshal.StringToHGlobalAnsi(password), passwordLength: (uint)password.Length, promptUser: false, initialAccess: IntPtr.Zero, keychain: out keychain);
		if (secStatusCode != 0)
		{
			throw new InvalidOperationException(secStatusCode.ToString());
		}
		return new SecKeyChain(keychain, owns: true);
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SecStatusCode SecKeychainOpen(IntPtr pathName, out IntPtr keychain);

	internal static SecKeyChain Open(string pathName)
	{
		IntPtr intPtr = IntPtr.Zero;
		try
		{
			intPtr = Marshal.StringToHGlobalAnsi(pathName);
			IntPtr keychain;
			SecStatusCode secStatusCode = SecKeychainOpen(intPtr, out keychain);
			if (secStatusCode != 0)
			{
				throw new InvalidOperationException(secStatusCode.ToString());
			}
			return new SecKeyChain(keychain, owns: true);
		}
		finally
		{
			if (intPtr != IntPtr.Zero)
			{
				Marshal.FreeHGlobal(intPtr);
			}
		}
	}

	internal static SecKeyChain OpenSystemRootCertificates()
	{
		return Open("/System/Library/Keychains/SystemRootCertificates.keychain");
	}

	~SecKeyChain()
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
