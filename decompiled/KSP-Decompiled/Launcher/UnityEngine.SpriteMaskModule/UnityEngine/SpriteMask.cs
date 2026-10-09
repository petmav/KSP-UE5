using System.Runtime.CompilerServices;
using UnityEngine.Bindings;

namespace UnityEngine;

[NativeType(Header = "Modules/SpriteMask/Public/SpriteMask.h")]
[RejectDragAndDropMaterial]
public sealed class SpriteMask : Renderer
{
	public extern int frontSortingLayerID
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern int frontSortingOrder
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern int backSortingLayerID
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern int backSortingOrder
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern float alphaCutoff
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern Sprite sprite
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern bool isCustomRangeActive
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		[NativeMethod("IsCustomRangeActive")]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		[NativeMethod("SetCustomRangeActive")]
		set;
	}

	public extern SpriteSortPoint spriteSortPoint
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	internal Bounds GetSpriteBounds()
	{
		GetSpriteBounds_Injected(out var ret);
		return ret;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern void GetSpriteBounds_Injected(out Bounds ret);
}
