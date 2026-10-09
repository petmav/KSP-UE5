using System;
using UnityEngine.Scripting;

namespace UnityEngine.Rendering;

[UsedByNativeCode]
internal struct CoreCameraValues : IEquatable<CoreCameraValues>
{
	private int filterMode;

	private uint cullingMask;

	private int instanceID;

	private int renderImmediateObjects;

	public bool Equals(CoreCameraValues other)
	{
		return filterMode == other.filterMode && cullingMask == other.cullingMask && instanceID == other.instanceID && renderImmediateObjects == other.renderImmediateObjects;
	}

	public override bool Equals(object obj)
	{
		if (obj == null)
		{
			return false;
		}
		return obj is CoreCameraValues && Equals((CoreCameraValues)obj);
	}

	public override int GetHashCode()
	{
		int num = filterMode;
		num = (num * 397) ^ (int)cullingMask;
		num = (num * 397) ^ instanceID;
		return (num * 397) ^ renderImmediateObjects;
	}

	public static bool operator ==(CoreCameraValues left, CoreCameraValues right)
	{
		return left.Equals(right);
	}

	public static bool operator !=(CoreCameraValues left, CoreCameraValues right)
	{
		return !left.Equals(right);
	}
}
