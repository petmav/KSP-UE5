using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;

namespace System.Reflection;

[Serializable]
[StructLayout(LayoutKind.Sequential)]
internal class MonoCMethod : RuntimeConstructorInfo
{
	internal IntPtr mhandle;

	private string name;

	private Type reftype;

	public override RuntimeMethodHandle MethodHandle => new RuntimeMethodHandle(mhandle);

	public override MethodAttributes Attributes => MonoMethodInfo.GetAttributes(mhandle);

	public override CallingConventions CallingConvention => MonoMethodInfo.GetCallingConvention(mhandle);

	public override bool ContainsGenericParameters => DeclaringType.ContainsGenericParameters;

	public override Type ReflectedType => reftype;

	public override Type DeclaringType => MonoMethodInfo.GetDeclaringType(mhandle);

	public override string Name
	{
		get
		{
			if (name != null)
			{
				return name;
			}
			return MonoMethod.get_name(this);
		}
	}

	public override bool IsSecurityTransparent => get_core_clr_security_level() == 0;

	public override bool IsSecurityCritical => get_core_clr_security_level() > 0;

	public override bool IsSecuritySafeCritical => get_core_clr_security_level() == 1;

	public override MethodImplAttributes GetMethodImplementationFlags()
	{
		return MonoMethodInfo.GetMethodImplementationFlags(mhandle);
	}

	public override ParameterInfo[] GetParameters()
	{
		return MonoMethodInfo.GetParametersInfo(mhandle, this);
	}

	internal override ParameterInfo[] GetParametersInternal()
	{
		return MonoMethodInfo.GetParametersInfo(mhandle, this);
	}

	internal override int GetParametersCount()
	{
		ParameterInfo[] parametersInfo = MonoMethodInfo.GetParametersInfo(mhandle, this);
		if (parametersInfo != null)
		{
			return parametersInfo.Length;
		}
		return 0;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal extern object InternalInvoke(object obj, object[] parameters, out Exception exc);

	[DebuggerStepThrough]
	[DebuggerHidden]
	public override object Invoke(object obj, BindingFlags invokeAttr, Binder binder, object[] parameters, CultureInfo culture)
	{
		if (obj == null)
		{
			if (!base.IsStatic)
			{
				throw new TargetException("Instance constructor requires a target");
			}
		}
		else if (!DeclaringType.IsInstanceOfType(obj))
		{
			throw new TargetException("Constructor does not match target type");
		}
		return DoInvoke(obj, invokeAttr, binder, parameters, culture);
	}

	private object DoInvoke(object obj, BindingFlags invokeAttr, Binder binder, object[] parameters, CultureInfo culture)
	{
		if (binder == null)
		{
			binder = Type.DefaultBinder;
		}
		ParameterInfo[] parametersInfo = MonoMethodInfo.GetParametersInfo(mhandle, this);
		MonoMethod.ConvertValues(binder, parameters, parametersInfo, culture, invokeAttr);
		if (obj == null && DeclaringType.ContainsGenericParameters)
		{
			throw new MemberAccessException(string.Concat("Cannot create an instance of ", DeclaringType, " because Type.ContainsGenericParameters is true."));
		}
		if ((invokeAttr & BindingFlags.CreateInstance) != 0 && DeclaringType.IsAbstract)
		{
			throw new MemberAccessException($"Cannot create an instance of {DeclaringType} because it is an abstract class");
		}
		return InternalInvoke(obj, parameters);
	}

	public object InternalInvoke(object obj, object[] parameters)
	{
		object obj2 = null;
		Exception exc;
		try
		{
			obj2 = InternalInvoke(obj, parameters, out exc);
		}
		catch (Exception inner)
		{
			throw new TargetInvocationException(inner);
		}
		if (exc != null)
		{
			throw exc;
		}
		if (obj != null)
		{
			return null;
		}
		return obj2;
	}

	[DebuggerStepThrough]
	[DebuggerHidden]
	public override object Invoke(BindingFlags invokeAttr, Binder binder, object[] parameters, CultureInfo culture)
	{
		return DoInvoke(null, invokeAttr, binder, parameters, culture);
	}

	public override bool IsDefined(Type attributeType, bool inherit)
	{
		return MonoCustomAttrs.IsDefined(this, attributeType, inherit);
	}

	public override object[] GetCustomAttributes(bool inherit)
	{
		return MonoCustomAttrs.GetCustomAttributes(this, inherit);
	}

	public override object[] GetCustomAttributes(Type attributeType, bool inherit)
	{
		return MonoCustomAttrs.GetCustomAttributes(this, attributeType, inherit);
	}

	public override MethodBody GetMethodBody()
	{
		return MethodBase.GetMethodBody(mhandle);
	}

	public override string ToString()
	{
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.Append("Void ");
		stringBuilder.Append(Name);
		stringBuilder.Append("(");
		ParameterInfo[] parameters = GetParameters();
		for (int i = 0; i < parameters.Length; i++)
		{
			if (i > 0)
			{
				stringBuilder.Append(", ");
			}
			stringBuilder.Append(parameters[i].ParameterType.Name);
		}
		if (CallingConvention == CallingConventions.Any)
		{
			stringBuilder.Append(", ...");
		}
		stringBuilder.Append(")");
		return stringBuilder.ToString();
	}

	public override IList<CustomAttributeData> GetCustomAttributesData()
	{
		return CustomAttributeData.GetCustomAttributes(this);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	public extern int get_core_clr_security_level();
}
