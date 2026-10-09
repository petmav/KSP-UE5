namespace System.Numerics;

internal static class Vector
{
	[JitIntrinsic]
	public static bool IsHardwareAccelerated => false;
}
