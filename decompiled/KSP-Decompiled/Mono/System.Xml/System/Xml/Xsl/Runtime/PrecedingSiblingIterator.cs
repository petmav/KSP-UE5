using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct PrecedingSiblingIterator
{
	private XmlNavigatorFilter filter;

	private XPathNavigator navCurrent;

	public XPathNavigator Current => navCurrent;

	public void Create(XPathNavigator context, XmlNavigatorFilter filter)
	{
		navCurrent = XmlQueryRuntime.SyncToNavigator(navCurrent, context);
		this.filter = filter;
	}

	public bool MoveNext()
	{
		return filter.MoveToPreviousSibling(navCurrent);
	}
}
