namespace System.Xml.Xsl.Runtime;

internal class XmlDoubleSortKey : XmlSortKey
{
	private double dblVal;

	private bool isNaN;

	public XmlDoubleSortKey(double value, XmlCollation collation)
	{
		if (double.IsNaN(value))
		{
			isNaN = true;
			dblVal = ((collation.EmptyGreatest != collation.DescendingOrder) ? double.PositiveInfinity : double.NegativeInfinity);
		}
		else
		{
			dblVal = (collation.DescendingOrder ? (0.0 - value) : value);
		}
	}

	public override int CompareTo(object obj)
	{
		if (!(obj is XmlDoubleSortKey xmlDoubleSortKey))
		{
			if (isNaN)
			{
				return BreakSortingTie(obj as XmlSortKey);
			}
			return CompareToEmpty(obj);
		}
		if (dblVal == xmlDoubleSortKey.dblVal)
		{
			if (isNaN)
			{
				if (xmlDoubleSortKey.isNaN)
				{
					return BreakSortingTie(xmlDoubleSortKey);
				}
				if (dblVal != double.NegativeInfinity)
				{
					return 1;
				}
				return -1;
			}
			if (xmlDoubleSortKey.isNaN)
			{
				if (xmlDoubleSortKey.dblVal != double.NegativeInfinity)
				{
					return -1;
				}
				return 1;
			}
			return BreakSortingTie(xmlDoubleSortKey);
		}
		if (!(dblVal < xmlDoubleSortKey.dblVal))
		{
			return 1;
		}
		return -1;
	}
}
