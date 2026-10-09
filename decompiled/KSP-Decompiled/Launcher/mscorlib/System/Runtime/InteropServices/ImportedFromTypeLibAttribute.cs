namespace System.Runtime.InteropServices;

[ComVisible(true)]
[AttributeUsage(AttributeTargets.Assembly, Inherited = false)]
public sealed class ImportedFromTypeLibAttribute : Attribute
{
	internal string _val;

	public string Value => _val;

	public ImportedFromTypeLibAttribute(string tlbFile)
	{
		_val = tlbFile;
	}
}
