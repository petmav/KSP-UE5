namespace System.Xml.Serialization;

[AttributeUsage(AttributeTargets.Field)]
public class SoapEnumAttribute : Attribute
{
	private string name;

	public string Name
	{
		get
		{
			if (name != null)
			{
				return name;
			}
			return string.Empty;
		}
		set
		{
			name = value;
		}
	}

	public SoapEnumAttribute()
	{
	}

	public SoapEnumAttribute(string name)
	{
		this.name = name;
	}
}
