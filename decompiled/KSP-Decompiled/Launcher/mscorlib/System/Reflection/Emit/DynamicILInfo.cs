using System.Runtime.InteropServices;
using System.Security;
using Unity;

namespace System.Reflection.Emit;

[ComVisible(true)]
public class DynamicILInfo
{
	public DynamicMethod DynamicMethod
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal DynamicILInfo()
	{
		ThrowStub.ThrowNotSupportedException();
	}

	public int GetTokenFor(byte[] signature)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public int GetTokenFor(DynamicMethod method)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public int GetTokenFor(RuntimeFieldHandle field)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public int GetTokenFor(RuntimeFieldHandle field, RuntimeTypeHandle contextType)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	[SecuritySafeCritical]
	public int GetTokenFor(RuntimeMethodHandle method)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public int GetTokenFor(RuntimeMethodHandle method, RuntimeTypeHandle contextType)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public int GetTokenFor(RuntimeTypeHandle type)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public int GetTokenFor(string literal)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	[SecurityCritical]
	[CLSCompliant(false)]
	public unsafe void SetCode(byte* code, int codeSize, int maxStackSize)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	public void SetCode(byte[] code, int maxStackSize)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	[CLSCompliant(false)]
	[SecurityCritical]
	public unsafe void SetExceptions(byte* exceptions, int exceptionsSize)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	public void SetExceptions(byte[] exceptions)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	[CLSCompliant(false)]
	[SecurityCritical]
	public unsafe void SetLocalSignature(byte* localSignature, int signatureSize)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	public void SetLocalSignature(byte[] localSignature)
	{
		ThrowStub.ThrowNotSupportedException();
	}
}
