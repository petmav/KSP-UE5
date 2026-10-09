using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct XPathPrecedingMergeIterator
{
	private enum IteratorState
	{
		NeedCandidateCurrent,
		HaveCandidateCurrent,
		HaveCurrentHaveNext,
		HaveCurrentNoNext
	}

	private XmlNavigatorFilter filter;

	private IteratorState state;

	private XPathNavigator navCurrent;

	private XPathNavigator navNext;

	private XmlNavigatorStack navStack;

	public XPathNavigator Current => navCurrent;

	public void Create(XmlNavigatorFilter filter)
	{
		this.filter = filter;
		state = IteratorState.NeedCandidateCurrent;
	}

	public IteratorResult MoveNext(XPathNavigator input)
	{
		switch (state)
		{
		case IteratorState.NeedCandidateCurrent:
			if (input == null)
			{
				return IteratorResult.NoMoreNodes;
			}
			navCurrent = XmlQueryRuntime.SyncToNavigator(navCurrent, input);
			state = IteratorState.HaveCandidateCurrent;
			return IteratorResult.NeedInputNode;
		case IteratorState.HaveCandidateCurrent:
			if (input == null)
			{
				state = IteratorState.HaveCurrentNoNext;
			}
			else
			{
				if (navCurrent.ComparePosition(input) != XmlNodeOrder.Unknown)
				{
					navCurrent = XmlQueryRuntime.SyncToNavigator(navCurrent, input);
					return IteratorResult.NeedInputNode;
				}
				navNext = XmlQueryRuntime.SyncToNavigator(navNext, input);
				state = IteratorState.HaveCurrentHaveNext;
			}
			PushAncestors();
			break;
		}
		if (!navStack.IsEmpty)
		{
			do
			{
				if (filter.MoveToFollowing(navCurrent, navStack.Peek()))
				{
					return IteratorResult.HaveCurrentNode;
				}
				navCurrent.MoveTo(navStack.Pop());
			}
			while (!navStack.IsEmpty);
		}
		if (state == IteratorState.HaveCurrentNoNext)
		{
			state = IteratorState.NeedCandidateCurrent;
			return IteratorResult.NoMoreNodes;
		}
		navCurrent = XmlQueryRuntime.SyncToNavigator(navCurrent, navNext);
		state = IteratorState.HaveCandidateCurrent;
		return IteratorResult.HaveCurrentNode;
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
