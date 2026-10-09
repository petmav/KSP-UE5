using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct ContentMergeIterator
{
	private enum IteratorState
	{
		NeedCurrent,
		HaveCurrentNeedNext,
		HaveCurrentNoNext,
		HaveCurrentHaveNext
	}

	private XmlNavigatorFilter filter;

	private XPathNavigator navCurrent;

	private XPathNavigator navNext;

	private XmlNavigatorStack navStack;

	private IteratorState state;

	public XPathNavigator Current => navCurrent;

	public void Create(XmlNavigatorFilter filter)
	{
		this.filter = filter;
		navStack.Reset();
		state = IteratorState.NeedCurrent;
	}

	public IteratorResult MoveNext(XPathNavigator input)
	{
		return MoveNext(input, isContent: true);
	}

	internal IteratorResult MoveNext(XPathNavigator input, bool isContent)
	{
		switch (state)
		{
		case IteratorState.NeedCurrent:
			if (input == null)
			{
				return IteratorResult.NoMoreNodes;
			}
			navCurrent = XmlQueryRuntime.SyncToNavigator(navCurrent, input);
			if (isContent ? filter.MoveToContent(navCurrent) : filter.MoveToFollowingSibling(navCurrent))
			{
				state = IteratorState.HaveCurrentNeedNext;
			}
			return IteratorResult.NeedInputNode;
		case IteratorState.HaveCurrentNeedNext:
			if (input == null)
			{
				state = IteratorState.HaveCurrentNoNext;
				return IteratorResult.HaveCurrentNode;
			}
			navNext = XmlQueryRuntime.SyncToNavigator(navNext, input);
			if (isContent ? filter.MoveToContent(navNext) : filter.MoveToFollowingSibling(navNext))
			{
				state = IteratorState.HaveCurrentHaveNext;
				return DocOrderMerge();
			}
			return IteratorResult.NeedInputNode;
		case IteratorState.HaveCurrentNoNext:
		case IteratorState.HaveCurrentHaveNext:
			if (isContent ? (!filter.MoveToNextContent(navCurrent)) : (!filter.MoveToFollowingSibling(navCurrent)))
			{
				if (navStack.IsEmpty)
				{
					if (state == IteratorState.HaveCurrentNoNext)
					{
						return IteratorResult.NoMoreNodes;
					}
					navCurrent = XmlQueryRuntime.SyncToNavigator(navCurrent, navNext);
					state = IteratorState.HaveCurrentNeedNext;
					return IteratorResult.NeedInputNode;
				}
				navCurrent = navStack.Pop();
			}
			if (state == IteratorState.HaveCurrentNoNext)
			{
				return IteratorResult.HaveCurrentNode;
			}
			return DocOrderMerge();
		default:
			return IteratorResult.NoMoreNodes;
		}
	}

	private IteratorResult DocOrderMerge()
	{
		switch (navCurrent.ComparePosition(navNext))
		{
		case XmlNodeOrder.Before:
		case XmlNodeOrder.Unknown:
			return IteratorResult.HaveCurrentNode;
		case XmlNodeOrder.After:
			navStack.Push(navCurrent);
			navCurrent = navNext;
			navNext = null;
			break;
		}
		state = IteratorState.HaveCurrentNeedNext;
		return IteratorResult.NeedInputNode;
	}
}
