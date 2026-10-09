namespace System.Diagnostics.Contracts;

[AttributeUsage(AttributeTargets.Assembly | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Constructor | AttributeTargets.Method | AttributeTargets.Property)]
[Conditional("CONTRACTS_FULL")]
public sealed class ContractVerificationAttribute : Attribute
{
	private bool _value;

	public bool Value => _value;

	public ContractVerificationAttribute(bool value)
	{
		_value = value;
	}
}
