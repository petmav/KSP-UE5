using Unity;

namespace System.Security.Permissions;

[Serializable]
public class ResourcePermissionBaseEntry
{
	public int PermissionAccess
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public string[] PermissionAccessPath
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ResourcePermissionBaseEntry()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ResourcePermissionBaseEntry(int permissionAccess, string[] permissionAccessPath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
