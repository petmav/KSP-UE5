using System.Configuration;
using Unity;

namespace System.Net.Configuration;

public sealed class ModuleElement : ConfigurationElement
{
	protected override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string Type
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

	public ModuleElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
