using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct UnionIterator
{
	private enum IteratorState
	{
		InitLeft,
		NeedLeft,
		NeedRight,
		LeftIsCurrent,
		RightIsCurrent
	}

	private XmlQueryRuntime runtime;

	private XPathNavigator navCurr;

	private XPathNavigator navOther;

	private IteratorState state;

	public XPathNavigator Current => navCurr;

	public void Create(XmlQueryRuntime runtime)
	{
		this.runtime = runtime;
		state = IteratorState.InitLeft;
	}

	public SetIteratorResult MoveNext(XPathNavigator nestedNavigator)
	{
		switch (state)
		{
		case IteratorState.InitLeft:
			navOther = nestedNavigator;
			state = IteratorState.NeedRight;
			return SetIteratorResult.InitRightIterator;
		case IteratorState.NeedLeft:
			navCurr = nestedNavigator;
			state = IteratorState.LeftIsCurrent;
			break;
		case IteratorState.NeedRight:
			navCurr = nestedNavigator;
			state = IteratorState.RightIsCurrent;
			break;
		case IteratorState.LeftIsCurrent:
			state = IteratorState.NeedLeft;
			return SetIteratorResult.NeedLeftNode;
		case IteratorState.RightIsCurrent:
			state = IteratorState.NeedRight;
			return SetIteratorResult.NeedRightNode;
		}
		if (navCurr == null)
		{
			if (navOther == null)
			{
				return SetIteratorResult.NoMoreNodes;
			}
			Swap();
		}
		else if (navOther != null)
		{
			int num = runtime.ComparePosition(navOther, navCurr);
			if (num == 0)
			{
				if (state == IteratorState.LeftIsCurrent)
				{
					state = IteratorState.NeedLeft;
					return SetIteratorResult.NeedLeftNode;
				}
				state = IteratorState.NeedRight;
				return SetIteratorResult.NeedRightNode;
			}
			if (num < 0)
			{
				Swap();
			}
		}
		return SetIteratorResult.HaveCurrentNode;
	}

	private void Swap()
	{
		XPathNavigator xPathNavigator = navCurr;
		navCurr = navOther;
		navOther = xPathNavigator;
		if (state == IteratorState.LeftIsCurrent)
		{
			state = IteratorState.RightIsCurrent;
		}
		else
		{
			state = IteratorState.LeftIsCurrent;
		}
	}
}
