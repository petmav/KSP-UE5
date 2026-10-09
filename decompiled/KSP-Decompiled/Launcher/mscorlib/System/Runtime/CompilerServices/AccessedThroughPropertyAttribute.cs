using System.Runtime.InteropServices;

namespace System.Runtime.CompilerServices;

[ComVisible(true)]
[AttributeUsage(AttributeTargets.Field)]
public sealed class AccessedThroughPropertyAttribute : Attribute
{
	private readonly string propertyName;

	public string PropertyName => propertyName;

	public AccessedThroughPropertyAttribute(string propertyName)
	{
		this.propertyName = propertyName;
	}
}
