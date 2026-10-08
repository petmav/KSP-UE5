using System;
using System.Runtime.CompilerServices;

namespace Mono;

public static class Runtime
{
	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern void mono_runtime_install_handlers();

	public static void InstallSignalHandlers()
	{
		mono_runtime_install_handlers();
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern void mono_runtime_cleanup_handlers();

	public static void RemoveSignalHandlers()
	{
		mono_runtime_cleanup_handlers();
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	public static extern string GetDisplayName();

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern string GetNativeStackTrace(Exception exception);

	public static bool SetGCAllowSynchronousMajor(bool flag)
	{
		return true;
	}
}
