using System.Security;
using System.Security.Permissions;
using Unity;

namespace System.Diagnostics;

[Serializable]
[AttributeUsage(AttributeTargets.Assembly | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Constructor | AttributeTargets.Method | AttributeTargets.Event, AllowMultiple = true, Inherited = false)]
public class EventLogPermissionAttribute : CodeAccessSecurityAttribute
{
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

	public EventLogPermissionAccess PermissionAccess
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(EventLogPermissionAccess);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public EventLogPermissionAttribute(SecurityAction action)
	{
	}

	public override IPermission CreatePermission()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
