using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct PrecedingIterator
{
	private XmlNavigatorStack stack;

	private XPathNavigator navCurrent;

	public XPathNavigator Current => navCurrent;

	public void Create(XPathNavigator context, XmlNavigatorFilter filter)
	{
		navCurrent = XmlQueryRuntime.SyncToNavigator(navCurrent, context);
		navCurrent.MoveToRoot();
		stack.Reset();
		if (!navCurrent.IsSamePosition(context))
		{
			if (!filter.IsFiltered(navCurrent))
			{
				stack.Push(navCurrent.Clone());
			}
			while (filter.MoveToFollowing(navCurrent, context))
			{
				stack.Push(navCurrent.Clone());
			}
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
