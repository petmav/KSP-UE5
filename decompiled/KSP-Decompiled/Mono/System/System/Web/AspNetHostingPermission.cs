using System.Security;
using System.Security.Permissions;

namespace System.Web;

[Serializable]
public sealed class AspNetHostingPermission : CodeAccessPermission, IUnrestrictedPermission
{
	private const int version = 1;

	private AspNetHostingPermissionLevel _level;

	public AspNetHostingPermissionLevel Level
	{
		get
		{
			return _level;
		}
		set
		{
			if (value < AspNetHostingPermissionLevel.None || value > AspNetHostingPermissionLevel.Unrestricted)
			{
				throw new ArgumentException(string.Format(global::Locale.GetText("Invalid enum {0}."), value), "Level");
			}
			_level = value;
		}
	}

	public AspNetHostingPermission(AspNetHostingPermissionLevel level)
	{
		Level = level;
	}

	public AspNetHostingPermission(PermissionState state)
	{
		if (PermissionHelper.CheckPermissionState(state, allowUnrestricted: true) == PermissionState.Unrestricted)
		{
			_level = AspNetHostingPermissionLevel.Unrestricted;
		}
		else
		{
			_level = AspNetHostingPermissionLevel.None;
		}
	}

	public bool IsUnrestricted()
	{
		return _level == AspNetHostingPermissionLevel.Unrestricted;
	}

	public override IPermission Copy()
	{
		return new AspNetHostingPermission(_level);
	}

	public override void FromXml(SecurityElement securityElement)
	{
		PermissionHelper.CheckSecurityElement(securityElement, "securityElement", 1, 1);
		if (securityElement.Tag != "IPermission")
		{
			throw new ArgumentException(string.Format(global::Locale.GetText("Invalid tag '{0}' for permission."), securityElement.Tag), "securityElement");
		}
		if (securityElement.Attribute("version") == null)
		{
			throw new ArgumentException(global::Locale.GetText("Missing version attribute."), "securityElement");
		}
		if (PermissionHelper.IsUnrestricted(securityElement))
		{
			_level = AspNetHostingPermissionLevel.Unrestricted;
			return;
		}
		string text = securityElement.Attribute("Level");
		if (text != null)
		{
			_level = (AspNetHostingPermissionLevel)Enum.Parse(typeof(AspNetHostingPermissionLevel), text);
		}
		else
		{
			_level = AspNetHostingPermissionLevel.None;
		}
	}

	public override SecurityElement ToXml()
	{
		SecurityElement securityElement = PermissionHelper.Element(typeof(AspNetHostingPermission), 1);
		if (IsUnrestricted())
		{
			securityElement.AddAttribute("Unrestricted", "true");
		}
		securityElement.AddAttribute("Level", _level.ToString());
		return securityElement;
	}

	public override IPermission Intersect(IPermission target)
	{
		AspNetHostingPermission aspNetHostingPermission = Cast(target);
		if (aspNetHostingPermission == null)
		{
			return null;
		}
		return new AspNetHostingPermission((_level <= aspNetHostingPermission.Level) ? _level : aspNetHostingPermission.Level);
	}

	public override bool IsSubsetOf(IPermission target)
	{
		AspNetHostingPermission aspNetHostingPermission = Cast(target);
		if (aspNetHostingPermission == null)
		{
			return IsEmpty();
		}
		return _level <= aspNetHostingPermission._level;
	}

	public override IPermission Union(IPermission target)
	{
		AspNetHostingPermission aspNetHostingPermission = Cast(target);
		if (aspNetHostingPermission == null)
		{
			return Copy();
		}
		return new AspNetHostingPermission((_level > aspNetHostingPermission.Level) ? _level : aspNetHostingPermission.Level);
	}

	private bool IsEmpty()
	{
		return _level == AspNetHostingPermissionLevel.None;
	}

	private AspNetHostingPermission Cast(IPermission target)
	{
		if (target == null)
		{
			return null;
		}
		AspNetHostingPermission obj = target as AspNetHostingPermission;
		if (obj == null)
		{
			PermissionHelper.ThrowInvalidPermission(target, typeof(AspNetHostingPermission));
		}
		return obj;
	}
}
