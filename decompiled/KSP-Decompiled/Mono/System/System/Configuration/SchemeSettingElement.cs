using Unity;

namespace System.Configuration;

public sealed class SchemeSettingElement : ConfigurationElement
{
	public GenericUriParserOptions GenericUriParserOptions
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(GenericUriParserOptions);
		}
	}

	public string Name
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SchemeSettingElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
