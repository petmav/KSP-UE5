namespace System.Security.Permissions;

[Serializable]
[AttributeUsage(AttributeTargets.Assembly | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Constructor | AttributeTargets.Method, AllowMultiple = true, Inherited = false)]
public sealed class DataProtectionPermissionAttribute : CodeAccessSecurityAttribute
{
	private DataProtectionPermissionFlags _flags;

	public DataProtectionPermissionFlags Flags
	{
		get
		{
			return _flags;
		}
		set
		{
			if ((value & DataProtectionPermissionFlags.AllFlags) != value)
			{
				throw new ArgumentException(string.Format(global::Locale.GetText("Invalid flags {0}"), value), "DataProtectionPermissionFlags");
			}
			_flags = value;
		}
	}

	public bool ProtectData
	{
		get
		{
			return (_flags & DataProtectionPermissionFlags.ProtectData) != 0;
		}
		set
		{
			if (value)
			{
				_flags |= DataProtectionPermissionFlags.ProtectData;
			}
			else
			{
				_flags &= ~DataProtectionPermissionFlags.ProtectData;
			}
		}
	}

	public bool UnprotectData
	{
		get
		{
			return (_flags & DataProtectionPermissionFlags.UnprotectData) != 0;
		}
		set
		{
			if (value)
			{
				_flags |= DataProtectionPermissionFlags.UnprotectData;
			}
			else
			{
				_flags &= ~DataProtectionPermissionFlags.UnprotectData;
			}
		}
	}

	public bool ProtectMemory
	{
		get
		{
			return (_flags & DataProtectionPermissionFlags.ProtectMemory) != 0;
		}
		set
		{
			if (value)
			{
				_flags |= DataProtectionPermissionFlags.ProtectMemory;
			}
			else
			{
				_flags &= ~DataProtectionPermissionFlags.ProtectMemory;
			}
		}
	}

	public bool UnprotectMemory
	{
		get
		{
			return (_flags & DataProtectionPermissionFlags.UnprotectMemory) != 0;
		}
		set
		{
			if (value)
			{
				_flags |= DataProtectionPermissionFlags.UnprotectMemory;
			}
			else
			{
				_flags &= ~DataProtectionPermissionFlags.UnprotectMemory;
			}
		}
	}

	public DataProtectionPermissionAttribute(SecurityAction action)
		: base(action)
	{
	}

	public override IPermission CreatePermission()
	{
		DataProtectionPermission dataProtectionPermission = null;
		if (base.Unrestricted)
		{
			return new DataProtectionPermission(PermissionState.Unrestricted);
		}
		return new DataProtectionPermission(_flags);
	}
}
