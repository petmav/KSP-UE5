namespace System.Xml.Xsl.Runtime;

internal class XmlEmptySortKey : XmlSortKey
{
	private bool isEmptyGreatest;

	public bool IsEmptyGreatest => isEmptyGreatest;

	public XmlEmptySortKey(XmlCollation collation)
	{
		isEmptyGreatest = collation.EmptyGreatest != collation.DescendingOrder;
	}

	public override int CompareTo(object obj)
	{
		if (!(obj is XmlEmptySortKey that))
		{
			return -(obj as XmlSortKey).CompareTo(this);
		}
		return BreakSortingTie(that);
	}
}
