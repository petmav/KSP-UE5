namespace System.Xml.Xsl.Runtime;

internal class XmlIntSortKey : XmlSortKey
{
	private int intVal;

	public XmlIntSortKey(int value, XmlCollation collation)
	{
		intVal = (collation.DescendingOrder ? (~value) : value);
	}

	public override int CompareTo(object obj)
	{
		if (!(obj is XmlIntSortKey xmlIntSortKey))
		{
			return CompareToEmpty(obj);
		}
		if (intVal == xmlIntSortKey.intVal)
		{
			return BreakSortingTie(xmlIntSortKey);
		}
		if (intVal >= xmlIntSortKey.intVal)
		{
			return 1;
		}
		return -1;
	}
}
