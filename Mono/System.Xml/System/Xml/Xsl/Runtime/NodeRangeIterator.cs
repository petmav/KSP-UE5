using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct NodeRangeIterator
{
	private enum IteratorState
	{
		HaveCurrent,
		NeedCurrent,
		HaveCurrentNoNext,
		NoNext
	}

	private XmlNavigatorFilter filter;

	private XPathNavigator navCurrent;

	private XPathNavigator navEnd;

	private IteratorState state;

	public XPathNavigator Current => navCurrent;

	public void Create(XPathNavigator start, XmlNavigatorFilter filter, XPathNavigator end)
	{
		navCurrent = XmlQueryRuntime.SyncToNavigator(navCurrent, start);
		navEnd = XmlQueryRuntime.SyncToNavigator(navEnd, end);
		this.filter = filter;
		if (start.IsSamePosition(end))
		{
			state = ((!filter.IsFiltered(start)) ? IteratorState.HaveCurrentNoNext : IteratorState.NoNext);
		}
		else
		{
			state = (filter.IsFiltered(start) ? IteratorState.NeedCurrent : IteratorState.HaveCurrent);
		}
	}

	public bool MoveNext()
	{
		switch (state)
		{
		case IteratorState.HaveCurrent:
			state = IteratorState.NeedCurrent;
			return true;
		case IteratorState.NeedCurrent:
			if (!filter.MoveToFollowing(navCurrent, navEnd))
			{
				if (filter.IsFiltered(navEnd))
				{
					state = IteratorState.NoNext;
					return false;
				}
				navCurrent.MoveTo(navEnd);
				state = IteratorState.NoNext;
			}
			return true;
		case IteratorState.HaveCurrentNoNext:
			state = IteratorState.NoNext;
			return true;
		default:
			return false;
		}
	}
}
