using System.Globalization;
using System.Runtime.InteropServices;
using System.Security;
using Unity;

namespace System.Reflection.Emit;

[ComVisible(true)]
public sealed class DynamicMethod : MethodInfo
{
	public override MethodAttributes Attributes
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return default(MethodAttributes);
		}
	}

	public override Type DeclaringType
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public bool InitLocals
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			ThrowStub.ThrowNotSupportedException();
		}
	}

	public override RuntimeMethodHandle MethodHandle
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return default(RuntimeMethodHandle);
		}
	}

	public override string Name
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public override Type ReflectedType
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public override ICustomAttributeProvider ReturnTypeCustomAttributes
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	[SecuritySafeCritical]
	public DynamicMethod(string name, MethodAttributes attributes, CallingConventions callingConvention, Type returnType, Type[] parameterTypes, Module m, bool skipVisibility)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	public DynamicMethod(string name, MethodAttributes attributes, CallingConventions callingConvention, Type returnType, Type[] parameterTypes, Type owner, bool skipVisibility)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	public DynamicMethod(string name, Type returnType, Type[] parameterTypes)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	public DynamicMethod(string name, Type returnType, Type[] parameterTypes, bool restrictedSkipVisibility)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	public DynamicMethod(string name, Type returnType, Type[] parameterTypes, Module m)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	public DynamicMethod(string name, Type returnType, Type[] parameterTypes, Module m, bool skipVisibility)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	public DynamicMethod(string name, Type returnType, Type[] parameterTypes, Type owner)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	public DynamicMethod(string name, Type returnType, Type[] parameterTypes, Type owner, bool skipVisibility)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	public ParameterBuilder DefineParameter(int position, ParameterAttributes attributes, string parameterName)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override MethodInfo GetBaseDefinition()
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override object[] GetCustomAttributes(bool inherit)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override object[] GetCustomAttributes(Type attributeType, bool inherit)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SecuritySafeCritical]
	public DynamicILInfo GetDynamicILInfo()
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public ILGenerator GetILGenerator()
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SecuritySafeCritical]
	public ILGenerator GetILGenerator(int streamSize)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override MethodImplAttributes GetMethodImplementationFlags()
	{
		ThrowStub.ThrowNotSupportedException();
		return default(MethodImplAttributes);
	}

	public override ParameterInfo[] GetParameters()
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SecuritySafeCritical]
	public override object Invoke(object obj, BindingFlags invokeAttr, Binder binder, object[] parameters, CultureInfo culture)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override bool IsDefined(Type attributeType, bool inherit)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}
