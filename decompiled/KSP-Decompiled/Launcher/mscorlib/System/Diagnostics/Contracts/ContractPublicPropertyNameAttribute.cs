namespace System.Diagnostics.Contracts;

[AttributeUsage(AttributeTargets.Field)]
[Conditional("CONTRACTS_FULL")]
public sealed class ContractPublicPropertyNameAttribute : Attribute
{
	private string _publicName;

	public string Name => _publicName;

	public ContractPublicPropertyNameAttribute(string name)
	{
		_publicName = name;
	}
}
