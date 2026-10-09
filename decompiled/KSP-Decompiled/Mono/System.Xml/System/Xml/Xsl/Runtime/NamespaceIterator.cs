using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct NamespaceIterator
{
	private XPathNavigator navCurrent;

	private XmlNavigatorStack navStack;

	public XPathNavigator Current => navCurrent;

	public void Create(XPathNavigator context)
	{
		navStack.Reset();
		if (!context.MoveToFirstNamespace(XPathNamespaceScope.All))
		{
			return;
		}
		do
		{
			if (context.LocalName.Length != 0 || context.Value.Length != 0)
			{
				navStack.Push(context.Clone());
			}
		}
		while (context.MoveToNextNamespace(XPathNamespaceScope.All));
		context.MoveToParent();
	}

	public bool MoveNext()
	{
		if (navStack.IsEmpty)
		{
			return false;
		}
		navCurrent = navStack.Pop();
		return true;
	}
}
