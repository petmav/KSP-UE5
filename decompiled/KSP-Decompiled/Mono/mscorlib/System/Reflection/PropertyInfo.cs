using System.Diagnostics;
using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Mono;

namespace System.Reflection;

[Serializable]
[ComDefaultInterface(typeof(_PropertyInfo))]
[ClassInterface(ClassInterfaceType.None)]
[ComVisible(true)]
public abstract class PropertyInfo : MemberInfo, _PropertyInfo
{
	public abstract PropertyAttributes Attributes { get; }

	public abstract bool CanRead { get; }

	public abstract bool CanWrite { get; }

	public virtual MethodInfo GetMethod => GetGetMethod(nonPublic: true);

	public virtual MethodInfo SetMethod => GetSetMethod(nonPublic: true);

	public bool IsSpecialName => (Attributes & PropertyAttributes.SpecialName) != 0;

	public override MemberTypes MemberType => MemberTypes.Property;

	public abstract Type PropertyType { get; }

	public MethodInfo[] GetAccessors()
	{
		return GetAccessors(nonPublic: false);
	}

	public abstract MethodInfo[] GetAccessors(bool nonPublic);

	public MethodInfo GetGetMethod()
	{
		return GetGetMethod(nonPublic: false);
	}

	public abstract MethodInfo GetGetMethod(bool nonPublic);

	public abstract ParameterInfo[] GetIndexParameters();

	public MethodInfo GetSetMethod()
	{
		return GetSetMethod(nonPublic: false);
	}

	public abstract MethodInfo GetSetMethod(bool nonPublic);

	[DebuggerStepThrough]
	[DebuggerHidden]
	public virtual object GetValue(object obj, object[] index)
	{
		return GetValue(obj, BindingFlags.Default, null, index, null);
	}

	[DebuggerStepThrough]
	[DebuggerHidden]
	public object GetValue(object obj)
	{
		return GetValue(obj, BindingFlags.Default, null, null, null);
	}

	public abstract object GetValue(object obj, BindingFlags invokeAttr, Binder binder, object[] index, CultureInfo culture);

	[DebuggerStepThrough]
	[DebuggerHidden]
	public virtual void SetValue(object obj, object value, object[] index)
	{
		SetValue(obj, value, BindingFlags.Default, null, index, null);
	}

	[DebuggerStepThrough]
	[DebuggerHidden]
	public void SetValue(object obj, object value)
	{
		SetValue(obj, value, BindingFlags.Default, null, null, null);
	}

	public abstract void SetValue(object obj, object value, BindingFlags invokeAttr, Binder binder, object[] index, CultureInfo culture);

	public virtual Type[] GetOptionalCustomModifiers()
	{
		return Type.EmptyTypes;
	}

	public virtual Type[] GetRequiredCustomModifiers()
	{
		return Type.EmptyTypes;
	}

	private static NotImplementedException CreateNIE()
	{
		return new NotImplementedException();
	}

	public virtual object GetConstantValue()
	{
		throw CreateNIE();
	}

	public virtual object GetRawConstantValue()
	{
		throw CreateNIE();
	}

	public override bool Equals(object obj)
	{
		return obj == this;
	}

	public override int GetHashCode()
	{
		return base.GetHashCode();
	}

	public static bool operator ==(PropertyInfo left, PropertyInfo right)
	{
		if ((object)left == right)
		{
			return true;
		}
		if (((object)left == null) ^ ((object)right == null))
		{
			return false;
		}
		return left.Equals(right);
	}

	public static bool operator !=(PropertyInfo left, PropertyInfo right)
	{
		if ((object)left == right)
		{
			return false;
		}
		if (((object)left == null) ^ ((object)right == null))
		{
			return true;
		}
		return !left.Equals(right);
	}

	void _PropertyInfo.GetIDsOfNames([In] ref Guid riid, IntPtr rgszNames, uint cNames, uint lcid, IntPtr rgDispId)
	{
		throw new NotImplementedException();
	}

	Type _PropertyInfo.GetType()
	{
		return GetType();
	}

	void _PropertyInfo.GetTypeInfo(uint iTInfo, uint lcid, IntPtr ppTInfo)
	{
		throw new NotImplementedException();
	}

	void _PropertyInfo.GetTypeInfoCount(out uint pcTInfo)
	{
		throw new NotImplementedException();
	}

	void _PropertyInfo.Invoke(uint dispIdMember, [In] ref Guid riid, uint lcid, short wFlags, IntPtr pDispParams, IntPtr pVarResult, IntPtr pExcepInfo, IntPtr puArgErr)
	{
		throw new NotImplementedException();
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern PropertyInfo internal_from_handle_type(IntPtr event_handle, IntPtr type_handle);

	internal static PropertyInfo GetPropertyFromHandle(RuntimePropertyHandle handle, RuntimeTypeHandle reflectedType)
	{
		if (handle.Value == IntPtr.Zero)
		{
			throw new ArgumentException("The handle is invalid.");
		}
		PropertyInfo propertyInfo = internal_from_handle_type(handle.Value, reflectedType.Value);
		if (propertyInfo == null)
		{
			throw new ArgumentException("The property handle and the type handle are incompatible.");
		}
		return propertyInfo;
	}
}
