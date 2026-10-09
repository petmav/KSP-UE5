namespace System.Runtime.InteropServices;

[ComVisible(true)]
[AttributeUsage(AttributeTargets.Assembly | AttributeTargets.Class, Inherited = false)]
public sealed class ClassInterfaceAttribute : Attribute
{
	internal ClassInterfaceType _val;

	public ClassInterfaceType Value => _val;

	public ClassInterfaceAttribute(ClassInterfaceType classInterfaceType)
	{
		_val = classInterfaceType;
	}

	public ClassInterfaceAttribute(short classInterfaceType)
	{
		_val = (ClassInterfaceType)classInterfaceType;
	}
}
