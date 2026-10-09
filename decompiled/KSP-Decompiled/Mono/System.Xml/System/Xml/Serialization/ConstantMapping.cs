namespace System.Xml.Serialization;

internal class ConstantMapping : Mapping
{
	private string xmlName;

	private string name;

	private long value;

	internal string XmlName
	{
		get
		{
			if (xmlName != null)
			{
				return xmlName;
			}
			return string.Empty;
		}
		set
		{
			xmlName = value;
		}
	}

	internal string Name
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

	internal long Value
	{
		get
		{
			return value;
		}
		set
		{
			this.value = value;
		}
	}
}
