using System.Collections;

namespace System.Xml.Serialization;

internal abstract class AccessorMapping : Mapping
{
	internal class AccessorComparer : IComparer
	{
		public int Compare(object o1, object o2)
		{
			if (o1 == o2)
			{
				return 0;
			}
			Accessor obj = (Accessor)o1;
			Accessor accessor = (Accessor)o2;
			int weight = obj.Mapping.TypeDesc.Weight;
			int weight2 = accessor.Mapping.TypeDesc.Weight;
			if (weight == weight2)
			{
				return 0;
			}
			if (weight < weight2)
			{
				return 1;
			}
			return -1;
		}
	}

	private TypeDesc typeDesc;

	private AttributeAccessor attribute;

	private ElementAccessor[] elements;

	private ElementAccessor[] sortedElements;

	private TextAccessor text;

	private ChoiceIdentifierAccessor choiceIdentifier;

	private XmlnsAccessor xmlns;

	private bool ignore;

	internal bool IsAttribute => attribute != null;

	internal bool IsText
	{
		get
		{
			if (text != null)
			{
				if (elements != null)
				{
					return elements.Length == 0;
				}
				return true;
			}
			return false;
		}
	}

	internal bool IsParticle
	{
		get
		{
			if (elements != null)
			{
				return elements.Length != 0;
			}
			return false;
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

	internal AttributeAccessor Attribute
	{
		get
		{
			return attribute;
		}
		set
		{
			attribute = value;
		}
	}

	internal ElementAccessor[] Elements
	{
		get
		{
			return elements;
		}
		set
		{
			elements = value;
			sortedElements = null;
		}
	}

	internal ElementAccessor[] ElementsSortedByDerivation
	{
		get
		{
			if (sortedElements != null)
			{
				return sortedElements;
			}
			if (elements == null)
			{
				return null;
			}
			sortedElements = new ElementAccessor[elements.Length];
			Array.Copy(elements, 0, sortedElements, 0, elements.Length);
			SortMostToLeastDerived(sortedElements);
			return sortedElements;
		}
	}

	internal TextAccessor Text
	{
		get
		{
			return text;
		}
		set
		{
			text = value;
		}
	}

	internal ChoiceIdentifierAccessor ChoiceIdentifier
	{
		get
		{
			return choiceIdentifier;
		}
		set
		{
			choiceIdentifier = value;
		}
	}

	internal XmlnsAccessor Xmlns
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

	internal bool Ignore
	{
		get
		{
			return ignore;
		}
		set
		{
			ignore = value;
		}
	}

	internal Accessor Accessor
	{
		get
		{
			if (xmlns != null)
			{
				return xmlns;
			}
			if (attribute != null)
			{
				return attribute;
			}
			if (elements != null && elements.Length != 0)
			{
				return elements[0];
			}
			return text;
		}
	}

	internal bool IsNeedNullable
	{
		get
		{
			if (xmlns != null)
			{
				return false;
			}
			if (attribute != null)
			{
				return false;
			}
			if (elements != null && elements.Length == 1)
			{
				return IsNeedNullableMember(elements[0]);
			}
			return false;
		}
	}

	internal AccessorMapping()
	{
	}

	protected AccessorMapping(AccessorMapping mapping)
		: base(mapping)
	{
		typeDesc = mapping.typeDesc;
		attribute = mapping.attribute;
		elements = mapping.elements;
		sortedElements = mapping.sortedElements;
		text = mapping.text;
		choiceIdentifier = mapping.choiceIdentifier;
		xmlns = mapping.xmlns;
		ignore = mapping.ignore;
	}

	internal static void SortMostToLeastDerived(ElementAccessor[] elements)
	{
		Array.Sort(elements, new AccessorComparer());
	}

	private static bool IsNeedNullableMember(ElementAccessor element)
	{
		if (element.Mapping is ArrayMapping)
		{
			ArrayMapping arrayMapping = (ArrayMapping)element.Mapping;
			if (arrayMapping.Elements != null && arrayMapping.Elements.Length == 1)
			{
				return IsNeedNullableMember(arrayMapping.Elements[0]);
			}
			return false;
		}
		if (element.IsNullable)
		{
			return element.Mapping.TypeDesc.IsValueType;
		}
		return false;
	}

	internal static bool ElementsMatch(ElementAccessor[] a, ElementAccessor[] b)
	{
		if (a == null)
		{
			if (b == null)
			{
				return true;
			}
			return false;
		}
		if (b == null)
		{
			return false;
		}
		if (a.Length != b.Length)
		{
			return false;
		}
		for (int i = 0; i < a.Length; i++)
		{
			if (a[i].Name != b[i].Name || a[i].Namespace != b[i].Namespace || a[i].Form != b[i].Form || a[i].IsNullable != b[i].IsNullable)
			{
				return false;
			}
		}
		return true;
	}

	internal bool Match(AccessorMapping mapping)
	{
		if (Elements != null && Elements.Length != 0)
		{
			if (!ElementsMatch(Elements, mapping.Elements))
			{
				return false;
			}
			if (Text == null)
			{
				return mapping.Text == null;
			}
		}
		if (Attribute != null)
		{
			if (mapping.Attribute == null)
			{
				return false;
			}
			if (Attribute.Name == mapping.Attribute.Name && Attribute.Namespace == mapping.Attribute.Namespace)
			{
				return Attribute.Form == mapping.Attribute.Form;
			}
			return false;
		}
		if (Text != null)
		{
			return mapping.Text != null;
		}
		return mapping.Accessor == null;
	}
}
