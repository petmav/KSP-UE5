namespace System.Security.Permissions;

[Serializable]
[AttributeUsage(AttributeTargets.Assembly | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Constructor | AttributeTargets.Method, AllowMultiple = true, Inherited = false)]
public sealed class StorePermissionAttribute : CodeAccessSecurityAttribute
{
	private StorePermissionFlags _flags;

	public StorePermissionFlags Flags
	{
		get
		{
			return _flags;
		}
		set
		{
			if ((value & StorePermissionFlags.AllFlags) != value)
			{
				throw new ArgumentException(string.Format(global::Locale.GetText("Invalid flags {0}"), value), "StorePermissionFlags");
			}
			_flags = value;
		}
	}

	public bool AddToStore
	{
		get
		{
			return (_flags & StorePermissionFlags.AddToStore) != 0;
		}
		set
		{
			if (value)
			{
				_flags |= StorePermissionFlags.AddToStore;
			}
			else
			{
				_flags &= ~StorePermissionFlags.AddToStore;
			}
		}
	}

	public bool CreateStore
	{
		get
		{
			return (_flags & StorePermissionFlags.CreateStore) != 0;
		}
		set
		{
			if (value)
			{
				_flags |= StorePermissionFlags.CreateStore;
			}
			else
			{
				_flags &= ~StorePermissionFlags.CreateStore;
			}
		}
	}

	public bool DeleteStore
	{
		get
		{
			return (_flags & StorePermissionFlags.DeleteStore) != 0;
		}
		set
		{
			if (value)
			{
				_flags |= StorePermissionFlags.DeleteStore;
			}
			else
			{
				_flags &= ~StorePermissionFlags.DeleteStore;
			}
		}
	}

	public bool EnumerateCertificates
	{
		get
		{
			return (_flags & StorePermissionFlags.EnumerateCertificates) != 0;
		}
		set
		{
			if (value)
			{
				_flags |= StorePermissionFlags.EnumerateCertificates;
			}
			else
			{
				_flags &= ~StorePermissionFlags.EnumerateCertificates;
			}
		}
	}

	public bool EnumerateStores
	{
		get
		{
			return (_flags & StorePermissionFlags.EnumerateStores) != 0;
		}
		set
		{
			if (value)
			{
				_flags |= StorePermissionFlags.EnumerateStores;
			}
			else
			{
				_flags &= ~StorePermissionFlags.EnumerateStores;
			}
		}
	}

	public bool OpenStore
	{
		get
		{
			return (_flags & StorePermissionFlags.OpenStore) != 0;
		}
		set
		{
			if (value)
			{
				_flags |= StorePermissionFlags.OpenStore;
			}
			else
			{
				_flags &= ~StorePermissionFlags.OpenStore;
			}
		}
	}

	public bool RemoveFromStore
	{
		get
		{
			return (_flags & StorePermissionFlags.RemoveFromStore) != 0;
		}
		set
		{
			if (value)
			{
				_flags |= StorePermissionFlags.RemoveFromStore;
			}
			else
			{
				_flags &= ~StorePermissionFlags.RemoveFromStore;
			}
		}
	}

	public StorePermissionAttribute(SecurityAction action)
		: base(action)
	{
		_flags = StorePermissionFlags.NoFlags;
	}

	public override IPermission CreatePermission()
	{
		StorePermission storePermission = null;
		if (base.Unrestricted)
		{
			return new StorePermission(PermissionState.Unrestricted);
		}
		return new StorePermission(_flags);
	}
}
