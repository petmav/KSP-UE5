using System.Collections.Generic;
using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal sealed class FollSiblingQuery : BaseAxisQuery
{
	private ClonableStack<XPathNavigator> elementStk;

	private List<XPathNavigator> parentStk;

	private XPathNavigator nextInput;

	public FollSiblingQuery(Query qyInput, string name, string prefix, XPathNodeType type)
		: base(qyInput, name, prefix, type)
	{
		elementStk = new ClonableStack<XPathNavigator>();
		parentStk = new List<XPathNavigator>();
	}

	private FollSiblingQuery(FollSiblingQuery other)
		: base(other)
	{
		elementStk = other.elementStk.Clone();
		parentStk = new List<XPathNavigator>(other.parentStk);
		nextInput = Query.Clone(other.nextInput);
	}

	public override void Reset()
	{
		elementStk.Clear();
		parentStk.Clear();
		nextInput = null;
		base.Reset();
	}

	private bool Visited(XPathNavigator nav)
	{
		XPathNavigator xPathNavigator = nav.Clone();
		xPathNavigator.MoveToParent();
		for (int i = 0; i < parentStk.Count; i++)
		{
			if (xPathNavigator.IsSamePosition(parentStk[i]))
			{
				return true;
			}
		}
		parentStk.Add(xPathNavigator);
		return false;
	}

	private XPathNavigator FetchInput()
	{
		XPathNavigator xPathNavigator;
		do
		{
			xPathNavigator = qyInput.Advance();
			if (xPathNavigator == null)
			{
				return null;
			}
		}
		while (Visited(xPathNavigator));
		return xPathNavigator.Clone();
	}

	public override XPathNavigator Advance()
	{
		while (true)
		{
			if (currentNode == null)
			{
				if (nextInput == null)
				{
					nextInput = FetchInput();
				}
				if (elementStk.Count == 0)
				{
					if (nextInput == null)
					{
						break;
					}
					currentNode = nextInput;
					nextInput = FetchInput();
				}
				else
				{
					currentNode = elementStk.Pop();
				}
			}
			while (currentNode.IsDescendant(nextInput))
			{
				elementStk.Push(currentNode);
				currentNode = nextInput;
				nextInput = qyInput.Advance();
				if (nextInput != null)
				{
					nextInput = nextInput.Clone();
				}
			}
			while (currentNode.MoveToNext())
			{
				if (matches(currentNode))
				{
					position++;
					return currentNode;
				}
			}
			currentNode = null;
		}
		return null;
	}

	public override XPathNodeIterator Clone()
	{
		return new FollSiblingQuery(this);
	}
}
