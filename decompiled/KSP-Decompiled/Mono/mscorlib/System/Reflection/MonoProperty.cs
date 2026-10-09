using System.Collections.Generic;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Security;

namespace System.Reflection;

[Serializable]
[StructLayout(LayoutKind.Sequential)]
internal class MonoProperty : RuntimePropertyInfo
{
	private delegate object GetterAdapter(object _this);

	private delegate R Getter<T, R>(T _this);

	private delegate R StaticGetter<R>();

	internal IntPtr klass;

	internal IntPtr prop;

	private MonoPropertyInfo info;

	private PInfo cached;

	private GetterAdapter cached_getter;

	public override PropertyAttributes Attributes
	{
		get
		{
			CachePropertyInfo(PInfo.Attributes);
			return info.attrs;
		}
	}

	public override bool CanRead
	{
		get
		{
			CachePropertyInfo(PInfo.GetMethod);
			return info.get_method != null;
		}
	}

	public override bool CanWrite
	{
		get
		{
			CachePropertyInfo(PInfo.SetMethod);
			return info.set_method != null;
		}
	}

	public override Type PropertyType
	{
		get
		{
			CachePropertyInfo(PInfo.GetMethod | PInfo.SetMethod);
			if (info.get_method != null)
			{
				return info.get_method.ReturnType;
			}
			ParameterInfo[] parametersInternal = info.set_method.GetParametersInternal();
			return parametersInternal[parametersInternal.Length - 1].ParameterType;
		}
	}

	public override Type ReflectedType
	{
		get
		{
			CachePropertyInfo(PInfo.ReflectedType);
			return info.parent;
		}
	}

	public override Type DeclaringType
	{
		get
		{
			CachePropertyInfo(PInfo.DeclaringType);
			return info.declaring_type;
		}
	}

	public override string Name
	{
		get
		{
			CachePropertyInfo(PInfo.Name);
			return info.name;
		}
	}

	private void CachePropertyInfo(PInfo flags)
	{
		if ((cached & flags) != flags)
		{
			MonoPropertyInfo.get_property_info(this, ref info, flags);
			cached |= flags;
		}
	}

	public override MethodInfo[] GetAccessors(bool nonPublic)
	{
		int num = 0;
		int num2 = 0;
		CachePropertyInfo(PInfo.GetMethod | PInfo.SetMethod);
		if (info.set_method != null && (nonPublic || info.set_method.IsPublic))
		{
			num2 = 1;
		}
		if (info.get_method != null && (nonPublic || info.get_method.IsPublic))
		{
			num = 1;
		}
		MethodInfo[] array = new MethodInfo[num + num2];
		int num3 = 0;
		if (num2 != 0)
		{
			array[num3++] = info.set_method;
		}
		if (num != 0)
		{
			array[num3++] = info.get_method;
		}
		return array;
	}

	public override MethodInfo GetGetMethod(bool nonPublic)
	{
		CachePropertyInfo(PInfo.GetMethod);
		if (info.get_method != null && (nonPublic || info.get_method.IsPublic))
		{
			return info.get_method;
		}
		return null;
	}

	public override ParameterInfo[] GetIndexParameters()
	{
		CachePropertyInfo(PInfo.GetMethod | PInfo.SetMethod);
		ParameterInfo[] parametersInternal;
		int num;
		if (info.get_method != null)
		{
			parametersInternal = info.get_method.GetParametersInternal();
			num = parametersInternal.Length;
		}
		else
		{
			if (!(info.set_method != null))
			{
				return EmptyArray<ParameterInfo>.Value;
			}
			parametersInternal = info.set_method.GetParametersInternal();
			num = parametersInternal.Length - 1;
		}
		ParameterInfo[] array = new ParameterInfo[num];
		for (int i = 0; i < num; i++)
		{
			array[i] = ParameterInfo.New(parametersInternal[i], this);
		}
		return array;
	}

	public override MethodInfo GetSetMethod(bool nonPublic)
	{
		CachePropertyInfo(PInfo.SetMethod);
		if (info.set_method != null && (nonPublic || info.set_method.IsPublic))
		{
			return info.set_method;
		}
		return null;
	}

	public override object GetConstantValue()
	{
		return MonoPropertyInfo.get_default_value(this);
	}

	public override object GetRawConstantValue()
	{
		return MonoPropertyInfo.get_default_value(this);
	}

	public override bool IsDefined(Type attributeType, bool inherit)
	{
		return MonoCustomAttrs.IsDefined(this, attributeType, inherit: false);
	}

