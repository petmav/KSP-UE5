using System.ComponentModel;
using System.Reflection;

namespace System.Xml.Serialization;

public class SoapAttributes
{
	private bool soapIgnore;

	private SoapTypeAttribute soapType;

	private SoapElementAttribute soapElement;

	private SoapAttributeAttribute soapAttribute;

	private SoapEnumAttribute soapEnum;

	private object soapDefaultValue;

	internal SoapAttributeFlags SoapFlags
	{
		get
		{
			SoapAttributeFlags soapAttributeFlags = (SoapAttributeFlags)0;
			if (soapElement != null)
			{
				soapAttributeFlags |= SoapAttributeFlags.Element;
			}
			if (soapAttribute != null)
			{
				soapAttributeFlags |= SoapAttributeFlags.Attribute;
			}
			if (soapEnum != null)
			{
				soapAttributeFlags |= SoapAttributeFlags.Enum;
			}
			if (soapType != null)
			{
				soapAttributeFlags |= SoapAttributeFlags.Type;
			}
			return soapAttributeFlags;
		}
	}

	public SoapTypeAttribute SoapType
	{
		get
		{
			return soapType;
		}
		set
		{
			soapType = value;
		}
	}

	public SoapEnumAttribute SoapEnum
	{
		get
		{
			return soapEnum;
		}
		set
		{
			soapEnum = value;
		}
	}

	public bool SoapIgnore
	{
		get
		{
			return soapIgnore;
		}
		set
		{
			soapIgnore = value;
		}
	}

	public SoapElementAttribute SoapElement
	{
		get
		{
			return soapElement;
		}
		set
		{
			soapElement = value;
		}
	}

	public SoapAttributeAttribute SoapAttribute
	{
		get
		{
			return soapAttribute;
		}
		set
		{
			soapAttribute = value;
		}
	}

	public object SoapDefaultValue
	{
		get
		{
			return soapDefaultValue;
		}
		set
		{
			soapDefaultValue = value;
		}
	}

	public SoapAttributes()
	{
	}

	public SoapAttributes(ICustomAttributeProvider provider)
	{
		object[] customAttributes = provider.GetCustomAttributes(inherit: false);
		for (int i = 0; i < customAttributes.Length; i++)
		{
			if (customAttributes[i] is SoapIgnoreAttribute || customAttributes[i] is ObsoleteAttribute)
			{
				soapIgnore = true;
				break;
			}
			if (customAttributes[i] is SoapElementAttribute)
			{
				soapElement = (SoapElementAttribute)customAttributes[i];
			}
			else if (customAttributes[i] is SoapAttributeAttribute)
			{
				soapAttribute = (SoapAttributeAttribute)customAttributes[i];
			}
			else if (customAttributes[i] is SoapTypeAttribute)
			{
				soapType = (SoapTypeAttribute)customAttributes[i];
			}
			else if (customAttributes[i] is SoapEnumAttribute)
			{
				soapEnum = (SoapEnumAttribute)customAttributes[i];
			}
			else if (customAttributes[i] is DefaultValueAttribute)
			{
				soapDefaultValue = ((DefaultValueAttribute)customAttributes[i]).Value;
			}
		}
		if (soapIgnore)
		{
			soapElement = null;
			soapAttribute = null;
			soapType = null;
			soapEnum = null;
			soapDefaultValue = null;
		}
	}
}
