using System.Runtime.InteropServices;
using System.Security;
using Unity;

namespace System.Reflection.Emit;

public class SignatureHelper : _SignatureHelper
{
	private SignatureHelper()
	{
	}

	public void AddArgument(Type clsArgument)
	{
		throw new PlatformNotSupportedException();
	}

	public void AddArgument(Type argument, bool pinned)
	{
		throw new PlatformNotSupportedException();
	}

	public void AddArgument(Type argument, Type[] requiredCustomModifiers, Type[] optionalCustomModifiers)
	{
		throw new PlatformNotSupportedException();
	}

	public void AddArguments(Type[] arguments, Type[][] requiredCustomModifiers, Type[][] optionalCustomModifiers)
	{
		throw new PlatformNotSupportedException();
	}

	public void AddSentinel()
	{
		throw new PlatformNotSupportedException();
	}

	public static SignatureHelper GetFieldSigHelper(Module mod)
	{
		throw new PlatformNotSupportedException();
	}

	public static SignatureHelper GetLocalVarSigHelper()
	{
		throw new PlatformNotSupportedException();
	}

	public static SignatureHelper GetLocalVarSigHelper(Module mod)
	{
		throw new PlatformNotSupportedException();
	}

	public static SignatureHelper GetMethodSigHelper(CallingConventions callingConvention, Type returnType)
	{
		throw new PlatformNotSupportedException();
	}

	public static SignatureHelper GetMethodSigHelper(Module mod, CallingConventions callingConvention, Type returnType)
	{
		throw new PlatformNotSupportedException();
	}

	public static SignatureHelper GetMethodSigHelper(Module mod, Type returnType, Type[] parameterTypes)
	{
		throw new PlatformNotSupportedException();
	}

	public static SignatureHelper GetPropertySigHelper(Module mod, Type returnType, Type[] parameterTypes)
	{
		throw new PlatformNotSupportedException();
	}

	public static SignatureHelper GetPropertySigHelper(Module mod, Type returnType, Type[] requiredReturnTypeCustomModifiers, Type[] optionalReturnTypeCustomModifiers, Type[] parameterTypes, Type[][] requiredParameterTypeCustomModifiers, Type[][] optionalParameterTypeCustomModifiers)
	{
		throw new PlatformNotSupportedException();
	}

	public static SignatureHelper GetPropertySigHelper(Module mod, CallingConventions callingConvention, Type returnType, Type[] requiredReturnTypeCustomModifiers, Type[] optionalReturnTypeCustomModifiers, Type[] parameterTypes, Type[][] requiredParameterTypeCustomModifiers, Type[][] optionalParameterTypeCustomModifiers)
	{
		throw new PlatformNotSupportedException();
	}

	public byte[] GetSignature()
	{
		throw new PlatformNotSupportedException();
	}

	[SecuritySafeCritical]
	public static SignatureHelper GetMethodSigHelper(Module mod, CallingConvention unmanagedCallConv, Type returnType)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static SignatureHelper GetMethodSigHelper(CallingConvention unmanagedCallingConvention, Type returnType)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	void _SignatureHelper.GetIDsOfNames([In] ref Guid riid, IntPtr rgszNames, uint cNames, uint lcid, IntPtr rgDispId)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _SignatureHelper.GetTypeInfo(uint iTInfo, uint lcid, IntPtr ppTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _SignatureHelper.GetTypeInfoCount(out uint pcTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _SignatureHelper.Invoke(uint dispIdMember, [In] ref Guid riid, uint lcid, short wFlags, IntPtr pDispParams, IntPtr pVarResult, IntPtr pExcepInfo, IntPtr puArgErr)
	{
		ThrowStub.ThrowNotSupportedException();
	}
}