	public override object[] GetCustomAttributes(bool inherit)
	{
		return MonoCustomAttrs.GetCustomAttributes(this, inherit: false);
	}

	public override object[] GetCustomAttributes(Type attributeType, bool inherit)
	{
		return MonoCustomAttrs.GetCustomAttributes(this, attributeType, inherit: false);
	}

	private static object GetterAdapterFrame<T, R>(Getter<T, R> getter, object obj)
	{
		return getter((T)obj);
	}

	private static object StaticGetterAdapterFrame<R>(StaticGetter<R> getter, object obj)
	{
		return getter();
	}

	private static GetterAdapter CreateGetterDelegate(MethodInfo method)
	{
		Type[] typeArguments;
		Type typeFromHandle;
		string name;
		if (method.IsStatic)
		{
			typeArguments = new Type[1] { method.ReturnType };
			typeFromHandle = typeof(StaticGetter<>);
			name = "StaticGetterAdapterFrame";
		}
		else
		{
			typeArguments = new Type[2] { method.DeclaringType, method.ReturnType };
			typeFromHandle = typeof(Getter<, >);
			name = "GetterAdapterFrame";
		}
		object firstArgument = Delegate.CreateDelegate(typeFromHandle.MakeGenericType(typeArguments), method);
		MethodInfo method2 = typeof(MonoProperty).GetMethod(name, BindingFlags.Static | BindingFlags.NonPublic);
		method2 = method2.MakeGenericMethod(typeArguments);
		return (GetterAdapter)Delegate.CreateDelegate(typeof(GetterAdapter), firstArgument, method2, throwOnBindFailure: true);
	}

	public override object GetValue(object obj, object[] index)
	{
		if (index == null || index.Length == 0)
		{
			if (cached_getter != null)
			{
				try
				{
					return cached_getter(obj);
				}
				catch (Exception inner)
				{
					throw new TargetInvocationException(inner);
				}
			}
			MethodInfo getMethod = GetGetMethod(nonPublic: true);
			if (!DeclaringType.IsValueType && !getMethod.ContainsGenericParameters)
			{
				if (getMethod == null)
				{
					throw new ArgumentException("Get Method not found for '" + Name + "'");
				}
				cached_getter = CreateGetterDelegate(getMethod);
				try
				{
					return cached_getter(obj);
				}
				catch (Exception inner2)
				{
					throw new TargetInvocationException(inner2);
				}
			}
		}
		return GetValue(obj, BindingFlags.Default, null, index, null);
	}

	public override object GetValue(object obj, BindingFlags invokeAttr, Binder binder, object[] index, CultureInfo culture)
	{
		object obj2 = null;
		MethodInfo getMethod = GetGetMethod(nonPublic: true);
		if (getMethod == null)
		{
			throw new ArgumentException("Get Method not found for '" + Name + "'");
		}
		try
		{
			if (index == null || index.Length == 0)
			{
				return getMethod.Invoke(obj, invokeAttr, binder, null, culture);
			}
			return getMethod.Invoke(obj, invokeAttr, binder, index, culture);
		}
		catch (SecurityException inner)
		{
			throw new TargetInvocationException(inner);
		}
	}

	public override void SetValue(object obj, object value, BindingFlags invokeAttr, Binder binder, object[] index, CultureInfo culture)
	{
		MethodInfo setMethod = GetSetMethod(nonPublic: true);
		if (setMethod == null)
		{
			throw new ArgumentException("Set Method not found for '" + Name + "'");
		}
		object[] array;
		if (index == null || index.Length == 0)
		{
			array = new object[1] { value };
		}
		else
		{
			int num = index.Length;
			array = new object[num + 1];
			index.CopyTo(array, 0);
			array[num] = value;
		}
		setMethod.Invoke(obj, invokeAttr, binder, array, culture);
	}

	public override Type[] GetOptionalCustomModifiers()
	{
		Type[] typeModifiers = MonoPropertyInfo.GetTypeModifiers(this, optional: true);
		if (typeModifiers == null)
		{
			return Type.EmptyTypes;
		}
		return typeModifiers;
	}

	public override Type[] GetRequiredCustomModifiers()
	{
		Type[] typeModifiers = MonoPropertyInfo.GetTypeModifiers(this, optional: false);
		if (typeModifiers == null)
		{
			return Type.EmptyTypes;
		}
		return typeModifiers;
	}

	public override IList<CustomAttributeData> GetCustomAttributesData()
	{
		return CustomAttributeData.GetCustomAttributes(this);
	}
}
