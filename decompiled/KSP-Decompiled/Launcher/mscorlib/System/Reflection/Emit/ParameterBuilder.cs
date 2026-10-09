using System.Runtime.InteropServices;
using System.Security;
using Unity;

namespace System.Reflection.Emit;

public class ParameterBuilder : _ParameterBuilder
{
	public int Attributes
	{
		get
		{
			throw new PlatformNotSupportedException();
		}
	}

	public bool IsIn
	{
		get
		{
			throw new PlatformNotSupportedException();
		}
	}

	public bool IsOptional
	{
		get
		{
			throw new PlatformNotSupportedException();
		}
	}

	public bool IsOut
	{
		get
		{
			throw new PlatformNotSupportedException();
		}
	}

	public string Name
	{
		get
		{
			throw new PlatformNotSupportedException();
		}
	}

	public int Position
	{
		get
		{
			throw new PlatformNotSupportedException();
		}
	}

	private ParameterBuilder()
	{
	}

	public virtual void SetConstant(object defaultValue)
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

	public virtual ParameterToken GetToken()
	{
		ThrowStub.ThrowNotSupportedException();
		return default(ParameterToken);
	}

	[SecuritySafeCritical]
	[Obsolete("An alternate API is available: Emit the MarshalAs custom attribute instead. http://go.microsoft.com/fwlink/?linkid=14202")]
	public virtual void SetMarshal(UnmanagedMarshal unmanagedMarshal)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _ParameterBuilder.GetIDsOfNames([In] ref Guid riid, IntPtr rgszNames, uint cNames, uint lcid, IntPtr rgDispId)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _ParameterBuilder.GetTypeInfo(uint iTInfo, uint lcid, IntPtr ppTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _ParameterBuilder.GetTypeInfoCount(out uint pcTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _ParameterBuilder.Invoke(uint dispIdMember, [In] ref Guid riid, uint lcid, short wFlags, IntPtr pDispParams, IntPtr pVarResult, IntPtr pExcepInfo, IntPtr puArgErr)
	{
		ThrowStub.ThrowNotSupportedException();
	}
}
