using Unity;

namespace System.Security.Permissions;

[Serializable]
[AttributeUsage(AttributeTargets.Assembly | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Constructor | AttributeTargets.Method, AllowMultiple = true, Inherited = false)]
public sealed class TypeDescriptorPermissionAttribute : CodeAccessSecurityAttribute
{
	public TypeDescriptorPermissionFlags Flags
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(TypeDescriptorPermissionFlags);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool RestrictedRegistrationAccess
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public TypeDescriptorPermissionAttribute(SecurityAction action)
	{
	}

	public override IPermission CreatePermission()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
