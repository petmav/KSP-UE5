using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct XPathFollowingMergeIterator
{
	private enum IteratorState
	{
		NeedCandidateCurrent,
		HaveCandidateCurrent,
		HaveCurrentNeedNext,
		HaveCurrentHaveNext,
		HaveCurrentNoNext
	}

	private XmlNavigatorFilter filter;

	private IteratorState state;

	private XPathNavigator navCurrent;

	private XPathNavigator navNext;

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
				return MoveFirst();
			}
			if (navCurrent.IsDescendant(input))
			{
				goto case IteratorState.NeedCandidateCurrent;
			}
			state = IteratorState.HaveCurrentNeedNext;
			goto case IteratorState.HaveCurrentNeedNext;
		case IteratorState.HaveCurrentNeedNext:
			if (input == null)
			{
				state = IteratorState.HaveCurrentNoNext;
				return MoveFirst();
			}
			if (navCurrent.ComparePosition(input) != XmlNodeOrder.Unknown)
			{
				return IteratorResult.NeedInputNode;
			}
			navNext = XmlQueryRuntime.SyncToNavigator(navNext, input);
			state = IteratorState.HaveCurrentHaveNext;
			return MoveFirst();
		default:
			if (!filter.MoveToFollowing(navCurrent, null))
			{
				return MoveFailed();
			}
			return IteratorResult.HaveCurrentNode;
		}
	}

	private IteratorResult MoveFailed()
	{
		if (state == IteratorState.HaveCurrentNoNext)
		{
			state = IteratorState.NeedCandidateCurrent;
			return IteratorResult.NoMoreNodes;
		}
		state = IteratorState.HaveCandidateCurrent;
		XPathNavigator xPathNavigator = navCurrent;
		navCurrent = navNext;
		navNext = xPathNavigator;
		return IteratorResult.NeedInputNode;
	}

	private IteratorResult MoveFirst()
	{
		if (!XPathFollowingIterator.MoveFirst(filter, navCurrent))
		{
			return MoveFailed();
		}
		return IteratorResult.HaveCurrentNode;
	}
}
