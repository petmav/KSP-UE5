using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct XPathPrecedingIterator
{
	private XmlNavigatorStack stack;

	private XPathNavigator navCurrent;

	public XPathNavigator Current => navCurrent;

	public void Create(XPathNavigator context, XmlNavigatorFilter filter)
	{
		XPathPrecedingDocOrderIterator xPathPrecedingDocOrderIterator = default(XPathPrecedingDocOrderIterator);
		xPathPrecedingDocOrderIterator.Create(context, filter);
		stack.Reset();
		while (xPathPrecedingDocOrderIterator.MoveNext())
		{
			stack.Push(xPathPrecedingDocOrderIterator.Current.Clone());
		}
	}

	public bool MoveNext()
	{
		if (stack.IsEmpty)
		{
			return false;
		}
		navCurrent = stack.Pop();
		return true;
	}
}
