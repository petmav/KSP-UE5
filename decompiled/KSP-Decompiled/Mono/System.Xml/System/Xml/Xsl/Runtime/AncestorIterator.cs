using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct AncestorIterator
{
	private XmlNavigatorFilter filter;

	private XPathNavigator navCurrent;

	private bool haveCurrent;

	public XPathNavigator Current => navCurrent;

	public void Create(XPathNavigator context, XmlNavigatorFilter filter, bool orSelf)
	{
		this.filter = filter;
		navCurrent = XmlQueryRuntime.SyncToNavigator(navCurrent, context);
		haveCurrent = orSelf && !this.filter.IsFiltered(navCurrent);
	}

	public bool MoveNext()
	{
		if (haveCurrent)
		{
			haveCurrent = false;
			return true;
		}
		while (navCurrent.MoveToParent())
		{
			if (!filter.IsFiltered(navCurrent))
			{
				return true;
			}
		}
		return false;
	}
}
