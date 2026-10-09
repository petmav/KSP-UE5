using System.Security;
using System.Security.Permissions;

namespace System.Net.Mail;

[Serializable]
[AttributeUsage(AttributeTargets.Assembly | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Constructor | AttributeTargets.Method, AllowMultiple = true, Inherited = false)]
public sealed class SmtpPermissionAttribute : CodeAccessSecurityAttribute
{
	private string access;

	public string Access
	{
		get
		{
			return access;
		}
		set
		{
			access = value;
		}
	}

	public SmtpPermissionAttribute(SecurityAction action)
		: base(action)
	{
	}

	private SmtpAccess GetSmtpAccess()
	{
		if (access == null)
		{
			return SmtpAccess.None;
		}
		switch (access.ToLowerInvariant())
		{
		case "connecttounrestrictedport":
			return SmtpAccess.ConnectToUnrestrictedPort;
		case "connect":
			return SmtpAccess.Connect;
		case "none":
			return SmtpAccess.None;
		default:
		{
			string text = global::Locale.GetText("Invalid Access='{0}' value.", access);
			throw new ArgumentException("Access", text);
		}
		}
	}

	public override IPermission CreatePermission()
	{
		if (base.Unrestricted)
		{
			return new SmtpPermission(unrestricted: true);
		}
		return new SmtpPermission(GetSmtpAccess());
	}
}
