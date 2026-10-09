using System.Configuration;
using System.Net.Mail;
using Unity;

namespace System.Net.Configuration;

public sealed class SmtpSection : ConfigurationSection
{
	public SmtpDeliveryFormat DeliveryFormat
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(SmtpDeliveryFormat);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public SmtpDeliveryMethod DeliveryMethod
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(SmtpDeliveryMethod);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public string From
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

	public SmtpNetworkElement Network
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

	public SmtpSpecifiedPickupDirectoryElement SpecifiedPickupDirectory
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SmtpSection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
