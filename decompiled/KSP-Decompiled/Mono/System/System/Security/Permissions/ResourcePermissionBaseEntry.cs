namespace System.Security.Permissions;

[Serializable]
public class ResourcePermissionBaseEntry
{
	private int permissionAccess;

	private string[] permissionAccessPath;

	public int PermissionAccess => permissionAccess;

	public string[] PermissionAccessPath => permissionAccessPath;

	public ResourcePermissionBaseEntry()
	{
		permissionAccessPath = new string[0];
	}

	public ResourcePermissionBaseEntry(int permissionAccess, string[] permissionAccessPath)
	{
		if (permissionAccessPath == null)
		{
			throw new ArgumentNullException("permissionAccessPath");
		}
		this.permissionAccess = permissionAccess;
		this.permissionAccessPath = permissionAccessPath;
	}
}
