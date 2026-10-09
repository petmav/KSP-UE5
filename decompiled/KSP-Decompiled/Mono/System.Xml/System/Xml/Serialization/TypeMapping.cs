namespace System.Xml.Serialization;

internal abstract class TypeMapping : Mapping
{
	private TypeDesc typeDesc;

	private string typeNs;

	private string typeName;

	private bool referencedByElement;

	private bool referencedByTopLevelElement;

	private bool includeInSchema = true;

	private bool reference;

	internal bool ReferencedByTopLevelElement
	{
		get
		{
			return referencedByTopLevelElement;
		}
		set
		{
			referencedByTopLevelElement = value;
		}
	}

	internal bool ReferencedByElement
	{
		get
		{
			if (!referencedByElement)
			{
				return referencedByTopLevelElement;
			}
			return true;
		}
		set
		{
			referencedByElement = value;
		}
	}

	internal string Namespace
	{
		get
		{
			return typeNs;
		}
		set
		{
			typeNs = value;
		}
	}

	internal string TypeName
	{
		get
		{
			return typeName;
		}
		set
		{
			typeName = value;
		}
	}

	internal TypeDesc TypeDesc
	{
		get
		{
			return typeDesc;
		}
		set
		{
			typeDesc = value;
		}
	}

	internal bool IncludeInSchema
	{
		get
		{
			return includeInSchema;
		}
		set
		{
			includeInSchema = value;
		}
	}

	internal virtual bool IsList
	{
		get
		{
			return false;
		}
		set
		{
		}
	}

	internal bool IsReference
	{
		get
		{
			return reference;
		}
		set
		{
			reference = value;
		}
	}

	internal bool IsAnonymousType
	{
		get
		{
			if (typeName != null)
			{
				return typeName.Length == 0;
			}
			return true;
		}
	}

	internal virtual string DefaultElementName
	{
		get
		{
			if (!IsAnonymousType)
			{
				return typeName;
			}
			return XmlConvert.EncodeLocalName(typeDesc.Name);
		}
	}
}
