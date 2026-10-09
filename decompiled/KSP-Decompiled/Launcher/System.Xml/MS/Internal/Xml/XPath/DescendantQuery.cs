using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal class DescendantQuery : DescendantBaseQuery
{
	private XPathNodeIterator nodeIterator;

	internal DescendantQuery(Query qyParent, string Name, string Prefix, XPathNodeType Type, bool matchSelf, bool abbrAxis)
		: base(qyParent, Name, Prefix, Type, matchSelf, abbrAxis)
	{
	}

	public DescendantQuery(DescendantQuery other)
		: base(other)
	{
		nodeIterator = Query.Clone(other.nodeIterator);
	}

	public override void Reset()
	{
		nodeIterator = null;
		base.Reset();
	}

	public override XPathNavigator Advance()
	{
		while (true)
		{
			if (nodeIterator == null)
			{
				position = 0;
				XPathNavigator xPathNavigator = qyInput.Advance();
				if (xPathNavigator == null)
				{
					return null;
				}
				if (base.NameTest)
				{
					if (base.TypeTest == XPathNodeType.ProcessingInstruction)
					{
						nodeIterator = new IteratorFilter(xPathNavigator.SelectDescendants(base.TypeTest, matchSelf), base.Name);
					}
					else
					{
						nodeIterator = xPathNavigator.SelectDescendants(base.Name, base.Namespace, matchSelf);
					}
				}
				else
				{
					nodeIterator = xPathNavigator.SelectDescendants(base.TypeTest, matchSelf);
				}
			}
			if (nodeIterator.MoveNext())
			{
				break;
			}
			nodeIterator = null;
		}
		position++;
		currentNode = nodeIterator.Current;
		return currentNode;
	}

	public override XPathNodeIterator Clone()
	{
		return new DescendantQuery(this);
	}
}
