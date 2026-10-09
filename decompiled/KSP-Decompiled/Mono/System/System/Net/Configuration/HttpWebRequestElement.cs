using System.Configuration;

namespace System.Net.Configuration;

public sealed class HttpWebRequestElement : ConfigurationElement
{
	private static ConfigurationProperty maximumErrorResponseLengthProp;

	private static ConfigurationProperty maximumResponseHeadersLengthProp;

	private static ConfigurationProperty maximumUnauthorizedUploadLengthProp;

	private static ConfigurationProperty useUnsafeHeaderParsingProp;

	private static ConfigurationPropertyCollection properties;

	[ConfigurationProperty("maximumErrorResponseLength", DefaultValue = "64")]
	public int MaximumErrorResponseLength
	{
		get
		{
			return (int)base[maximumErrorResponseLengthProp];
		}
		set
		{
			base[maximumErrorResponseLengthProp] = value;
		}
	}

	[ConfigurationProperty("maximumResponseHeadersLength", DefaultValue = "64")]
	public int MaximumResponseHeadersLength
	{
		get
		{
			return (int)base[maximumResponseHeadersLengthProp];
		}
		set
		{
			base[maximumResponseHeadersLengthProp] = value;
		}
	}

	[ConfigurationProperty("maximumUnauthorizedUploadLength", DefaultValue = "-1")]
	public int MaximumUnauthorizedUploadLength
	{
		get
		{
			return (int)base[maximumUnauthorizedUploadLengthProp];
		}
		set
		{
			base[maximumUnauthorizedUploadLengthProp] = value;
		}
	}

	[ConfigurationProperty("useUnsafeHeaderParsing", DefaultValue = "False")]
	public bool UseUnsafeHeaderParsing
	{
		get
		{
			return (bool)base[useUnsafeHeaderParsingProp];
		}
		set
		{
			base[useUnsafeHeaderParsingProp] = value;
		}
	}

	protected override ConfigurationPropertyCollection Properties => properties;

	static HttpWebRequestElement()
	{
		maximumErrorResponseLengthProp = new ConfigurationProperty("maximumErrorResponseLength", typeof(int), 64);
		maximumResponseHeadersLengthProp = new ConfigurationProperty("maximumResponseHeadersLength", typeof(int), 64);
		maximumUnauthorizedUploadLengthProp = new ConfigurationProperty("maximumUnauthorizedUploadLength", typeof(int), -1);
		useUnsafeHeaderParsingProp = new ConfigurationProperty("useUnsafeHeaderParsing", typeof(bool), false);
		properties = new ConfigurationPropertyCollection();
		properties.Add(maximumErrorResponseLengthProp);
		properties.Add(maximumResponseHeadersLengthProp);
		properties.Add(maximumUnauthorizedUploadLengthProp);
		properties.Add(useUnsafeHeaderParsingProp);
	}

	[System.MonoTODO]
	protected override void PostDeserialize()
	{
		base.PostDeserialize();
	}
}
