namespace System.Diagnostics.Contracts;

[Conditional("CONTRACTS_FULL")]
[AttributeUsage(AttributeTargets.All, AllowMultiple = true, Inherited = false)]
public sealed class ContractOptionAttribute : Attribute
{
	private string _category;

	private string _setting;

	private bool _enabled;

	private string _value;

	public string Category => _category;

	public string Setting => _setting;

	public bool Enabled => _enabled;

	public string Value => _value;

	public ContractOptionAttribute(string category, string setting, bool enabled)
	{
		_category = category;
		_setting = setting;
		_enabled = enabled;
	}

	public ContractOptionAttribute(string category, string setting, string value)
	{
		_category = category;
		_setting = setting;
		_value = value;
	}
}
