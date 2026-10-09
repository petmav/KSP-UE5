using System.Configuration;
using Unity;

namespace System.Net.Configuration;

public sealed class MailSettingsSectionGroup : ConfigurationSectionGroup
{
	public SmtpSection Smtp
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public MailSettingsSectionGroup()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
