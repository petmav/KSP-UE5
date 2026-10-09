using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct ElementContentIterator
{
	private string localName;

	private string ns;

	private XPathNavigator navCurrent;

	private bool needFirst;

	public XPathNavigator Current => navCurrent;

	public void Create(XPathNavigator context, string localName, string ns)
	{
		navCurrent = XmlQueryRuntime.SyncToNavigator(navCurrent, context);
		this.localName = localName;
		this.ns = ns;
		needFirst = true;
	}

	public bool MoveNext()
	{
		if (needFirst)
		{
			needFirst = !navCurrent.MoveToChild(localName, ns);
			return !needFirst;
		}
		return navCurrent.MoveToNext(localName, ns);
	}
}
