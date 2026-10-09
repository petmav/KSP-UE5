using System.Configuration;

namespace System.Net.Configuration;

public sealed class SmtpSpecifiedPickupDirectoryElement : ConfigurationElement
{
	private static ConfigurationProperty pickupDirectoryLocationProp;

	private static ConfigurationPropertyCollection properties;

	[ConfigurationProperty("pickupDirectoryLocation")]
	public string PickupDirectoryLocation
	{
		get
		{
			return (string)base[pickupDirectoryLocationProp];
		}
		set
		{
			base[pickupDirectoryLocationProp] = value;
		}
	}

	protected override ConfigurationPropertyCollection Properties => properties;

	static SmtpSpecifiedPickupDirectoryElement()
	{
		pickupDirectoryLocationProp = new ConfigurationProperty("pickupDirectoryLocation", typeof(string));
		properties = new ConfigurationPropertyCollection();
		properties.Add(pickupDirectoryLocationProp);
	}
}
