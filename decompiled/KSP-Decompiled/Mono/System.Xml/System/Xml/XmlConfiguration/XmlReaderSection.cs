using System.ComponentModel;
using System.Configuration;

namespace System.Xml.XmlConfiguration;

[EditorBrowsable(EditorBrowsableState.Never)]
public sealed class XmlReaderSection : ConfigurationSection
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

	internal static bool ProhibitDefaultUrlResolver
	{
		get
		{
			if (!(ConfigurationManager.GetSection(XmlConfigurationString.XmlReaderSectionPath) is XmlReaderSection xmlReaderSection))
			{
				return false;
			}
			return xmlReaderSection._ProhibitDefaultResolver;
		}
	}

	[ConfigurationProperty("CollapseWhiteSpaceIntoEmptyString", DefaultValue = "false")]
	public string CollapseWhiteSpaceIntoEmptyStringString
	{
		get
		{
			return (string)base["CollapseWhiteSpaceIntoEmptyString"];
		}
		set
		{
			base["CollapseWhiteSpaceIntoEmptyString"] = value;
		}
	}

	private bool _CollapseWhiteSpaceIntoEmptyString
	{
		get
		{
			XmlConvert.TryToBoolean(CollapseWhiteSpaceIntoEmptyStringString, out var result);
			return result;
		}
	}

	internal static bool CollapseWhiteSpaceIntoEmptyString
	{
		get
		{
			if (!(ConfigurationManager.GetSection(XmlConfigurationString.XmlReaderSectionPath) is XmlReaderSection xmlReaderSection))
			{
				return false;
			}
			return xmlReaderSection._CollapseWhiteSpaceIntoEmptyString;
		}
	}

	internal static XmlResolver CreateDefaultResolver()
	{
		if (ProhibitDefaultUrlResolver)
		{
			return null;
		}
		return new XmlUrlResolver();
	}
}
