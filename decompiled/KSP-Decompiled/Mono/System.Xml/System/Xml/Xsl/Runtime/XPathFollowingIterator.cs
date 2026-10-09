using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct XPathFollowingIterator
{
	private XmlNavigatorFilter filter;

	private XPathNavigator navCurrent;

	private bool needFirst;

	public XPathNavigator Current => navCurrent;

	public void Create(XPathNavigator input, XmlNavigatorFilter filter)
	{
		navCurrent = XmlQueryRuntime.SyncToNavigator(navCurrent, input);
		this.filter = filter;
		needFirst = true;
	}

	public bool MoveNext()
	{
		if (needFirst)
		{
			if (!MoveFirst(filter, navCurrent))
			{
				return false;
			}
			needFirst = false;
			return true;
		}
		return filter.MoveToFollowing(navCurrent, null);
	}

	internal static bool MoveFirst(XmlNavigatorFilter filter, XPathNavigator nav)
	{
		if (nav.NodeType == XPathNodeType.Attribute || nav.NodeType == XPathNodeType.Namespace)
		{
			if (!nav.MoveToParent())
			{
				return false;
			}
			if (!filter.MoveToFollowing(nav, null))
			{
				return false;
			}
		}
		else
		{
			if (!nav.MoveToNonDescendant())
			{
				return false;
			}
			if (filter.IsFiltered(nav) && !filter.MoveToFollowing(nav, null))
			{
				return false;
			}
		}
		return true;
	}
}
