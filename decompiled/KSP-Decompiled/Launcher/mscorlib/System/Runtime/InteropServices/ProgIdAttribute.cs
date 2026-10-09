namespace System.Runtime.InteropServices;

[ComVisible(true)]
[AttributeUsage(AttributeTargets.Class, Inherited = false)]
public sealed class ProgIdAttribute : Attribute
{
	internal string _val;

	public string Value => _val;

	public ProgIdAttribute(string progId)
	{
		_val = progId;
	}
}
