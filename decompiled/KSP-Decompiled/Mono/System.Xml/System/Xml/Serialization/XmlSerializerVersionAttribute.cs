namespace System.Xml.Serialization;

[AttributeUsage(AttributeTargets.Assembly)]
public sealed class XmlSerializerVersionAttribute : Attribute
{
	private string mvid;

	private string serializerVersion;

	private string ns;

	private Type type;

	public string ParentAssemblyId
	{
		get
		{
			return mvid;
		}
		set
		{
			mvid = value;
		}
	}

	public string Version
	{
		get
		{
			return serializerVersion;
		}
		set
		{
			serializerVersion = value;
		}
	}

	public string Namespace
	{
		get
		{
			return ns;
		}
		set
		{
			ns = value;
		}
	}

	public Type Type
	{
		get
		{
			return type;
		}
		set
		{
			type = value;
		}
	}

	public XmlSerializerVersionAttribute()
	{
	}

	public XmlSerializerVersionAttribute(Type type)
	{
		this.type = type;
	}
}
