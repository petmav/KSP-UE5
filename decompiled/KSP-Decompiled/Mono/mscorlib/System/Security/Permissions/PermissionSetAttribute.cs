using System.IO;
using System.Runtime.InteropServices;
using System.Security.Policy;
using System.Text;
using Mono.Security.Cryptography;
using Mono.Xml;

namespace System.Security.Permissions;

[Serializable]
[AttributeUsage(AttributeTargets.Assembly | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Constructor | AttributeTargets.Method, AllowMultiple = true, Inherited = false)]
[ComVisible(true)]
public sealed class PermissionSetAttribute : CodeAccessSecurityAttribute
{
	private string file;

	private string name;

	private bool isUnicodeEncoded;

	private string xml;

	private string hex;

	public string File
	{
		get
		{
			return file;
		}
		set
		{
			file = value;
		}
	}

	public string Hex
	{
		get
		{
			return hex;
		}
		set
		{
			hex = value;
		}
	}

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

	public bool UnicodeEncoded
	{
		get
		{
			return isUnicodeEncoded;
		}
		set
		{
			isUnicodeEncoded = value;
		}
	}

	public string XML
	{
		get
		{
			return xml;
		}
		set
		{
			xml = value;
		}
	}

	public PermissionSetAttribute(SecurityAction action)
		: base(action)
	{
	}

	public override IPermission CreatePermission()
	{
		return null;
	}

	private PermissionSet CreateFromXml(string xml)
	{
		SecurityParser securityParser = new SecurityParser();
		try
		{
			securityParser.LoadXml(xml);
		}
		catch (SmallXmlParserException ex)
		{
			throw new XmlSyntaxException(ex.Line, ex.ToString());
		}
		SecurityElement securityElement = securityParser.ToXml();
		string text = securityElement.Attribute("class");
		if (text == null)
		{
			return null;
		}
		PermissionState state = PermissionState.None;
		if (CodeAccessPermission.IsUnrestricted(securityElement))
		{
			state = PermissionState.Unrestricted;
		}
		if (text.EndsWith("NamedPermissionSet"))
		{
			NamedPermissionSet namedPermissionSet = new NamedPermissionSet(securityElement.Attribute("Name"), state);
			namedPermissionSet.FromXml(securityElement);
			return namedPermissionSet;
		}
		if (text.EndsWith("PermissionSet"))
		{
			PermissionSet permissionSet = new PermissionSet(state);
			permissionSet.FromXml(securityElement);
			return permissionSet;
		}
		return null;
	}

	public PermissionSet CreatePermissionSet()
	{
		PermissionSet permissionSet = null;
		if (base.Unrestricted)
		{
			permissionSet = new PermissionSet(PermissionState.Unrestricted);
		}
		else
		{
			permissionSet = new PermissionSet(PermissionState.None);
			if (name != null)
			{
				return PolicyLevel.CreateAppDomainLevel().GetNamedPermissionSet(name);
			}
			if (file != null)
			{
				Encoding encoding = (isUnicodeEncoded ? Encoding.Unicode : Encoding.ASCII);
				using StreamReader streamReader = new StreamReader(file, encoding);
				permissionSet = CreateFromXml(streamReader.ReadToEnd());
			}
			else if (xml != null)
			{
				permissionSet = CreateFromXml(xml);
			}
			else if (hex != null)
			{
				Encoding aSCII = Encoding.ASCII;
				byte[] array = CryptoConvert.FromHex(hex);
				permissionSet = CreateFromXml(aSCII.GetString(array, 0, array.Length));
			}
		}
		return permissionSet;
	}
}
