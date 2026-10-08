using System.Runtime.InteropServices;
using System.Security.Permissions;
using Unity;

namespace System.Reflection.Emit;

[Serializable]
[Obsolete("An alternate API is available: Emit the MarshalAs custom attribute instead. http://go.microsoft.com/fwlink/?linkid=14202")]
[ComVisible(true)]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public sealed class UnmanagedMarshal
{
	public UnmanagedType BaseType
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return default(UnmanagedType);
		}
	}

	public int ElementCount
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public UnmanagedType GetUnmanagedType
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return default(UnmanagedType);
		}
	}

	public Guid IIDGuid
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return default(Guid);
		}
	}

	internal UnmanagedMarshal()
	{
		ThrowStub.ThrowNotSupportedException();
	}

	public static UnmanagedMarshal DefineByValArray(int elemCount)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static UnmanagedMarshal DefineByValTStr(int elemCount)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static UnmanagedMarshal DefineLPArray(UnmanagedType elemType)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static UnmanagedMarshal DefineSafeArray(UnmanagedType elemType)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static UnmanagedMarshal DefineUnmanagedMarshal(UnmanagedType unmanagedType)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
