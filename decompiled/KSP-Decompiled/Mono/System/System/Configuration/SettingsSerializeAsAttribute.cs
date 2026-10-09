namespace System.Configuration;

[AttributeUsage(AttributeTargets.Class | AttributeTargets.Property)]
public sealed class SettingsSerializeAsAttribute : Attribute
{
	private SettingsSerializeAs serializeAs;

	public SettingsSerializeAs SerializeAs => serializeAs;

	public SettingsSerializeAsAttribute(SettingsSerializeAs serializeAs)
	{
		this.serializeAs = serializeAs;
	}
}
