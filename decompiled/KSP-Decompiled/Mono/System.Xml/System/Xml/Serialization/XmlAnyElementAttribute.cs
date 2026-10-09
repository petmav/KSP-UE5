namespace System.Xml.Serialization;

[AttributeUsage(AttributeTargets.Property | AttributeTargets.Field | AttributeTargets.Parameter | AttributeTargets.ReturnValue, AllowMultiple = true)]
public class XmlAnyElementAttribute : Attribute
{
	private string name;

	private string ns;

	private int order = -1;

	private bool nsSpecified;

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

	public string Namespace
	{
		get
		{
			return ns;
		}
		set
		{
			ns = value;
			nsSpecified = true;
		}
	}

	public int Order
	{
		get
		{
			return order;
		}
		set
		{
			if (value < 0)
			{
				throw new ArgumentException(Res.GetString("Negative values are prohibited."), "Order");
			}
			order = value;
		}
	}

	internal bool NamespaceSpecified => nsSpecified;

	public XmlAnyElementAttribute()
	{
	}

	public XmlAnyElementAttribute(string name)
	{
		this.name = name;
	}

	public XmlAnyElementAttribute(string name, string ns)
	{
		this.name = name;
		this.ns = ns;
		nsSpecified = true;
	}
}
