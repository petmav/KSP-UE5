namespace System.Xml.Xsl.Runtime;

internal class XmlIntegerSortKey : XmlSortKey
{
	private long longVal;

	public XmlIntegerSortKey(long value, XmlCollation collation)
	{
		longVal = (collation.DescendingOrder ? (~value) : value);
	}

	public override int CompareTo(object obj)
	{
		if (!(obj is XmlIntegerSortKey xmlIntegerSortKey))
		{
			return CompareToEmpty(obj);
		}
		if (longVal == xmlIntegerSortKey.longVal)
		{
			return BreakSortingTie(xmlIntegerSortKey);
		}
		if (longVal >= xmlIntegerSortKey.longVal)
		{
			return 1;
		}
		return -1;
	}
}
