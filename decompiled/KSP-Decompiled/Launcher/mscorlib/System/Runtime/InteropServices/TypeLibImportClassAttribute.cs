namespace System.Runtime.InteropServices;

[ComVisible(true)]
[AttributeUsage(AttributeTargets.Interface, Inherited = false)]
public sealed class TypeLibImportClassAttribute : Attribute
{
	internal string _importClassName;

	public string Value => _importClassName;

	public TypeLibImportClassAttribute(Type importClass)
	{
		_importClassName = importClass.ToString();
	}
}
