using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct ContentIterator
{
	private XPathNavigator navCurrent;

	private bool needFirst;

	public XPathNavigator Current => navCurrent;

	public void Create(XPathNavigator context)
	{
		navCurrent = XmlQueryRuntime.SyncToNavigator(navCurrent, context);
		needFirst = true;
	}

	public bool MoveNext()
	{
		if (needFirst)
		{
			needFirst = !navCurrent.MoveToFirstChild();
			return !needFirst;
		}
		return navCurrent.MoveToNext();
	}
}
