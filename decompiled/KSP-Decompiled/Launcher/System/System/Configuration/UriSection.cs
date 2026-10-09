using Unity;

namespace System.Configuration;

public sealed class UriSection : ConfigurationSection
{
	public IdnElement Idn
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public IriParsingElement IriParsing
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SchemeSettingElementCollection SchemeSettings
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public UriSection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
