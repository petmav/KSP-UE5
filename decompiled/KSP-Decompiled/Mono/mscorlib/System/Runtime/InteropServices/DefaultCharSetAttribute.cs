namespace System.Runtime.InteropServices;

[ComVisible(true)]
[AttributeUsage(AttributeTargets.Module, Inherited = false)]
public sealed class DefaultCharSetAttribute : Attribute
{
	internal CharSet _CharSet;

	public CharSet CharSet => _CharSet;

	public DefaultCharSetAttribute(CharSet charSet)
	{
		_CharSet = charSet;
	}
}
