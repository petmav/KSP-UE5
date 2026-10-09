using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct PrecedingSiblingDocOrderIterator
{
	private XmlNavigatorFilter filter;

	private XPathNavigator navCurrent;

	private XPathNavigator navEnd;

	private bool needFirst;

	private bool useCompPos;

	public XPathNavigator Current => navCurrent;

	public void Create(XPathNavigator context, XmlNavigatorFilter filter)
	{
		this.filter = filter;
		navCurrent = XmlQueryRuntime.SyncToNavigator(navCurrent, context);
		navEnd = XmlQueryRuntime.SyncToNavigator(navEnd, context);
		needFirst = true;
		useCompPos = this.filter.IsFiltered(context);
	}

	public bool MoveNext()
	{
		if (needFirst)
		{
			if (!navCurrent.MoveToParent())
			{
				return false;
			}
			if (!filter.MoveToContent(navCurrent))
			{
				return false;
			}
			needFirst = false;
		}
		else if (!filter.MoveToFollowingSibling(navCurrent))
		{
			return false;
		}
		if (useCompPos)
		{
			return navCurrent.ComparePosition(navEnd) == XmlNodeOrder.Before;
		}
		if (navCurrent.IsSamePosition(navEnd))
		{
			useCompPos = true;
			return false;
		}
		return true;
	}
}
