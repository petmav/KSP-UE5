using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.ConstrainedExecution;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security;
using System.Security.Permissions;
using System.Text;

namespace System;

[Serializable]
[ComVisible(true)]
public struct RuntimeMethodHandle : ISerializable
{
	private IntPtr value;

	public IntPtr Value => value;

	internal RuntimeMethodHandle(IntPtr v)
	{
		value = v;
	}

	private RuntimeMethodHandle(SerializationInfo info, StreamingContext context)
	{
		if (info == null)
		{
			throw new ArgumentNullException("info");
		}
		MonoMethod monoMethod = (MonoMethod)info.GetValue("MethodObj", typeof(MonoMethod));
		value = monoMethod.MethodHandle.Value;
		if (value == IntPtr.Zero)
		{
			throw new SerializationException(Locale.GetText("Insufficient state."));
		}
	}

	[SecurityCritical]
	public void GetObjectData(SerializationInfo info, StreamingContext context)
	{
		if (info == null)
		{
			throw new ArgumentNullException("info");
		}
		if (value == IntPtr.Zero)
		{
			throw new SerializationException("Object fields may not be properly initialized");
		}
		info.AddValue("MethodObj", (MonoMethod)MethodBase.GetMethodFromHandle(this), typeof(MonoMethod));
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern IntPtr GetFunctionPointer(IntPtr m);

	[SecurityPermission(SecurityAction.Demand, UnmanagedCode = true)]
	public IntPtr GetFunctionPointer()
	{
		return GetFunctionPointer(value);
	}

	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
	public override bool Equals(object obj)
	{
		if (obj == null || GetType() != obj.GetType())
		{
			return false;
		}
		return value == ((RuntimeMethodHandle)obj).Value;
	}

	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
	public bool Equals(RuntimeMethodHandle handle)
	{
		return value == handle.Value;
	}

	public override int GetHashCode()
	{
		return value.GetHashCode();
	}

	public static bool operator ==(RuntimeMethodHandle left, RuntimeMethodHandle right)
	{
		return left.Equals(right);
	}

	public static bool operator !=(RuntimeMethodHandle left, RuntimeMethodHandle right)
	{
		return !left.Equals(right);
	}

	internal static string ConstructInstantiation(RuntimeMethodInfo method, TypeNameFormatFlags format)
	{
		StringBuilder stringBuilder = new StringBuilder();
		Type[] genericArguments = method.GetGenericArguments();
		stringBuilder.Append("[");
		for (int i = 0; i < genericArguments.Length; i++)
		{
			if (i > 0)
			{
				stringBuilder.Append(",");
			}
			stringBuilder.Append(genericArguments[i].Name);
		}
		stringBuilder.Append("]");
		return stringBuilder.ToString();
	}

	internal bool IsNullHandle()
	{
		return value == IntPtr.Zero;
	}
}
