using System.Runtime.CompilerServices;
using System.Security;

namespace System;

[FriendAccessAllowed]
internal class CLRConfig
{
	[SecurityCritical]
	[FriendAccessAllowed]
	[SuppressUnmanagedCodeSecurity]
	internal static bool CheckLegacyManagedDeflateStream()
	{
		return false;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SuppressUnmanagedCodeSecurity]
	[SecurityCritical]
	internal static extern bool CheckThrowUnobservedTaskExceptions();
}
