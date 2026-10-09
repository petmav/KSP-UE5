using System.ComponentModel;
using System.Reflection;

namespace System.Xml.Serialization;

public class XmlAttributes
{
	private XmlElementAttributes xmlElements = new XmlElementAttributes();

	private XmlArrayItemAttributes xmlArrayItems = new XmlArrayItemAttributes();

	private XmlAnyElementAttributes xmlAnyElements = new XmlAnyElementAttributes();

	private XmlArrayAttribute xmlArray;

	private XmlAttributeAttribute xmlAttribute;

	private XmlTextAttribute xmlText;

	private XmlEnumAttribute xmlEnum;

	private bool xmlIgnore;

	private bool xmlns;

	private object xmlDefaultValue;

	private XmlRootAttribute xmlRoot;

	private XmlTypeAttribute xmlType;

	private XmlAnyAttributeAttribute xmlAnyAttribute;

	private XmlChoiceIdentifierAttribute xmlChoiceIdentifier;

	private static volatile Type ignoreAttributeType;

	internal XmlAttributeFlags XmlFlags
	{
		get
		{
			XmlAttributeFlags xmlAttributeFlags = (XmlAttributeFlags)0;
			if (xmlElements.Count > 0)
			{
				xmlAttributeFlags |= XmlAttributeFlags.Elements;
			}
			if (xmlArrayItems.Count > 0)
			{
				xmlAttributeFlags |= XmlAttributeFlags.ArrayItems;
			}
			if (xmlAnyElements.Count > 0)
			{
				xmlAttributeFlags |= XmlAttributeFlags.AnyElements;
			}
			if (xmlArray != null)
			{
				xmlAttributeFlags |= XmlAttributeFlags.Array;
			}
			if (xmlAttribute != null)
			{
				xmlAttributeFlags |= XmlAttributeFlags.Attribute;
			}
			if (xmlText != null)
			{
				xmlAttributeFlags |= XmlAttributeFlags.Text;
			}
			if (xmlEnum != null)
			{
				xmlAttributeFlags |= XmlAttributeFlags.Enum;
			}
			if (xmlRoot != null)
			{
				xmlAttributeFlags |= XmlAttributeFlags.Root;
			}
			if (xmlType != null)
			{
				xmlAttributeFlags |= XmlAttributeFlags.Type;
			}
			if (xmlAnyAttribute != null)
			{
				xmlAttributeFlags |= XmlAttributeFlags.AnyAttribute;
			}
			if (xmlChoiceIdentifier != null)
			{
				xmlAttributeFlags |= XmlAttributeFlags.ChoiceIdentifier;
			}
			if (xmlns)
			{
				xmlAttributeFlags |= XmlAttributeFlags.XmlnsDeclarations;
			}
			return xmlAttributeFlags;
		}
	}

	private static Type IgnoreAttribute
	{
		get
		{
			if (ignoreAttributeType == null)
			{
				ignoreAttributeType = typeof(object).Assembly.GetType("System.XmlIgnoreMemberAttribute");
				if (ignoreAttributeType == null)
				{
					ignoreAttributeType = typeof(XmlIgnoreAttribute);
				}
			}
			return ignoreAttributeType;
		}
	}

	public XmlElementAttributes XmlElements => xmlElements;

	public XmlAttributeAttribute XmlAttribute
	{
		get
		{
			return xmlAttribute;
		}
		set
		{
			xmlAttribute = value;
		}
	}

	public XmlEnumAttribute XmlEnum
	{
		get
		{
			return xmlEnum;
		}
		set
		{
			xmlEnum = value;
		}
	}

	public XmlTextAttribute XmlText
	{
		get
		{
			return xmlText;
		}
		set
		{
			xmlText = value;
		}
	}

	public XmlArrayAttribute XmlArray
	{
		get
		{
			return xmlArray;
		}
		set
		{
			xmlArray = value;
		}
	}

	public XmlArrayItemAttributes XmlArrayItems => xmlArrayItems;

	public object XmlDefaultValue
	{
		get
		{
			return xmlDefaultValue;
		}
		set
		{
			xmlDefaultValue = value;
		}
	}

	public bool XmlIgnore
	{
		get
		{
			return xmlIgnore;
		}
		set
		{
			xmlIgnore = value;
		}
	}

	public XmlTypeAttribute XmlType
	{
		get
		{
			return xmlType;
		}
		set
		{
			xmlType = value;
		}
	}

	public XmlRootAttribute XmlRoot
	{
		get
		{
			return xmlRoot;
		}
		set
		{
			xmlRoot = value;
		}
	}

