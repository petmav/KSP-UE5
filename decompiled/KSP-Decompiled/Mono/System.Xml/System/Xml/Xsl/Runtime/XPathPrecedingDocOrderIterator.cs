using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct XPathPrecedingDocOrderIterator
{
	private XmlNavigatorFilter filter;

	private XPathNavigator navCurrent;

	private XmlNavigatorStack navStack;

	public XPathNavigator Current => navCurrent;

	public void Create(XPathNavigator input, XmlNavigatorFilter filter)
	{
		navCurrent = XmlQueryRuntime.SyncToNavigator(navCurrent, input);
		this.filter = filter;
		PushAncestors();
	}

	public bool MoveNext()
	{
		if (!navStack.IsEmpty)
		{
			do
			{
				if (filter.MoveToFollowing(navCurrent, navStack.Peek()))
				{
					return true;
				}
				navCurrent.MoveTo(navStack.Pop());
			}
			while (!navStack.IsEmpty);
		}
		return false;
	}

	private void PushAncestors()
	{
		navStack.Reset();
		do
		{
			navStack.Push(navCurrent.Clone());
		}
		while (navCurrent.MoveToParent());
		navStack.Pop();
	}
}
