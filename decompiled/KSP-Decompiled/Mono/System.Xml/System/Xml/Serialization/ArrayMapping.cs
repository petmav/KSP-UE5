namespace System.Xml.Serialization;

internal class ArrayMapping : TypeMapping
{
	private ElementAccessor[] elements;

	private ElementAccessor[] sortedElements;

	private ArrayMapping next;

	private StructMapping topLevelMapping;

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
			AccessorMapping.SortMostToLeastDerived(sortedElements);
			return sortedElements;
		}
	}

	internal ArrayMapping Next
	{
		get
		{
			return next;
		}
		set
		{
			next = value;
		}
	}

	internal StructMapping TopLevelMapping
	{
		get
		{
			return topLevelMapping;
		}
		set
		{
			topLevelMapping = value;
		}
	}
}
