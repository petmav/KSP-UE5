using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security;

namespace System;

[CLSCompliant(false)]
[ComVisible(true)]
public struct TypedReference
{
	private RuntimeTypeHandle type;

	private IntPtr Value;

	private IntPtr Type;

	internal bool IsNull
	{
		get
		{
			if (Value.IsNull())
			{
				return Type.IsNull();
			}
			return false;
		}
	}

	[CLSCompliant(false)]
	[SecurityCritical]
	public static TypedReference MakeTypedReference(object target, FieldInfo[] flds)
	{
		if (target == null)
		{
			throw new ArgumentNullException("target");
		}
		if (flds == null)
		{
			throw new ArgumentNullException("flds");
		}
		if (flds.Length == 0)
		{
			throw new ArgumentException(Environment.GetResourceString("Array must not be of length zero."));
		}
		IntPtr[] array = new IntPtr[flds.Length];
		RuntimeType runtimeType = (RuntimeType)target.GetType();
		for (int i = 0; i < flds.Length; i++)
		{
			RuntimeFieldInfo runtimeFieldInfo = flds[i] as RuntimeFieldInfo;
			if (runtimeFieldInfo == null)
			{
				throw new ArgumentException(Environment.GetResourceString("FieldInfo must be a runtime FieldInfo object."));
			}
			if (runtimeFieldInfo.IsStatic)
			{
				throw new ArgumentException(Environment.GetResourceString("Field in TypedReferences cannot be static or init only."));
			}
			if (runtimeType != runtimeFieldInfo.GetDeclaringTypeInternal() && !runtimeType.IsSubclassOf(runtimeFieldInfo.GetDeclaringTypeInternal()))
			{
				throw new MissingMemberException(Environment.GetResourceString("FieldInfo does not match the target Type."));
			}
			RuntimeType runtimeType2 = (RuntimeType)runtimeFieldInfo.FieldType;
			if (runtimeType2.IsPrimitive)
			{
				throw new ArgumentException(Environment.GetResourceString("TypedReferences cannot be redefined as primitives."));
			}
			if (i < flds.Length - 1 && !runtimeType2.IsValueType)
			{
				throw new MissingMemberException(Environment.GetResourceString("TypedReference can only be made on nested value Types."));
			}
			array[i] = runtimeFieldInfo.FieldHandle.Value;
			runtimeType = runtimeType2;
		}
		return MakeTypedReferenceInternal(target, flds);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SecurityCritical]
	private static extern TypedReference MakeTypedReferenceInternal(object target, FieldInfo[] fields);

	public override int GetHashCode()
	{
		if (Type == IntPtr.Zero)
		{
			return 0;
		}
		return __reftype(this).GetHashCode();
	}

	public override bool Equals(object o)
	{
		throw new NotSupportedException(Environment.GetResourceString("This feature is not currently implemented."));
	}

	[SecuritySafeCritical]
	public unsafe static object ToObject(TypedReference value)
	{
		return InternalToObject(&value);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SecurityCritical]
	internal unsafe static extern object InternalToObject(void* value);

	public static Type GetTargetType(TypedReference value)
	{
		return __reftype(value);
	}

	public static RuntimeTypeHandle TargetTypeToken(TypedReference value)
	{
		return __reftype(value).TypeHandle;
	}

	[SecuritySafeCritical]
	[CLSCompliant(false)]
	public static void SetTypedReference(TypedReference target, object value)
	{
		throw new NotImplementedException("SetTypedReference");
	}
}
