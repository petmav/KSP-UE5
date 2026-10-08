using System.Configuration.Internal;
using System.Runtime.CompilerServices;
using System.Security.Permissions;
using System.Xml;
using Unity;

namespace System.Configuration;

[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
public sealed class ConfigXmlDocument : XmlDocument, IConfigErrorInfo
{
	public string Filename
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public int LineNumber
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public ConfigXmlDocument()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SpecialName]
	string IConfigErrorInfo.get_Filename()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SpecialName]
	int IConfigErrorInfo.get_LineNumber()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void LoadSingleElement(string filename, XmlTextReader sourceReader)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
