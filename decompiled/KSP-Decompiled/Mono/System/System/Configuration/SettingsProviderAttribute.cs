namespace System.Configuration;

[AttributeUsage(AttributeTargets.Class | AttributeTargets.Property)]
public sealed class SettingsProviderAttribute : Attribute
{
	private string providerTypeName;

	public string ProviderTypeName => providerTypeName;

	public SettingsProviderAttribute(string providerTypeName)
	{
		if (providerTypeName == null)
		{
			throw new ArgumentNullException("providerTypeName");
		}
		this.providerTypeName = providerTypeName;
	}

	public SettingsProviderAttribute(Type providerType)
	{
		if (providerType == null)
		{
			throw new ArgumentNullException("providerType");
		}
		providerTypeName = providerType.AssemblyQualifiedName;
	}
}
