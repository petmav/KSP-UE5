using System.Collections.Generic;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Security;
using System.Security.Permissions;
using Unity;

namespace System.Reflection.Emit;

public abstract class ConstructorBuilder : ConstructorInfo, _ConstructorBuilder
{
	public bool InitLocals { get; set; }

	public override MethodAttributes Attributes
	{
		get
		{
			throw new PlatformNotSupportedException();
		}
	}

	public override Type DeclaringType
	{
		get
		{
			throw new PlatformNotSupportedException();
		}
	}

	public override string Name
	{
		get
		{
			throw new PlatformNotSupportedException();
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

	public override Type ReflectedType
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public Type ReturnType
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string Signature
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ParameterBuilder DefineParameter(int iSequence, ParameterAttributes attributes, string strParamName)
	{
		throw new PlatformNotSupportedException();
	}

	public ILGenerator GetILGenerator()
	{
		throw new PlatformNotSupportedException();
	}

	public ILGenerator GetILGenerator(int streamSize)
	{
		throw new PlatformNotSupportedException();
	}

	public override ParameterInfo[] GetParameters()
	{
		throw new PlatformNotSupportedException();
	}

	public void SetCustomAttribute(CustomAttributeBuilder customBuilder)
	{
		throw new PlatformNotSupportedException();
	}

	public void SetCustomAttribute(ConstructorInfo con, byte[] binaryAttribute)
	{
		throw new PlatformNotSupportedException();
	}

	public void SetImplementationFlags(MethodImplAttributes attributes)
	{
		throw new PlatformNotSupportedException();
	}

	[SecuritySafeCritical]
	public void AddDeclarativeSecurity(SecurityAction action, PermissionSet pset)
	{
		ThrowStub.ThrowNotSupportedException();
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

	public override MethodImplAttributes GetMethodImplementationFlags()
	{
		ThrowStub.ThrowNotSupportedException();
		return default(MethodImplAttributes);
	}

	public Module GetModule()
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public MethodToken GetToken()
	{
		ThrowStub.ThrowNotSupportedException();
		return default(MethodToken);
	}

	public override object Invoke(object obj, BindingFlags invokeAttr, Binder binder, object[] parameters, CultureInfo culture)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override object Invoke(BindingFlags invokeAttr, Binder binder, object[] parameters, CultureInfo culture)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override bool IsDefined(Type attributeType, bool inherit)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void SetMethodBody(byte[] il, int maxStack, byte[] localSignature, IEnumerable<ExceptionHandler> exceptionHandlers, IEnumerable<int> tokenFixups)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	public void SetSymCustomAttribute(string name, byte[] data)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _ConstructorBuilder.GetIDsOfNames([In] ref Guid riid, IntPtr rgszNames, uint cNames, uint lcid, IntPtr rgDispId)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _ConstructorBuilder.GetTypeInfo(uint iTInfo, uint lcid, IntPtr ppTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _ConstructorBuilder.GetTypeInfoCount(out uint pcTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _ConstructorBuilder.Invoke(uint dispIdMember, [In] ref Guid riid, uint lcid, short wFlags, IntPtr pDispParams, IntPtr pVarResult, IntPtr pExcepInfo, IntPtr puArgErr)
	{
		ThrowStub.ThrowNotSupportedException();
	}
}
