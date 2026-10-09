namespace System.Runtime.CompilerServices;

[Serializable]
[AttributeUsage(AttributeTargets.Assembly, AllowMultiple = false)]
public sealed class ReferenceAssemblyAttribute : Attribute
{
	private string _description;

	public string Description => _description;

	public ReferenceAssemblyAttribute()
	{
	}

	public ReferenceAssemblyAttribute(string description)
	{
		_description = description;
	}
}
