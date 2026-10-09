using System.Security;
using System.Security.Permissions;

namespace System.Configuration;

[Serializable]
public sealed class ConfigurationPermission : CodeAccessPermission, IUnrestrictedPermission
{
	private bool unrestricted;

	public ConfigurationPermission(PermissionState state)
	{
		unrestricted = state == PermissionState.Unrestricted;
	}

	public override IPermission Copy()
	{
		return new ConfigurationPermission(unrestricted ? PermissionState.Unrestricted : PermissionState.None);
	}

	public override void FromXml(SecurityElement securityElement)
	{
		if (securityElement == null)
		{
			throw new ArgumentNullException("securityElement");
		}
		if (securityElement.Tag != "IPermission")
		{
			throw new ArgumentException("securityElement");
		}
		string text = securityElement.Attribute("Unrestricted");
		if (text != null)
		{
			unrestricted = string.Compare(text, "true", StringComparison.InvariantCultureIgnoreCase) == 0;
		}
	}

	public override IPermission Intersect(IPermission target)
	{
		if (target == null)
		{
			return null;
		}
		if (!(target is ConfigurationPermission configurationPermission))
		{
			throw new ArgumentException("target");
		}
		return new ConfigurationPermission((unrestricted && configurationPermission.IsUnrestricted()) ? PermissionState.Unrestricted : PermissionState.None);
	}

	public override IPermission Union(IPermission target)
	{
		if (target == null)
		{
			return Copy();
		}
		if (!(target is ConfigurationPermission configurationPermission))
		{
			throw new ArgumentException("target");
		}
		return new ConfigurationPermission((unrestricted || configurationPermission.IsUnrestricted()) ? PermissionState.Unrestricted : PermissionState.None);
	}

	public override bool IsSubsetOf(IPermission target)
	{
		if (target == null)
		{
			return !unrestricted;
		}
		if (!(target is ConfigurationPermission configurationPermission))
		{
			throw new ArgumentException("target");
		}
		if (unrestricted)
		{
			return configurationPermission.IsUnrestricted();
		}
		return true;
	}

	public bool IsUnrestricted()
	{
		return unrestricted;
	}

	public override SecurityElement ToXml()
	{
		SecurityElement securityElement = new SecurityElement("IPermission");
		securityElement.AddAttribute("class", GetType().AssemblyQualifiedName);
		securityElement.AddAttribute("version", "1");
		if (unrestricted)
		{
			securityElement.AddAttribute("Unrestricted", "true");
		}
		return securityElement;
	}
}
