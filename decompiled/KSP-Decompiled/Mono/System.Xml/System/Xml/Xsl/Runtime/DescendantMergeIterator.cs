using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct DescendantMergeIterator
{
	private enum IteratorState
	{
		NoPrevious,
		NeedCurrent,
		NeedDescendant
	}

	private XmlNavigatorFilter filter;

	private XPathNavigator navCurrent;

	private XPathNavigator navRoot;

	private XPathNavigator navEnd;

	private IteratorState state;

	private bool orSelf;

	public XPathNavigator Current => navCurrent;

	public void Create(XmlNavigatorFilter filter, bool orSelf)
	{
		this.filter = filter;
		state = IteratorState.NoPrevious;
		this.orSelf = orSelf;
	}

	public IteratorResult MoveNext(XPathNavigator input)
	{
		if (state != IteratorState.NeedDescendant)
		{
			if (input == null)
			{
				return IteratorResult.NoMoreNodes;
			}
			if (state != 0 && navRoot.IsDescendant(input))
			{
				return IteratorResult.NeedInputNode;
			}
			navCurrent = XmlQueryRuntime.SyncToNavigator(navCurrent, input);
			navRoot = XmlQueryRuntime.SyncToNavigator(navRoot, input);
			navEnd = XmlQueryRuntime.SyncToNavigator(navEnd, input);
			navEnd.MoveToNonDescendant();
			state = IteratorState.NeedDescendant;
			if (orSelf && !filter.IsFiltered(input))
			{
				return IteratorResult.HaveCurrentNode;
			}
		}
		if (filter.MoveToFollowing(navCurrent, navEnd))
		{
			return IteratorResult.HaveCurrentNode;
		}
		state = IteratorState.NeedCurrent;
		return IteratorResult.NeedInputNode;
	}
}
