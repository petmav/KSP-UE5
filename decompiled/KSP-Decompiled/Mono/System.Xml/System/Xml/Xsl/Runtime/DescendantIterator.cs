using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct DescendantIterator
{
	private XmlNavigatorFilter filter;

	private XPathNavigator navCurrent;

	private XPathNavigator navEnd;

	private bool hasFirst;

	public XPathNavigator Current => navCurrent;

	public void Create(XPathNavigator input, XmlNavigatorFilter filter, bool orSelf)
	{
		navCurrent = XmlQueryRuntime.SyncToNavigator(navCurrent, input);
		this.filter = filter;
		if (input.NodeType == XPathNodeType.Root)
		{
			navEnd = null;
		}
		else
		{
			navEnd = XmlQueryRuntime.SyncToNavigator(navEnd, input);
			navEnd.MoveToNonDescendant();
		}
		hasFirst = orSelf && !this.filter.IsFiltered(navCurrent);
	}

	public bool MoveNext()
	{
		if (hasFirst)
		{
			hasFirst = false;
			return true;
		}
		return filter.MoveToFollowing(navCurrent, navEnd);
	}
}
