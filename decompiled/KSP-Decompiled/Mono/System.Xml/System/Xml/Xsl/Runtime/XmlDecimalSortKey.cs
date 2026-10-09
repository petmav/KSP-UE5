namespace System.Xml.Xsl.Runtime;

internal class XmlDecimalSortKey : XmlSortKey
{
	private decimal decVal;

	public XmlDecimalSortKey(decimal value, XmlCollation collation)
	{
		decVal = (collation.DescendingOrder ? (-value) : value);
	}

	public override int CompareTo(object obj)
	{
		if (!(obj is XmlDecimalSortKey xmlDecimalSortKey))
		{
			return CompareToEmpty(obj);
		}
		int num = decimal.Compare(decVal, xmlDecimalSortKey.decVal);
		if (num == 0)
		{
			return BreakSortingTie(xmlDecimalSortKey);
		}
		return num;
	}
}
