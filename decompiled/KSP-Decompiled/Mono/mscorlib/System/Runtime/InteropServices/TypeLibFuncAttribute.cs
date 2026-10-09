namespace System.Runtime.InteropServices;

[ComVisible(true)]
[AttributeUsage(AttributeTargets.Method, Inherited = false)]
public sealed class TypeLibFuncAttribute : Attribute
{
	internal TypeLibFuncFlags _val;

	public TypeLibFuncFlags Value => _val;

	public TypeLibFuncAttribute(TypeLibFuncFlags flags)
	{
		_val = flags;
	}

	public TypeLibFuncAttribute(short flags)
	{
		_val = (TypeLibFuncFlags)flags;
	}
}
