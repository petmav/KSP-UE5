using System.Runtime.CompilerServices;
using System.Security;

namespace System;

[FriendAccessAllowed]
internal class CLRConfig
{
	[FriendAccessAllowed]
	[SuppressUnmanagedCodeSecurity]
	[SecurityCritical]
	internal static bool CheckLegacyManagedDeflateStream()
	{
		return false;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SuppressUnmanagedCodeSecurity]
	[SecurityCritical]
	internal static extern bool CheckThrowUnobservedTaskExceptions();
}
