using System.Globalization;

namespace System.Security.Permissions;

internal sealed class PermissionHelper
{
	internal static SecurityElement Element(Type type, int version)
	{
		SecurityElement securityElement = new SecurityElement("IPermission");
		securityElement.AddAttribute("class", type.FullName + ", " + type.Assembly.ToString().Replace('"', '\''));
		securityElement.AddAttribute("version", version.ToString());
		return securityElement;
	}

	internal static PermissionState CheckPermissionState(PermissionState state, bool allowUnrestricted)
	{
		switch (state)
		{
		case PermissionState.Unrestricted:
			if (!allowUnrestricted)
			{
				throw new ArgumentException(global::Locale.GetText("Unrestricted isn't not allowed for identity permissions."), "state");
			}
			break;
		default:
			throw new ArgumentException(string.Format(global::Locale.GetText("Invalid enum {0}"), state), "state");
		case PermissionState.None:
			break;
		}
		return state;
	}

	internal static int CheckSecurityElement(SecurityElement se, string parameterName, int minimumVersion, int maximumVersion)
	{
		if (se == null)
		{
			throw new ArgumentNullException(parameterName);
		}
		if (se.Attribute("class") == null)
		{
			throw new ArgumentException(global::Locale.GetText("Missing 'class' attribute."), parameterName);
		}
		int num = minimumVersion;
		string text = se.Attribute("version");
		if (text != null)
		{
			try
			{
				num = int.Parse(text);
			}
			catch (Exception innerException)
			{
				throw new ArgumentException(string.Format(global::Locale.GetText("Couldn't parse version from '{0}'."), text), parameterName, innerException);
			}
		}
		if (num < minimumVersion || num > maximumVersion)
		{
			throw new ArgumentException(string.Format(global::Locale.GetText("Unknown version '{0}', expected versions between ['{1}','{2}']."), num, minimumVersion, maximumVersion), parameterName);
		}
		return num;
	}

	internal static bool IsUnrestricted(SecurityElement se)
	{
		string text = se.Attribute("Unrestricted");
		if (text == null)
		{
			return false;
		}
		return string.Compare(text, bool.TrueString, ignoreCase: true, CultureInfo.InvariantCulture) == 0;
	}

	internal static void ThrowInvalidPermission(IPermission target, Type expected)
	{
		throw new ArgumentException(string.Format(global::Locale.GetText("Invalid permission type '{0}', expected type '{1}'."), target.GetType(), expected), "target");
	}
}
