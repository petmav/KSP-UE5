using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct AncestorDocOrderIterator
{
	private XmlNavigatorStack stack;

	private XPathNavigator navCurrent;

	public XPathNavigator Current => navCurrent;

	public void Create(XPathNavigator context, XmlNavigatorFilter filter, bool orSelf)
	{
		AncestorIterator ancestorIterator = default(AncestorIterator);
		ancestorIterator.Create(context, filter, orSelf);
		stack.Reset();
		while (ancestorIterator.MoveNext())
		{
			stack.Push(ancestorIterator.Current.Clone());
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
