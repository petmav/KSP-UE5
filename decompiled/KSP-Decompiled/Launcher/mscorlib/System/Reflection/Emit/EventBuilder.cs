using System.Runtime.InteropServices;
using Unity;

namespace System.Reflection.Emit;

public class EventBuilder : _EventBuilder
{
	public void AddOtherMethod(MethodBuilder mdBuilder)
	{
		throw new PlatformNotSupportedException();
	}

	public void SetAddOnMethod(MethodBuilder mdBuilder)
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

	public void SetRaiseMethod(MethodBuilder mdBuilder)
	{
		throw new PlatformNotSupportedException();
	}

	public void SetRemoveOnMethod(MethodBuilder mdBuilder)
	{
		throw new PlatformNotSupportedException();
	}

	public EventToken GetEventToken()
	{
		ThrowStub.ThrowNotSupportedException();
		return default(EventToken);
	}

	void _EventBuilder.GetIDsOfNames([In] ref Guid riid, IntPtr rgszNames, uint cNames, uint lcid, IntPtr rgDispId)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _EventBuilder.GetTypeInfo(uint iTInfo, uint lcid, IntPtr ppTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _EventBuilder.GetTypeInfoCount(out uint pcTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _EventBuilder.Invoke(uint dispIdMember, [In] ref Guid riid, uint lcid, short wFlags, IntPtr pDispParams, IntPtr pVarResult, IntPtr pExcepInfo, IntPtr puArgErr)
	{
		ThrowStub.ThrowNotSupportedException();
	}
}
