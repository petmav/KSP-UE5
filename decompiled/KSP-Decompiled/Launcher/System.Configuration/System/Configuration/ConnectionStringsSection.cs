using Unity;

namespace System.Configuration;

public sealed class ConnectionStringsSection : ConfigurationSection
{
	public ConnectionStringSettingsCollection ConnectionStrings
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected internal override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ConnectionStringsSection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal override object GetRuntimeObject()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
