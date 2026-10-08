namespace System.Runtime.CompilerServices;

[AttributeUsage(AttributeTargets.Field, Inherited = false)]
public sealed class FixedBufferAttribute : Attribute
{
	private Type elementType;

	private int length;

	public Type ElementType => elementType;

	public int Length => length;

	public FixedBufferAttribute(Type elementType, int length)
	{
		this.elementType = elementType;
		this.length = length;
	}
}
