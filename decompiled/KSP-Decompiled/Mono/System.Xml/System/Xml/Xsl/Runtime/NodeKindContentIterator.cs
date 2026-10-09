using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct NodeKindContentIterator
{
	private XPathNodeType nodeType;

	private XPathNavigator navCurrent;

	private bool needFirst;

	public XPathNavigator Current => navCurrent;

	public void Create(XPathNavigator context, XPathNodeType nodeType)
	{
		navCurrent = XmlQueryRuntime.SyncToNavigator(navCurrent, context);
		this.nodeType = nodeType;
		needFirst = true;
	}

	public bool MoveNext()
	{
		if (needFirst)
		{
			needFirst = !navCurrent.MoveToChild(nodeType);
			return !needFirst;
		}
		return navCurrent.MoveToNext(nodeType);
	}
}
