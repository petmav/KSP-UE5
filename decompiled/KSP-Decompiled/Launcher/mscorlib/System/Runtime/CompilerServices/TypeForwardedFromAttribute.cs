namespace System.Runtime.CompilerServices;

[AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Interface | AttributeTargets.Delegate, Inherited = false, AllowMultiple = false)]
public sealed class TypeForwardedFromAttribute : Attribute
{
	private string assemblyFullName;

	public string AssemblyFullName => assemblyFullName;

	private TypeForwardedFromAttribute()
	{
	}

	public TypeForwardedFromAttribute(string assemblyFullName)
	{
		if (string.IsNullOrEmpty(assemblyFullName))
		{
			throw new ArgumentNullException("assemblyFullName");
		}
		this.assemblyFullName = assemblyFullName;
	}
}
