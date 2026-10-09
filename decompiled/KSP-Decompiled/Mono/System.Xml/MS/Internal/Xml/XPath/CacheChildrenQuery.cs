using System.Xml;
using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal sealed class CacheChildrenQuery : ChildrenQuery
{
	private XPathNavigator nextInput;

	private ClonableStack<XPathNavigator> elementStk;

	private ClonableStack<int> positionStk;

	private bool needInput;

	public CacheChildrenQuery(Query qyInput, string name, string prefix, XPathNodeType type)
		: base(qyInput, name, prefix, type)
	{
		elementStk = new ClonableStack<XPathNavigator>();
		positionStk = new ClonableStack<int>();
		needInput = true;
	}

	private CacheChildrenQuery(CacheChildrenQuery other)
		: base(other)
	{
		nextInput = Query.Clone(other.nextInput);
		elementStk = other.elementStk.Clone();
		positionStk = other.positionStk.Clone();
		needInput = other.needInput;
	}

	public override void Reset()
	{
		nextInput = null;
		elementStk.Clear();
		positionStk.Clear();
		needInput = true;
		base.Reset();
	}

	public override XPathNavigator Advance()
	{
		do
		{
			IL_0000:
			if (needInput)
			{
				if (elementStk.Count == 0)
				{
					currentNode = GetNextInput();
					if (currentNode == null)
					{
						return null;
					}
					if (!currentNode.MoveToFirstChild())
					{
						goto IL_0000;
					}
					position = 0;
				}
				else
				{
					currentNode = elementStk.Pop();
					position = positionStk.Pop();
					if (!DecideNextNode())
					{
						goto IL_0000;
					}
				}
				needInput = false;
			}
			else if (!currentNode.MoveToNext() || !DecideNextNode())
			{
				needInput = true;
				goto IL_0000;
			}
		}
		while (!matches(currentNode));
		position++;
		return currentNode;
	}

	private bool DecideNextNode()
	{
		nextInput = GetNextInput();
		if (nextInput != null && Query.CompareNodes(currentNode, nextInput) == XmlNodeOrder.After)
		{
			elementStk.Push(currentNode);
			positionStk.Push(position);
			currentNode = nextInput;
			nextInput = null;
			if (!currentNode.MoveToFirstChild())
			{
				return false;
			}
			position = 0;
		}
		return true;
	}

	private XPathNavigator GetNextInput()
	{
		XPathNavigator xPathNavigator;
		if (nextInput != null)
		{
			xPathNavigator = nextInput;
			nextInput = null;
		}
		else
		{
			xPathNavigator = qyInput.Advance();
			if (xPathNavigator != null)
			{
				xPathNavigator = xPathNavigator.Clone();
			}
		}
		return xPathNavigator;
	}

	public override XPathNodeIterator Clone()
	{
		return new CacheChildrenQuery(this);
	}
}
