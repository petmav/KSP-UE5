using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct IdIterator
{
	private XPathNavigator navCurrent;

	private string[] idrefs;

	private int idx;

	public XPathNavigator Current => navCurrent;

	public void Create(XPathNavigator context, string value)
	{
		navCurrent = XmlQueryRuntime.SyncToNavigator(navCurrent, context);
		idrefs = XmlConvert.SplitString(value);
		idx = -1;
	}

	public bool MoveNext()
	{
		do
		{
			idx++;
			if (idx >= idrefs.Length)
			{
				return false;
			}
		}
		while (!navCurrent.MoveToId(idrefs[idx]));
		return true;
	}
}