	public XmlAnyElementAttributes XmlAnyElements => xmlAnyElements;

	public XmlAnyAttributeAttribute XmlAnyAttribute
	{
		get
		{
			return xmlAnyAttribute;
		}
		set
		{
			xmlAnyAttribute = value;
		}
	}

	public XmlChoiceIdentifierAttribute XmlChoiceIdentifier => xmlChoiceIdentifier;

	public bool Xmlns
	{
		get
		{
			return xmlns;
		}
		set
		{
			xmlns = value;
		}
	}

	public XmlAttributes()
	{
	}

	public XmlAttributes(ICustomAttributeProvider provider)
	{
		object[] customAttributes = provider.GetCustomAttributes(inherit: false);
		XmlAnyElementAttribute xmlAnyElementAttribute = null;
		for (int i = 0; i < customAttributes.Length; i++)
		{
			if (customAttributes[i] is XmlIgnoreAttribute || customAttributes[i] is ObsoleteAttribute || customAttributes[i].GetType() == IgnoreAttribute)
			{
				xmlIgnore = true;
				break;
			}
			if (customAttributes[i] is XmlElementAttribute)
			{
				xmlElements.Add((XmlElementAttribute)customAttributes[i]);
			}
			else if (customAttributes[i] is XmlArrayItemAttribute)
			{
				xmlArrayItems.Add((XmlArrayItemAttribute)customAttributes[i]);
			}
			else if (customAttributes[i] is XmlAnyElementAttribute)
			{
				XmlAnyElementAttribute xmlAnyElementAttribute2 = (XmlAnyElementAttribute)customAttributes[i];
				if ((xmlAnyElementAttribute2.Name == null || xmlAnyElementAttribute2.Name.Length == 0) && xmlAnyElementAttribute2.NamespaceSpecified && xmlAnyElementAttribute2.Namespace == null)
				{
					xmlAnyElementAttribute = xmlAnyElementAttribute2;
				}
				else
				{
					xmlAnyElements.Add((XmlAnyElementAttribute)customAttributes[i]);
				}
			}
			else if (customAttributes[i] is DefaultValueAttribute)
			{
				xmlDefaultValue = ((DefaultValueAttribute)customAttributes[i]).Value;
			}
			else if (customAttributes[i] is XmlAttributeAttribute)
			{
				xmlAttribute = (XmlAttributeAttribute)customAttributes[i];
			}
			else if (customAttributes[i] is XmlArrayAttribute)
			{
				xmlArray = (XmlArrayAttribute)customAttributes[i];
			}
			else if (customAttributes[i] is XmlTextAttribute)
			{
				xmlText = (XmlTextAttribute)customAttributes[i];
			}
			else if (customAttributes[i] is XmlEnumAttribute)
			{
				xmlEnum = (XmlEnumAttribute)customAttributes[i];
			}
			else if (customAttributes[i] is XmlRootAttribute)
			{
				xmlRoot = (XmlRootAttribute)customAttributes[i];
			}
			else if (customAttributes[i] is XmlTypeAttribute)
			{
				xmlType = (XmlTypeAttribute)customAttributes[i];
			}
			else if (customAttributes[i] is XmlAnyAttributeAttribute)
			{
				xmlAnyAttribute = (XmlAnyAttributeAttribute)customAttributes[i];
			}
			else if (customAttributes[i] is XmlChoiceIdentifierAttribute)
			{
				xmlChoiceIdentifier = (XmlChoiceIdentifierAttribute)customAttributes[i];
			}
			else if (customAttributes[i] is XmlNamespaceDeclarationsAttribute)
			{
				xmlns = true;
			}
		}
		if (xmlIgnore)
		{
			xmlElements.Clear();
			xmlArrayItems.Clear();
			xmlAnyElements.Clear();
			xmlDefaultValue = null;
			xmlAttribute = null;
			xmlArray = null;
			xmlText = null;
			xmlEnum = null;
			xmlType = null;
			xmlAnyAttribute = null;
			xmlChoiceIdentifier = null;
			xmlns = false;
		}
		else if (xmlAnyElementAttribute != null)
		{
			xmlAnyElements.Add(xmlAnyElementAttribute);
		}
	}

	internal static object GetAttr(ICustomAttributeProvider provider, Type attrType)
	{
		object[] customAttributes = provider.GetCustomAttributes(attrType, inherit: false);
		if (customAttributes.Length == 0)
		{
			return null;
		}
		return customAttributes[0];
	}
}
