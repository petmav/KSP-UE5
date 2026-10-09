using System.ComponentModel;
using Unity;

namespace System.Xml.XmlConfiguration;

[EditorBrowsable(EditorBrowsableState.Never)]
public sealed class XsltConfigSection
{
	private static bool s_ProhibitDefaultUrlResolver => false;

	internal static bool LimitXPathComplexity => true;

	internal static bool EnableMemberAccessForXslCompiledTransform => false;

	public string ProhibitDefaultResolverString
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	internal static XmlResolver CreateDefaultResolver()
	{
		if (s_ProhibitDefaultUrlResolver)
		{
			return XmlNullResolver.Singleton;
		}
		return new XmlUrlResolver();
	}
}
