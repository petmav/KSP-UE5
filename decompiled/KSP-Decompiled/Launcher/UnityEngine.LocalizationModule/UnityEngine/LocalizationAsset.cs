using System.Runtime.CompilerServices;
using UnityEngine.Bindings;
using UnityEngine.Scripting.APIUpdating;

namespace UnityEngine;

[MovedFrom("UnityEditor")]
[ExcludeFromPreset]
[NativeClass("LocalizationAsset")]
[NativeHeader("Modules/Localization/Public/LocalizationAsset.bindings.h")]
[NativeHeader("Modules/Localization/Public/LocalizationAsset.h")]
public sealed class LocalizationAsset : Object
{
	public extern string localeIsoCode
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern bool isEditorAsset
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public LocalizationAsset()
	{
		Internal_CreateInstance(this);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("Internal_CreateInstance")]
	private static extern void Internal_CreateInstance([Writable] LocalizationAsset locAsset);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod("StoreLocalizedString")]
	public extern void SetLocalizedString(string original, string localized);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod("GetLocalized")]
	public extern string GetLocalizedString(string original);
}
