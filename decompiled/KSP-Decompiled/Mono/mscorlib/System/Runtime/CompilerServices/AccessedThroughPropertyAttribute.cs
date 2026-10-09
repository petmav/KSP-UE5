using System.Runtime.InteropServices;

namespace System.Runtime.CompilerServices;

[AttributeUsage(AttributeTargets.Field)]
[ComVisible(true)]
public sealed class AccessedThroughPropertyAttribute : Attribute
{
	private readonly string propertyName;

	public string PropertyName => propertyName;

	public AccessedThroughPropertyAttribute(string propertyName)
	{
		this.propertyName = propertyName;
	}
}
