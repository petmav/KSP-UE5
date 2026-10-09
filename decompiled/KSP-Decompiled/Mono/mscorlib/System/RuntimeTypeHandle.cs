using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.ConstrainedExecution;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security;

namespace System;

[Serializable]
[ComVisible(true)]
public struct RuntimeTypeHandle : ISerializable
{
	private IntPtr value;

	public IntPtr Value => value;

	internal RuntimeTypeHandle(IntPtr val)
	{
		value = val;
	}

	internal RuntimeTypeHandle(RuntimeType type)
		: this(type._impl.value)
	{
	}

	private RuntimeTypeHandle(SerializationInfo info, StreamingContext context)
	{
		if (info == null)
		{
			throw new ArgumentNullException("info");
		}
		RuntimeType runtimeType = (RuntimeType)info.GetValue("TypeObj", typeof(RuntimeType));
		value = runtimeType.TypeHandle.Value;
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
		info.AddValue("TypeObj", Type.GetTypeHandle(this), typeof(RuntimeType));
	}

	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
	public override bool Equals(object obj)
	{
		if (obj == null || GetType() != obj.GetType())
		{
			return false;
		}
		return value == ((RuntimeTypeHandle)obj).Value;
	}

	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
	public bool Equals(RuntimeTypeHandle handle)
	{
		return value == handle.Value;
	}

	public override int GetHashCode()
	{
		return value.GetHashCode();
	}

	public static bool operator ==(RuntimeTypeHandle left, object right)
	{
		if (right != null && right is RuntimeTypeHandle)
		{
			return left.Equals((RuntimeTypeHandle)right);
		}
		return false;
	}

	public static bool operator !=(RuntimeTypeHandle left, object right)
	{
		if (right != null && right is RuntimeTypeHandle)
		{
			return !left.Equals((RuntimeTypeHandle)right);
		}
		return true;
	}

	public static bool operator ==(object left, RuntimeTypeHandle right)
	{
		if (left != null && left is RuntimeTypeHandle runtimeTypeHandle)
		{
			return runtimeTypeHandle.Equals(right);
		}
		return false;
	}

	public static bool operator !=(object left, RuntimeTypeHandle right)
	{
		if (left != null && left is RuntimeTypeHandle runtimeTypeHandle)
		{
			return !runtimeTypeHandle.Equals(right);
		}
		return true;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern TypeAttributes GetAttributes(RuntimeType type);

	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
	[CLSCompliant(false)]
	public ModuleHandle GetModuleHandle()
	{
		if (value == IntPtr.Zero)
		{
			throw new InvalidOperationException("Object fields may not be properly initialized");
		}
		return Type.GetTypeFromHandle(this).Module.ModuleHandle;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern int GetMetadataToken(RuntimeType type);

	internal static int GetToken(RuntimeType type)
	{
		return GetMetadataToken(type);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern Type GetGenericTypeDefinition_impl(RuntimeType type);

	internal static Type GetGenericTypeDefinition(RuntimeType type)
	{
		return GetGenericTypeDefinition_impl(type);
	}

	internal static bool HasElementType(RuntimeType type)
	{
		if (!IsArray(type) && !IsByRef(type))
		{
			return IsPointer(type);
		}
		return true;
	}

	internal static bool HasProxyAttribute(RuntimeType type)
	{
		throw new NotImplementedException("HasProxyAttribute");
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern bool HasInstantiation(RuntimeType type);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern bool IsArray(RuntimeType type);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern bool IsByRef(RuntimeType type);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern bool IsComObject(RuntimeType type);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern bool IsInstanceOfType(RuntimeType type, object o);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern bool IsPointer(RuntimeType type);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern bool IsPrimitive(RuntimeType type);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern bool HasReferences(RuntimeType type);

	internal static bool IsComObject(RuntimeType type, bool isGenericCOM)
	{
		if (!isGenericCOM)
		{
			return IsComObject(type);
		}
		return false;
	}

	internal static bool IsContextful(RuntimeType type)
	{
		return typeof(ContextBoundObject).IsAssignableFrom(type);
	}

	internal static bool IsEquivalentTo(RuntimeType rtType1, RuntimeType rtType2)
	{
		return false;
	}

	internal static bool IsSzArray(RuntimeType type)
	{
		if (IsArray(type))
		{
			return type.GetArrayRank() == 1;
		}
		return false;
	}

	internal static bool IsInterface(RuntimeType type)
	{
		return (type.Attributes & TypeAttributes.ClassSemanticsMask) == TypeAttributes.ClassSemanticsMask;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern int GetArrayRank(RuntimeType type);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern RuntimeAssembly GetAssembly(RuntimeType type);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern RuntimeType GetElementType(RuntimeType type);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern RuntimeModule GetModule(RuntimeType type);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern bool IsGenericVariable(RuntimeType type);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern RuntimeType GetBaseType(RuntimeType type);

	internal static bool CanCastTo(RuntimeType type, RuntimeType target)
	{
		return type_is_assignable_from(target, type);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern bool type_is_assignable_from(Type a, Type b);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern bool IsGenericTypeDefinition(RuntimeType type);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern IntPtr GetGenericParameterInfo(RuntimeType type);
}
