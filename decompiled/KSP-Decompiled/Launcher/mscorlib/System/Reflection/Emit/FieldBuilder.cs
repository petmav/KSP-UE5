using System.Globalization;
using System.Runtime.InteropServices;
using System.Security;
using Unity;

namespace System.Reflection.Emit;

public abstract class FieldBuilder : FieldInfo, _FieldBuilder
{
	public override FieldAttributes Attributes
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

	public override Type FieldType
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

	public override RuntimeFieldHandle FieldHandle
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return default(RuntimeFieldHandle);
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

	public override object GetValue(object obj)
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

	public void SetOffset(int iOffset)
	{
		throw new PlatformNotSupportedException();
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

	public FieldToken GetToken()
	{
		ThrowStub.ThrowNotSupportedException();
		return default(FieldToken);
	}

	public override bool IsDefined(Type attributeType, bool inherit)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	[Obsolete("An alternate API is available: Emit the MarshalAs custom attribute instead. http://go.microsoft.com/fwlink/?linkid=14202")]
	[SecuritySafeCritical]
	public void SetMarshal(UnmanagedMarshal unmanagedMarshal)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	public override void SetValue(object obj, object val, BindingFlags invokeAttr, Binder binder, CultureInfo culture)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _FieldBuilder.GetIDsOfNames([In] ref Guid riid, IntPtr rgszNames, uint cNames, uint lcid, IntPtr rgDispId)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _FieldBuilder.GetTypeInfo(uint iTInfo, uint lcid, IntPtr ppTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _FieldBuilder.GetTypeInfoCount(out uint pcTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _FieldBuilder.Invoke(uint dispIdMember, [In] ref Guid riid, uint lcid, short wFlags, IntPtr pDispParams, IntPtr pVarResult, IntPtr pExcepInfo, IntPtr puArgErr)
	{
		ThrowStub.ThrowNotSupportedException();
	}
}
