using System.Collections;

namespace System.Security.Permissions;

[Serializable]
public abstract class ResourcePermissionBase : CodeAccessPermission, IUnrestrictedPermission
{
	private const int version = 1;

	private ArrayList _list;

	private bool _unrestricted;

	private Type _type;

	private string[] _tags;

	public const string Any = "*";

	public const string Local = ".";

	private static char[] invalidChars = new char[8] { '\t', '\n', '\v', '\f', '\r', ' ', '\\', 'Š' };

	protected Type PermissionAccessType
	{
		get
		{
			return _type;
		}
		set
		{
			if (value == null)
			{
				throw new ArgumentNullException("PermissionAccessType");
			}
			if (!value.IsEnum)
			{
				throw new ArgumentException("!Enum", "PermissionAccessType");
			}
			_type = value;
		}
	}

	protected string[] TagNames
	{
		get
		{
			return _tags;
		}
		set
		{
			if (value == null)
			{
				throw new ArgumentNullException("TagNames");
			}
			if (value.Length == 0)
			{
				throw new ArgumentException("Length==0", "TagNames");
			}
			_tags = value;
		}
	}

	protected ResourcePermissionBase()
	{
		_list = new ArrayList();
	}

	protected ResourcePermissionBase(PermissionState state)
		: this()
	{
		PermissionHelper.CheckPermissionState(state, allowUnrestricted: true);
		_unrestricted = state == PermissionState.Unrestricted;
	}

	protected void AddPermissionAccess(ResourcePermissionBaseEntry entry)
	{
		CheckEntry(entry);
		if (Exists(entry))
		{
			throw new InvalidOperationException(global::Locale.GetText("Entry already exists."));
		}
		_list.Add(entry);
	}

	protected void Clear()
	{
		_list.Clear();
	}

	public override IPermission Copy()
	{
		ResourcePermissionBase resourcePermissionBase = CreateFromType(GetType(), _unrestricted);
		if (_tags != null)
		{
			resourcePermissionBase._tags = (string[])_tags.Clone();
		}
		resourcePermissionBase._type = _type;
		resourcePermissionBase._list.AddRange(_list);
		return resourcePermissionBase;
	}

	[System.MonoTODO("incomplete - need more test")]
	public override void FromXml(SecurityElement securityElement)
	{
		if (securityElement == null)
		{
			throw new ArgumentNullException("securityElement");
		}
		CodeAccessPermission.CheckSecurityElement(securityElement, "securityElement", 1, 1);
		_list.Clear();
		_unrestricted = PermissionHelper.IsUnrestricted(securityElement);
		if (securityElement.Children == null || securityElement.Children.Count < 1)
		{
			return;
		}
		string[] array = new string[1];
		foreach (SecurityElement child in securityElement.Children)
		{
			array[0] = child.Attribute("name");
			ResourcePermissionBaseEntry entry = new ResourcePermissionBaseEntry((int)Enum.Parse(PermissionAccessType, child.Attribute("access")), array);
			AddPermissionAccess(entry);
		}
	}

	protected ResourcePermissionBaseEntry[] GetPermissionEntries()
	{
		ResourcePermissionBaseEntry[] array = new ResourcePermissionBaseEntry[_list.Count];
		_list.CopyTo(array, 0);
		return array;
	}

	public override IPermission Intersect(IPermission target)
	{
		ResourcePermissionBase resourcePermissionBase = Cast(target);
		if (resourcePermissionBase == null)
		{
			return null;
		}
		bool flag = IsUnrestricted();
		bool flag2 = resourcePermissionBase.IsUnrestricted();
		if (IsEmpty() && !flag2)
		{
			return null;
		}
		if (resourcePermissionBase.IsEmpty() && !flag)
		{
			return null;
		}
		ResourcePermissionBase resourcePermissionBase2 = CreateFromType(GetType(), flag && flag2);
		foreach (ResourcePermissionBaseEntry item in _list)
		{
			if (flag2 || resourcePermissionBase.Exists(item))
			{
				resourcePermissionBase2.AddPermissionAccess(item);
			}
		}
		foreach (ResourcePermissionBaseEntry item2 in resourcePermissionBase._list)
		{
			if ((flag || Exists(item2)) && !resourcePermissionBase2.Exists(item2))
			{
				resourcePermissionBase2.AddPermissionAccess(item2);
			}
		}
		return resourcePermissionBase2;
	}

	public override bool IsSubsetOf(IPermission target)
	{
		if (target == null)
		{
			return true;
		}
		if (!(target is ResourcePermissionBase resourcePermissionBase))
		{
			return false;
		}
		if (resourcePermissionBase.IsUnrestricted())
		{
			return true;
		}
		if (IsUnrestricted())
		{
			return resourcePermissionBase.IsUnrestricted();
		}
		foreach (ResourcePermissionBaseEntry item in _list)
		{
			if (!resourcePermissionBase.Exists(item))
			{
				return false;
			}
		}
		return true;
	}

