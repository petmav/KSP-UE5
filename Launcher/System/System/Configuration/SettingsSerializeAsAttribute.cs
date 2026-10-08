using Unity;

namespace System.Configuration;

[AttributeUsage(AttributeTargets.Class | AttributeTargets.Property)]
public sealed class SettingsSerializeAsAttribute : Attribute
{
	public SettingsSerializeAs SerializeAs
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(SettingsSerializeAs);
		}
	}

	public SettingsSerializeAsAttribute(SettingsSerializeAs serializeAs)
	{
	}
}
