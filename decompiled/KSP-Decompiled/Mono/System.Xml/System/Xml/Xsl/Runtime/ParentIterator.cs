using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct ParentIterator
{
	private XPathNavigator navCurrent;

	private bool haveCurrent;

	public XPathNavigator Current => navCurrent;

	public void Create(XPathNavigator context, XmlNavigatorFilter filter)
	{
		navCurrent = XmlQueryRuntime.SyncToNavigator(navCurrent, context);
		haveCurrent = navCurrent.MoveToParent() && !filter.IsFiltered(navCurrent);
	}

	public bool MoveNext()
	{
		if (haveCurrent)
		{
			haveCurrent = false;
			return true;
		}
		return false;
	}
}
