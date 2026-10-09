using System.Xml.Schema;

namespace System.Xml.Serialization;

internal abstract class Accessor
{
	private string name;

	private object defaultValue;

	private string ns;

	private TypeMapping mapping;

	private bool any;

	private string anyNs;

	private bool topLevelInSchema;

	private bool isFixed;

	private bool isOptional;

	private XmlSchemaForm form;

	internal TypeMapping Mapping
	{
		get
		{
			return mapping;
		}
		set
		{
			mapping = value;
		}
	}

	internal object Default
	{
		get
		{
			return defaultValue;
		}
		set
		{
			defaultValue = value;
		}
	}

	internal bool HasDefault
	{
		get
		{
			if (defaultValue != null)
			{
				return defaultValue != DBNull.Value;
			}
			return false;
		}
	}

	internal virtual string Name
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

	internal bool Any
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

	internal string AnyNamespaces
	{
		get
		{
			return anyNs;
		}
		set
		{
			anyNs = value;
		}
	}

	internal string Namespace
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

	internal XmlSchemaForm Form
	{
		get
		{
			return form;
		}
		set
		{
			form = value;
		}
	}

	internal bool IsFixed
	{
		get
		{
			return isFixed;
		}
		set
		{
			isFixed = value;
		}
	}

	internal bool IsOptional
	{
		get
		{
			return isOptional;
		}
		set
		{
			isOptional = value;
		}
	}

	internal bool IsTopLevelInSchema
	{
		get
		{
			return topLevelInSchema;
		}
		set
		{
			topLevelInSchema = value;
		}
	}

	internal Accessor()
	{
	}

	internal static string EscapeName(string name)
	{
		if (name == null || name.Length == 0)
		{
			return name;
		}
		return XmlConvert.EncodeLocalName(name);
	}

	internal static string EscapeQName(string name)
	{
		if (name == null || name.Length == 0)
		{
			return name;
		}
		int num = name.LastIndexOf(':');
		if (num < 0)
		{
			return XmlConvert.EncodeLocalName(name);
		}
		if (num == 0 || num == name.Length - 1)
		{
			throw new ArgumentException(Res.GetString("Invalid name character in '{0}'.", name), "name");
		}
		return new XmlQualifiedName(XmlConvert.EncodeLocalName(name.Substring(num + 1)), XmlConvert.EncodeLocalName(name.Substring(0, num))).ToString();
	}

	internal static string UnescapeName(string name)
	{
		return XmlConvert.DecodeName(name);
	}

	internal string ToString(string defaultNs)
	{
		if (Any)
		{
			return ((Namespace == null) ? "##any" : Namespace) + ":" + Name;
		}
		if (!(Namespace == defaultNs))
		{
			return Namespace + ":" + Name;
		}
		return Name;
	}
}
