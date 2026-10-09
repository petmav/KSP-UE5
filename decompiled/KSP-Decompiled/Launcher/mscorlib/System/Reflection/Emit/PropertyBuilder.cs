using System.Globalization;
using System.Runtime.InteropServices;
using Unity;

namespace System.Reflection.Emit;

public abstract class PropertyBuilder : PropertyInfo, _PropertyBuilder
{
	public override PropertyAttributes Attributes
	{
		get
		{
			throw new PlatformNotSupportedException();
		}
	}

	public override bool CanRead
	{
		get
		{
			throw new PlatformNotSupportedException();
		}
	}

	public override bool CanWrite
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

	public override Type PropertyType
	{
		get
		{
			throw new PlatformNotSupportedException();
		}
	}

	public PropertyToken PropertyToken
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return default(PropertyToken);
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

	public void AddOtherMethod(MethodBuilder mdBuilder)
	{
		throw new PlatformNotSupportedException();
	}

	public void SetConstant(object defaultValue)
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

	public void SetGetMethod(MethodBuilder mdBuilder)
	{
		throw new PlatformNotSupportedException();
	}

	public void SetSetMethod(MethodBuilder mdBuilder)
	{
		throw new PlatformNotSupportedException();
	}

	public override ParameterInfo[] GetIndexParameters()
	{
		throw new PlatformNotSupportedException();
	}

	public override MethodInfo[] GetAccessors(bool nonPublic)
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

	public override MethodInfo GetGetMethod(bool nonPublic)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override MethodInfo GetSetMethod(bool nonPublic)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override object GetValue(object obj, BindingFlags invokeAttr, Binder binder, object[] index, CultureInfo culture)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override bool IsDefined(Type attributeType, bool inherit)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public override void SetValue(object obj, object value, BindingFlags invokeAttr, Binder binder, object[] index, CultureInfo culture)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _PropertyBuilder.GetIDsOfNames([In] ref Guid riid, IntPtr rgszNames, uint cNames, uint lcid, IntPtr rgDispId)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _PropertyBuilder.GetTypeInfo(uint iTInfo, uint lcid, IntPtr ppTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _PropertyBuilder.GetTypeInfoCount(out uint pcTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _PropertyBuilder.Invoke(uint dispIdMember, [In] ref Guid riid, uint lcid, short wFlags, IntPtr pDispParams, IntPtr pVarResult, IntPtr pExcepInfo, IntPtr puArgErr)
	{
		ThrowStub.ThrowNotSupportedException();
	}
}
