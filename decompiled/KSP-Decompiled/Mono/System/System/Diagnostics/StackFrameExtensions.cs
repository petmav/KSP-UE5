using System.ComponentModel;
using Unity;

namespace System.Diagnostics;

[EditorBrowsable(EditorBrowsableState.Never)]
public static class StackFrameExtensions
{
	[EditorBrowsable(EditorBrowsableState.Never)]
	public static IntPtr GetNativeImageBase(this StackFrame stackFrame)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(IntPtr);
	}

	[EditorBrowsable(EditorBrowsableState.Never)]
	public static IntPtr GetNativeIP(this StackFrame stackFrame)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(IntPtr);
	}

	[EditorBrowsable(EditorBrowsableState.Never)]
	public static bool HasILOffset(this StackFrame stackFrame)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	[EditorBrowsable(EditorBrowsableState.Never)]
	public static bool HasMethod(this StackFrame stackFrame)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	[EditorBrowsable(EditorBrowsableState.Never)]
	public static bool HasNativeImage(this StackFrame stackFrame)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	[EditorBrowsable(EditorBrowsableState.Never)]
	public static bool HasSource(this StackFrame stackFrame)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}