	public bool IsUnrestricted()
	{
		return _unrestricted;
	}

	protected void RemovePermissionAccess(ResourcePermissionBaseEntry entry)
	{
		CheckEntry(entry);
		for (int i = 0; i < _list.Count; i++)
		{
			ResourcePermissionBaseEntry entry2 = (ResourcePermissionBaseEntry)_list[i];
			if (Equals(entry, entry2))
			{
				_list.RemoveAt(i);
				return;
			}
		}
		throw new InvalidOperationException(global::Locale.GetText("Entry doesn't exists."));
	}

	public override SecurityElement ToXml()
	{
		SecurityElement securityElement = PermissionHelper.Element(GetType(), 1);
		if (IsUnrestricted())
		{
			securityElement.AddAttribute("Unrestricted", "true");
		}
		else
		{
			foreach (ResourcePermissionBaseEntry item in _list)
			{
				SecurityElement securityElement2 = securityElement;
				string text = null;
				if (PermissionAccessType != null)
				{
					text = Enum.Format(PermissionAccessType, item.PermissionAccess, "g");
				}
				for (int i = 0; i < _tags.Length; i++)
				{
					SecurityElement securityElement3 = new SecurityElement(_tags[i]);
					securityElement3.AddAttribute("name", item.PermissionAccessPath[i]);
					if (text != null)
					{
						securityElement3.AddAttribute("access", text);
					}
					securityElement2.AddChild(securityElement3);
					securityElement3 = securityElement2;
				}
			}
		}
		return securityElement;
	}

	public override IPermission Union(IPermission target)
	{
		ResourcePermissionBase resourcePermissionBase = Cast(target);
		if (resourcePermissionBase == null)
		{
			return Copy();
		}
		if (IsEmpty() && resourcePermissionBase.IsEmpty())
		{
			return null;
		}
		if (resourcePermissionBase.IsEmpty())
		{
			return Copy();
		}
		if (IsEmpty())
		{
			return resourcePermissionBase.Copy();
		}
		bool flag = IsUnrestricted() || resourcePermissionBase.IsUnrestricted();
		ResourcePermissionBase resourcePermissionBase2 = CreateFromType(GetType(), flag);
		if (!flag)
		{
			foreach (ResourcePermissionBaseEntry item in _list)
			{
				resourcePermissionBase2.AddPermissionAccess(item);
			}
			foreach (ResourcePermissionBaseEntry item2 in resourcePermissionBase._list)
			{
				if (!resourcePermissionBase2.Exists(item2))
				{
					resourcePermissionBase2.AddPermissionAccess(item2);
				}
			}
		}
		return resourcePermissionBase2;
	}

	private bool IsEmpty()
	{
		if (!_unrestricted)
		{
			return _list.Count == 0;
		}
		return false;
	}

	private ResourcePermissionBase Cast(IPermission target)
	{
		if (target == null)
		{
			return null;
		}
		ResourcePermissionBase obj = target as ResourcePermissionBase;
		if (obj == null)
		{
			PermissionHelper.ThrowInvalidPermission(target, typeof(ResourcePermissionBase));
		}
		return obj;
	}

	internal void CheckEntry(ResourcePermissionBaseEntry entry)
	{
		if (entry == null)
		{
			throw new ArgumentNullException("entry");
		}
		if (entry.PermissionAccessPath == null || entry.PermissionAccessPath.Length != _tags.Length)
		{
			throw new InvalidOperationException(global::Locale.GetText("Entry doesn't match TagNames"));
		}
	}

	internal bool Equals(ResourcePermissionBaseEntry entry1, ResourcePermissionBaseEntry entry2)
	{
		if (entry1.PermissionAccess != entry2.PermissionAccess)
		{
			return false;
		}
		if (entry1.PermissionAccessPath.Length != entry2.PermissionAccessPath.Length)
		{
			return false;
		}
		for (int i = 0; i < entry1.PermissionAccessPath.Length; i++)
		{
			if (entry1.PermissionAccessPath[i] != entry2.PermissionAccessPath[i])
			{
				return false;
			}
		}
		return true;
	}

	internal bool Exists(ResourcePermissionBaseEntry entry)
	{
		if (_list.Count == 0)
		{
			return false;
		}
		foreach (ResourcePermissionBaseEntry item in _list)
		{
			if (Equals(item, entry))
			{
				return true;
			}
		}
		return false;
	}

	internal static void ValidateMachineName(string name)
	{
		if (name == null || name.Length == 0 || name.IndexOfAny(invalidChars) != -1)
		{
			string text = global::Locale.GetText("Invalid machine name '{0}'.");
			if (name == null)
			{
				name = "(null)";
			}
			throw new ArgumentException(string.Format(text, name), "MachineName");
		}
	}

	internal static ResourcePermissionBase CreateFromType(Type type, bool unrestricted)
	{
		return (ResourcePermissionBase)Activator.CreateInstance(type, unrestricted ? PermissionState.Unrestricted : PermissionState.None);
	}
}
