using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security;

namespace System.Reflection;

[Serializable]
[ComVisible(true)]
[CLSCompliant(false)]
public sealed class Pointer : ISerializable
{
	[SecurityCritical]
	private unsafe void* _ptr;

	private RuntimeType _ptrType;

	private Pointer()
	{
	}

	[SecurityCritical]
	private unsafe Pointer(SerializationInfo info, StreamingContext context)
	{
		_ptr = ((IntPtr)info.GetValue("_ptr", typeof(IntPtr))).ToPointer();
		_ptrType = (RuntimeType)info.GetValue("_ptrType", typeof(RuntimeType));
	}

	[SecurityCritical]
	public unsafe static object Box(void* ptr, Type type)
	{
		if (type == null)
		{
			throw new ArgumentNullException("type");
		}
		if (!type.IsPointer)
		{
			throw new ArgumentException(Environment.GetResourceString("Type must be a Pointer."), "ptr");
		}
		RuntimeType runtimeType = type as RuntimeType;
		if (runtimeType == null)
		{
			throw new ArgumentException(Environment.GetResourceString("Type must be a Pointer."), "ptr");
		}
		return new Pointer
		{
			_ptr = ptr,
			_ptrType = runtimeType
		};
	}

	[SecurityCritical]
	public unsafe static void* Unbox(object ptr)
	{
		if (!(ptr is Pointer))
		{
			throw new ArgumentException(Environment.GetResourceString("Type must be a Pointer."), "ptr");
		}
		return ((Pointer)ptr)._ptr;
	}

	internal RuntimeType GetPointerType()
	{
		return _ptrType;
	}

	[SecurityCritical]
	internal unsafe object GetPointerValue()
	{
		return (IntPtr)_ptr;
	}

	[SecurityCritical]
	unsafe void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
	{
		info.AddValue("_ptr", new IntPtr(_ptr));
		info.AddValue("_ptrType", _ptrType);
	}
}
