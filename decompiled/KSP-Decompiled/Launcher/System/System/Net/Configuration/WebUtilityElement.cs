using System.Configuration;
using Unity;

namespace System.Net.Configuration;

public sealed class WebUtilityElement : ConfigurationElement
{
	protected override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public UnicodeDecodingConformance UnicodeDecodingConformance
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(UnicodeDecodingConformance);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public UnicodeEncodingConformance UnicodeEncodingConformance
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(UnicodeEncodingConformance);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public WebUtilityElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
