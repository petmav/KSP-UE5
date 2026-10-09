using System.Runtime.CompilerServices;

namespace System;

internal static class AppContextSwitches
{
	private static int _noAsyncCurrentCulture;

	private static int _throwExceptionIfDisposedCancellationTokenSource;

	private static int _preserveEventListnerObjectIdentity;

	private static int _useLegacyPathHandling;

	private static int _blockLongPaths;

	private static int _cloneActor;

	private static int _doNotAddrOfCspParentWindowHandle;

	public static bool NoAsyncCurrentCulture
	{
		[MethodImpl(MethodImplOptions.AggressiveInlining)]
		get
		{
			return GetCachedSwitchValue("Switch.System.Globalization.NoAsyncCurrentCulture", ref _noAsyncCurrentCulture);
		}
	}

	public static bool ThrowExceptionIfDisposedCancellationTokenSource
	{
		[MethodImpl(MethodImplOptions.AggressiveInlining)]
		get
		{
			return GetCachedSwitchValue("Switch.System.Threading.ThrowExceptionIfDisposedCancellationTokenSource", ref _throwExceptionIfDisposedCancellationTokenSource);
		}
	}

	public static bool PreserveEventListnerObjectIdentity
	{
		[MethodImpl(MethodImplOptions.AggressiveInlining)]
		get
		{
			return GetCachedSwitchValue("Switch.System.Diagnostics.EventSource.PreserveEventListnerObjectIdentity", ref _preserveEventListnerObjectIdentity);
		}
	}

	public static bool UseLegacyPathHandling
	{
		[MethodImpl(MethodImplOptions.AggressiveInlining)]
		get
		{
			return GetCachedSwitchValue("Switch.System.IO.UseLegacyPathHandling", ref _useLegacyPathHandling);
		}
	}

	public static bool BlockLongPaths
	{
		[MethodImpl(MethodImplOptions.AggressiveInlining)]
		get
		{
			return GetCachedSwitchValue("Switch.System.IO.BlockLongPaths", ref _blockLongPaths);
		}
	}

	public static bool SetActorAsReferenceWhenCopyingClaimsIdentity
	{
		[MethodImpl(MethodImplOptions.AggressiveInlining)]
		get
		{
			return GetCachedSwitchValue("Switch.System.Security.ClaimsIdentity.SetActorAsReferenceWhenCopyingClaimsIdentity", ref _cloneActor);
		}
	}

	public static bool DoNotAddrOfCspParentWindowHandle
	{
		[MethodImpl(MethodImplOptions.AggressiveInlining)]
		get
		{
			return GetCachedSwitchValue("Switch.System.Security.Cryptography.DoNotAddrOfCspParentWindowHandle", ref _doNotAddrOfCspParentWindowHandle);
		}
	}

	private static bool DisableCaching { get; set; }

	static AppContextSwitches()
	{
		if (AppContext.TryGetSwitch("TestSwitch.LocalAppContext.DisableCaching", out var isEnabled))
		{
			DisableCaching = isEnabled;
		}
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	internal static bool GetCachedSwitchValue(string switchName, ref int switchValue)
	{
		if (switchValue < 0)
		{
			return false;
		}
		if (switchValue > 0)
		{
			return true;
		}
		return GetCachedSwitchValueInternal(switchName, ref switchValue);
	}

	private static bool GetCachedSwitchValueInternal(string switchName, ref int switchValue)
	{
		AppContext.TryGetSwitch(switchName, out var isEnabled);
		if (DisableCaching)
		{
			return isEnabled;
		}
		switchValue = (isEnabled ? 1 : (-1));
		return isEnabled;
	}
}
