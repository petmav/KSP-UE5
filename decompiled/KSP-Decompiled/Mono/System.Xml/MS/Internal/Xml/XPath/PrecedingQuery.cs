using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal sealed class PrecedingQuery : BaseAxisQuery
{
	private XPathNodeIterator workIterator;

	private ClonableStack<XPathNavigator> ancestorStk;

	public override QueryProps Properties => base.Properties | QueryProps.Reverse;

	public PrecedingQuery(Query qyInput, string name, string prefix, XPathNodeType typeTest)
		: base(qyInput, name, prefix, typeTest)
	{
		ancestorStk = new ClonableStack<XPathNavigator>();
	}

	private PrecedingQuery(PrecedingQuery other)
		: base(other)
	{
		workIterator = Query.Clone(other.workIterator);
		ancestorStk = other.ancestorStk.Clone();
	}

	public override void Reset()
	{
		workIterator = null;
		ancestorStk.Clear();
		base.Reset();
	}

	public override XPathNavigator Advance()
	{
		if (workIterator == null)
		{
			XPathNavigator xPathNavigator = qyInput.Advance();
			if (xPathNavigator == null)
			{
				return null;
			}
			XPathNavigator xPathNavigator2 = xPathNavigator.Clone();
			do
			{
				xPathNavigator2.MoveTo(xPathNavigator);
			}
			while ((xPathNavigator = qyInput.Advance()) != null);
			if (xPathNavigator2.NodeType == XPathNodeType.Attribute || xPathNavigator2.NodeType == XPathNodeType.Namespace)
			{
				xPathNavigator2.MoveToParent();
			}
			do
			{
				ancestorStk.Push(xPathNavigator2.Clone());
			}
			while (xPathNavigator2.MoveToParent());
			workIterator = xPathNavigator2.SelectDescendants(XPathNodeType.All, matchSelf: true);
		}
		while (workIterator.MoveNext())
		{
			currentNode = workIterator.Current;
			if (currentNode.IsSamePosition(ancestorStk.Peek()))
			{
				ancestorStk.Pop();
				if (ancestorStk.Count == 0)
				{
					currentNode = null;
					workIterator = null;
					return null;
				}
			}
			else if (matches(currentNode))
			{
				position++;
				return currentNode;
			}
		}
		return null;
	}

	public override XPathNodeIterator Clone()
	{
		return new PrecedingQuery(this);
	}
}
