using System.Runtime.InteropServices;

namespace System.Security.Permissions;

[Serializable]
[AttributeUsage(AttributeTargets.Assembly | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Constructor | AttributeTargets.Method, AllowMultiple = true, Inherited = false)]
[ComVisible(true)]
public sealed class StrongNameIdentityPermissionAttribute : CodeAccessSecurityAttribute
{
	private string name;

	private string key;

	private string version;

	public string Name
	{
		get
		{
			return name;
		}
		set
		{
			name = value;
		}
	}

	public string PublicKey
	{
		get
		{
			return key;
		}
		set
		{
			key = value;
		}
	}

	public string Version
	{
		get
		{
			return version;
		}
		set
		{
			version = value;
		}
	}

	public StrongNameIdentityPermissionAttribute(SecurityAction action)
		: base(action)
	{
	}

	public override IPermission CreatePermission()
	{
		if (base.Unrestricted)
		{
			return new StrongNameIdentityPermission(PermissionState.Unrestricted);
		}
		if (name == null && key == null && this.version == null)
		{
			return new StrongNameIdentityPermission(PermissionState.None);
		}
		if (key == null)
		{
			throw new ArgumentException(Locale.GetText("PublicKey is required"));
		}
		StrongNamePublicKeyBlob blob = StrongNamePublicKeyBlob.FromString(key);
		Version version = null;
		if (this.version != null)
		{
			version = new Version(this.version);
		}
		return new StrongNameIdentityPermission(blob, name, version);
	}
}
