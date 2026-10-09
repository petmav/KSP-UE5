using System;
using Mono.Net;

namespace Mono.AppleTls;

internal class SecRecord : IDisposable
{
	internal static readonly IntPtr SecClassKey;

	private CFMutableDictionary _queryDict;

	internal CFMutableDictionary QueryDict => _queryDict;

	static SecRecord()
	{
		IntPtr intPtr = CFObject.dlopen("/System/Library/Frameworks/Security.framework/Security", 0);
		if (intPtr == IntPtr.Zero)
		{
			return;
		}
		try
		{
			SecClassKey = CFObject.GetIntPtr(intPtr, "kSecClass");
		}
		finally
		{
			CFObject.dlclose(intPtr);
		}
	}

	internal void SetValue(IntPtr key, IntPtr value)
	{
		_queryDict.SetValue(key, value);
	}

	public SecRecord(SecKind secKind)
	{
		IntPtr val = SecClass.FromSecKind(secKind);
		_queryDict = CFMutableDictionary.Create();
		_queryDict.SetValue(SecClassKey, val);
	}

	public void Dispose()
	{
		Dispose(disposing: true);
		GC.SuppressFinalize(this);
	}

	protected virtual void Dispose(bool disposing)
	{
		if (_queryDict != null && disposing)
		{
			_queryDict.Dispose();
			_queryDict = null;
		}
	}

	~SecRecord()
	{
		Dispose(disposing: false);
	}
}
