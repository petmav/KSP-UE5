namespace System.Runtime.InteropServices;

[ComVisible(true)]
[AttributeUsage(AttributeTargets.Method, Inherited = false)]
public sealed class LCIDConversionAttribute : Attribute
{
	internal int _val;

	public int Value => _val;

	public LCIDConversionAttribute(int lcid)
	{
		_val = lcid;
	}
}
