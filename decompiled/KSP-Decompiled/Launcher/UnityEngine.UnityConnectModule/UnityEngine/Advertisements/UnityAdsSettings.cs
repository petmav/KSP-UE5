using System;
using System.Runtime.CompilerServices;
using UnityEngine.Bindings;

namespace UnityEngine.Advertisements;

[NativeHeader("Modules/UnityConnect/UnityAds/UnityAdsSettings.h")]
internal static class UnityAdsSettings
{
	[StaticAccessor("GetUnityAdsSettings()", StaticAccessorType.Dot)]
	public static extern bool enabled
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		[ThreadSafe]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		[ThreadSafe]
		set;
	}

	[StaticAccessor("GetUnityAdsSettings()", StaticAccessorType.Dot)]
	public static extern bool initializeOnStartup
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[StaticAccessor("GetUnityAdsSettings()", StaticAccessorType.Dot)]
	public static extern bool testMode
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[Obsolete("warning No longer supported and will always return true")]
	public static bool IsPlatformEnabled(RuntimePlatform platform)
	{
		return true;
	}

	[Obsolete("warning No longer supported and will do nothing")]
	public static void SetPlatformEnabled(RuntimePlatform platform, bool value)
	{
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[StaticAccessor("GetUnityAdsSettings()", StaticAccessorType.Dot)]
	public static extern string GetGameId(RuntimePlatform platform);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[StaticAccessor("GetUnityAdsSettings()", StaticAccessorType.Dot)]
	public static extern void SetGameId(RuntimePlatform platform, string gameId);
}
