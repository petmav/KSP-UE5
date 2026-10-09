namespace System.Xml.Xsl.Runtime;

internal abstract class XmlSortKey : IComparable
{
	private int priority;

	private XmlSortKey nextKey;

	public int Priority
	{
		set
		{
			for (XmlSortKey xmlSortKey = this; xmlSortKey != null; xmlSortKey = xmlSortKey.nextKey)
			{
				xmlSortKey.priority = value;
			}
		}
	}

	public XmlSortKey AddSortKey(XmlSortKey sortKey)
	{
		if (nextKey != null)
		{
			nextKey.AddSortKey(sortKey);
		}
		else
		{
			nextKey = sortKey;
		}
		return this;
	}

	protected int BreakSortingTie(XmlSortKey that)
	{
		if (nextKey != null)
		{
			return nextKey.CompareTo(that.nextKey);
		}
		if (priority >= that.priority)
		{
			return 1;
		}
		return -1;
	}

	protected int CompareToEmpty(object obj)
	{
		if (!(obj as XmlEmptySortKey).IsEmptyGreatest)
		{
			return 1;
		}
		return -1;
	}

	public abstract int CompareTo(object that);
}
