using Unity;

namespace System.Configuration;

public sealed class IriParsingElement : ConfigurationElement
{
	public bool Enabled
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
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

	public IriParsingElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
