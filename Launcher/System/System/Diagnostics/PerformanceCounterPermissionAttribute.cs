using System.Security;
using System.Security.Permissions;
using Unity;

namespace System.Diagnostics;

[Serializable]
[AttributeUsage(AttributeTargets.Assembly | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Constructor | AttributeTargets.Method | AttributeTargets.Event, AllowMultiple = true, Inherited = false)]
public class PerformanceCounterPermissionAttribute : CodeAccessSecurityAttribute
{
	public string CategoryName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public string MachineName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public PerformanceCounterPermissionAccess PermissionAccess
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(PerformanceCounterPermissionAccess);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public PerformanceCounterPermissionAttribute(SecurityAction action)
	{
	}

	public override IPermission CreatePermission()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
