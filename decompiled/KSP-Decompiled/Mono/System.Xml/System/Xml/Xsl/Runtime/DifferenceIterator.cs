using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct DifferenceIterator
{
	private enum IteratorState
	{
		InitLeft,
		NeedLeft,
		NeedRight,
		NeedLeftAndRight,
		HaveCurrent
	}

	private XmlQueryRuntime runtime;

	private XPathNavigator navLeft;

	private XPathNavigator navRight;

	private IteratorState state;

	public XPathNavigator Current => navLeft;

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
			navLeft = nestedNavigator;
			state = IteratorState.NeedRight;
			return SetIteratorResult.InitRightIterator;
		case IteratorState.NeedLeft:
			navLeft = nestedNavigator;
			break;
		case IteratorState.NeedRight:
			navRight = nestedNavigator;
			break;
		case IteratorState.NeedLeftAndRight:
			navLeft = nestedNavigator;
			state = IteratorState.NeedRight;
			return SetIteratorResult.NeedRightNode;
		case IteratorState.HaveCurrent:
			state = IteratorState.NeedLeft;
			return SetIteratorResult.NeedLeftNode;
		}
		if (navLeft == null)
		{
			return SetIteratorResult.NoMoreNodes;
		}
		if (navRight != null)
		{
			int num = runtime.ComparePosition(navLeft, navRight);
			if (num == 0)
			{
				state = IteratorState.NeedLeftAndRight;
				return SetIteratorResult.NeedLeftNode;
			}
			if (num > 0)
			{
				state = IteratorState.NeedRight;
				return SetIteratorResult.NeedRightNode;
			}
		}
		state = IteratorState.HaveCurrent;
		return SetIteratorResult.HaveCurrentNode;
	}
}
