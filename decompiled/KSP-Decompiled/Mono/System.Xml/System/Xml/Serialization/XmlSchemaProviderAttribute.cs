namespace System.Xml.Serialization;

[AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Interface)]
public sealed class XmlSchemaProviderAttribute : Attribute
{
	private string methodName;

	private bool any;

	public string MethodName => methodName;

	public bool IsAny
	{
		get
		{
			return any;
		}
		set
		{
			any = value;
		}
	}

	public XmlSchemaProviderAttribute(string methodName)
	{
		this.methodName = methodName;
	}
}
