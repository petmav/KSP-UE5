namespace System.Xml.Serialization;

[AttributeUsage(AttributeTargets.Field)]
public class XmlEnumAttribute : Attribute
{
	private string name;

	public string Name
	{
		get
		{
			return name;
		}
		set
		{
			name = value;
		}
	}

	public XmlEnumAttribute()
	{
	}

	public XmlEnumAttribute(string name)
	{
		this.name = name;
	}
}
