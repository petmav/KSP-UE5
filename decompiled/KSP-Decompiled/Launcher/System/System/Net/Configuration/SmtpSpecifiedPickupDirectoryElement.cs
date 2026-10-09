using System.Configuration;
using Unity;

namespace System.Net.Configuration;

public sealed class SmtpSpecifiedPickupDirectoryElement : ConfigurationElement
{
	public string PickupDirectoryLocation
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

	protected override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SmtpSpecifiedPickupDirectoryElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
