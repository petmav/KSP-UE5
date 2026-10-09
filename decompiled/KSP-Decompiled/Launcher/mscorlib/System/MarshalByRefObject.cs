using System.Runtime.InteropServices;
using System.Runtime.Remoting;
using System.Security;
using System.Security.Permissions;

namespace System;

[Serializable]
[StructLayout(LayoutKind.Sequential)]
[ComVisible(true)]
public abstract class MarshalByRefObject
{
	[NonSerialized]
	private object _identity;

	internal ServerIdentity ObjectIdentity
	{
		get
		{
			throw new NotSupportedException();
		}
		set
		{
			throw new NotSupportedException();
		}
	}

	[SecurityPermission(SecurityAction.LinkDemand, Infrastructure = true)]
	public virtual ObjRef CreateObjRef(Type requestedType)
	{
		throw new NotSupportedException();
	}

	[SecurityCritical]
	[SecurityPermission(SecurityAction.LinkDemand, Infrastructure = true)]
	public object GetLifetimeService()
	{
		throw new NotSupportedException();
	}

	[SecurityPermission(SecurityAction.LinkDemand, Infrastructure = true)]
	public virtual object InitializeLifetimeService()
	{
		throw new NotSupportedException();
	}

	protected MarshalByRefObject MemberwiseClone(bool cloneIdentity)
	{
		throw new NotSupportedException();
	}
}
