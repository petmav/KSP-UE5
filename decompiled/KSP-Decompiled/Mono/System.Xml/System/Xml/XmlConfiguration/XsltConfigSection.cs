using System.ComponentModel;
using System.Configuration;

namespace System.Xml.XmlConfiguration;

[EditorBrowsable(EditorBrowsableState.Never)]
public sealed class XsltConfigSection : ConfigurationSection
{
	[ConfigurationProperty("prohibitDefaultResolver", DefaultValue = "false")]
	public string ProhibitDefaultResolverString
	{
		get
		{
			return (string)base["prohibitDefaultResolver"];
		}
		set
		{
			base["prohibitDefaultResolver"] = value;
		}
	}

	private bool _ProhibitDefaultResolver
	{
		get
		{
			XmlConvert.TryToBoolean(ProhibitDefaultResolverString, out var result);
			return result;
		}
	}

	private static bool s_ProhibitDefaultUrlResolver
	{
		get
		{
			if (!(ConfigurationManager.GetSection(XmlConfigurationString.XsltSectionPath) is XsltConfigSection xsltConfigSection))
			{
				return false;
			}
			return xsltConfigSection._ProhibitDefaultResolver;
		}
	}

	[ConfigurationProperty("limitXPathComplexity", DefaultValue = "true")]
	internal string LimitXPathComplexityString
	{
		get
		{
			return (string)base["limitXPathComplexity"];
		}
		set
		{
			base["limitXPathComplexity"] = value;
		}
	}

	private bool _LimitXPathComplexity
	{
		get
		{
			string limitXPathComplexityString = LimitXPathComplexityString;
			bool result = true;
			XmlConvert.TryToBoolean(limitXPathComplexityString, out result);
			return result;
		}
	}

	internal static bool LimitXPathComplexity
	{
		get
		{
			if (!(ConfigurationManager.GetSection(XmlConfigurationString.XsltSectionPath) is XsltConfigSection xsltConfigSection))
			{
				return true;
			}
			return xsltConfigSection._LimitXPathComplexity;
		}
	}

	[ConfigurationProperty("enableMemberAccessForXslCompiledTransform", DefaultValue = "False")]
	internal string EnableMemberAccessForXslCompiledTransformString
	{
		get
		{
			return (string)base["enableMemberAccessForXslCompiledTransform"];
		}
		set
		{
			base["enableMemberAccessForXslCompiledTransform"] = value;
		}
	}

	private bool _EnableMemberAccessForXslCompiledTransform
	{
		get
		{
			string enableMemberAccessForXslCompiledTransformString = EnableMemberAccessForXslCompiledTransformString;
			bool result = false;
			XmlConvert.TryToBoolean(enableMemberAccessForXslCompiledTransformString, out result);
			return result;
		}
	}

	internal static bool EnableMemberAccessForXslCompiledTransform
	{
		get
		{
			if (!(ConfigurationManager.GetSection(XmlConfigurationString.XsltSectionPath) is XsltConfigSection xsltConfigSection))
			{
				return false;
			}
			return xsltConfigSection._EnableMemberAccessForXslCompiledTransform;
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
