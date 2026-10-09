using System.Runtime.InteropServices;

namespace System.Security.Permissions;

[Serializable]
[AttributeUsage(AttributeTargets.Assembly | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Constructor | AttributeTargets.Method, AllowMultiple = true, Inherited = false)]
[ComVisible(true)]
public sealed class EnvironmentPermissionAttribute : CodeAccessSecurityAttribute
{
	private string read;

	private string write;

	public string All
	{
		get
		{
			throw new NotSupportedException("All");
		}
		set
		{
			read = value;
			write = value;
		}
	}

	public string Read
	{
		get
		{
			return read;
		}
		set
		{
			read = value;
		}
	}

	public string Write
	{
		get
		{
			return write;
		}
		set
		{
			write = value;
		}
	}

	public EnvironmentPermissionAttribute(SecurityAction action)
		: base(action)
	{
	}

	public override IPermission CreatePermission()
	{
		EnvironmentPermission environmentPermission = null;
		if (base.Unrestricted)
		{
			environmentPermission = new EnvironmentPermission(PermissionState.Unrestricted);
		}
		else
		{
			environmentPermission = new EnvironmentPermission(PermissionState.None);
			if (read != null)
			{
				environmentPermission.AddPathList(EnvironmentPermissionAccess.Read, read);
			}
			if (write != null)
			{
				environmentPermission.AddPathList(EnvironmentPermissionAccess.Write, write);
			}
		}
		return environmentPermission;
	}
}
